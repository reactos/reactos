/*
 * Copyright 2001, Ove Kåven, TransGaming Technologies Inc.
 * Copyright 2002 Greg Turner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winsvc.h"
#include "irot.h"
#include "epm.h"
#include "irpcss.h"
#ifdef __REACTOS__
#include <objbase.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static WCHAR rpcssW[] = L"RpcSs";
static HANDLE exit_event;
static SERVICE_STATUS_HANDLE service_handle;

struct registered_class
{
    struct list entry;
    GUID clsid;
    unsigned int cookie;
    PMInterfacePointer object;
    unsigned int single_use : 1;
};

static CRITICAL_SECTION registered_classes_cs = { NULL, -1, 0, 0, 0, 0 };
static struct list registered_classes = LIST_INIT(registered_classes);

HRESULT __cdecl irpcss_server_register(handle_t h, const GUID *clsid, unsigned int flags,
        PMInterfacePointer object, unsigned int *cookie)
{
    struct registered_class *entry;
    static LONG next_cookie;

    if (!(entry = calloc(1, sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->clsid = *clsid;
    entry->single_use = !(flags & (REGCLS_MULTIPLEUSE | REGCLS_MULTI_SEPARATE));
    if (!(entry->object = malloc(FIELD_OFFSET(MInterfacePointer, abData[object->ulCntData]))))
    {
        free(entry);
        return E_OUTOFMEMORY;
    }
    entry->object->ulCntData = object->ulCntData;
    memcpy(&entry->object->abData, object->abData, object->ulCntData);
    *cookie = entry->cookie = InterlockedIncrement(&next_cookie);

    EnterCriticalSection(&registered_classes_cs);
    list_add_tail(&registered_classes, &entry->entry);
    LeaveCriticalSection(&registered_classes_cs);

    return S_OK;
}

static void scm_revoke_class(struct registered_class *_class)
{
    list_remove(&_class->entry);
    free(_class->object);
    free(_class);
}

HRESULT __cdecl irpcss_server_revoke(handle_t h, unsigned int cookie)
{
    struct registered_class *cur;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY(cur, &registered_classes, struct registered_class, entry)
    {
        if (cur->cookie == cookie)
        {
            scm_revoke_class(cur);
            break;
        }
    }

    LeaveCriticalSection(&registered_classes_cs);

    return S_OK;
}

HRESULT __cdecl irpcss_get_class_object(handle_t h, const GUID *clsid,
        PMInterfacePointer *object)
{
    struct registered_class *cur;

    *object = NULL;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY(cur, &registered_classes, struct registered_class, entry)
    {
        if (!memcmp(clsid, &cur->clsid, sizeof(*clsid)))
        {
            *object = MIDL_user_allocate(FIELD_OFFSET(MInterfacePointer, abData[cur->object->ulCntData]));
            if (*object)
            {
                (*object)->ulCntData = cur->object->ulCntData;
                memcpy((*object)->abData, cur->object->abData, cur->object->ulCntData);
            }

            if (cur->single_use)
                scm_revoke_class(cur);

            break;
        }
    }

    LeaveCriticalSection(&registered_classes_cs);

    return *object ? S_OK : E_NOINTERFACE;
}

HRESULT __cdecl irpcss_get_thread_seq_id(handle_t h, DWORD *id)
{
    static LONG thread_seq_id;
    *id = InterlockedIncrement(&thread_seq_id);
    return S_OK;
}

static RPC_STATUS RPCSS_Initialize(void)
{
    static unsigned short irot_protseq[] = IROT_PROTSEQ;
    static unsigned short irot_endpoint[] = IROT_ENDPOINT;
    static unsigned short epm_protseq[] = L"ncacn_np";
    static unsigned short epm_endpoint[] = L"\\pipe\\epmapper";
    static unsigned short epm_protseq_lrpc[] = L"ncalrpc";
    static unsigned short epm_endpoint_lrpc[] = L"epmapper";
    static unsigned short irpcss_protseq[] = IRPCSS_PROTSEQ;
    static unsigned short irpcss_endpoint[] = IRPCSS_ENDPOINT;
    static const struct protseq_map
    {
        unsigned short *protseq;
        unsigned short *endpoint;
    } protseqs[] =
    {
        { epm_protseq, epm_endpoint },
        { epm_protseq_lrpc, epm_endpoint_lrpc },
        { irot_protseq, irot_endpoint },
        { irpcss_protseq, irpcss_endpoint },
    };
    RPC_IF_HANDLE ifspecs[] =
    {
        epm_v3_0_s_ifspec,
        Irot_v0_2_s_ifspec,
        Irpcss_v0_0_s_ifspec,
    };
    RPC_STATUS status;
    int i, j;

    WINE_TRACE("\n");

    for (i = 0, j = 0; i < ARRAY_SIZE(ifspecs); ++i, j = i)
    {
        status = RpcServerRegisterIf(ifspecs[i], NULL, NULL);
        if (status != RPC_S_OK)
            goto fail;
    }

    for (i = 0; i < ARRAY_SIZE(protseqs); ++i)
    {
        status = RpcServerUseProtseqEpW(protseqs[i].protseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                protseqs[i].endpoint, NULL);
        if (status != RPC_S_OK)
            goto fail;
    }

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (status != RPC_S_OK)
        goto fail;

    return RPC_S_OK;

fail:
    for (i = 0; i < j; ++i)
        RpcServerUnregisterIf(ifspecs[i], NULL, FALSE);

    return status;
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
#ifdef __REACTOS__
    status.dwControlsAccepted        = 0;
#else
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
#endif
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch (ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        TRACE( "shutting down\n" );
        RpcMgmtStopServerListening( NULL );
        RpcServerUnregisterIf( epm_v3_0_s_ifspec, NULL, TRUE );
        RpcServerUnregisterIf( Irot_v0_2_s_ifspec, NULL, TRUE );
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( exit_event );
        return NO_ERROR;
    default:
        FIXME( "got service ctrl %lx\n", ctrl );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

#ifdef __REACTOS__
extern VOID DoRpcSsSetupConfiguration(VOID);
#endif

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    SERVICE_STATUS status;
    RPC_STATUS ret;

    TRACE( "starting service\n" );

    if ((ret = RPCSS_Initialize()))
    {
        WARN("Failed to initialize rpc interfaces, status %ld.\n", ret);
        return;
    }

    exit_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( rpcssW, service_handler, NULL );
    if (!service_handle) return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
#ifdef __REACTOS__
    status.dwControlsAccepted        = 0;
#else
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
#endif
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
#ifdef __REACTOS__
    status.dwWaitHint                = 0;
#else
    status.dwWaitHint                = 10000;
#endif
    SetServiceStatus( service_handle, &status );

#ifdef __REACTOS__
    DoRpcSsSetupConfiguration();
#endif

    WaitForSingleObject( exit_event, INFINITE );

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    TRACE( "service stopped\n" );
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    static const SERVICE_TABLE_ENTRYW service_table[] =
    {
        { rpcssW, ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcherW( service_table );
    return 0;
}
