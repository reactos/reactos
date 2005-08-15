/*
 *	RPC Manager
 *
 * Copyright 2001  Ove Kåven, TransGaming Technologies
 * Copyright 2002  Marcus Meissner
 * Copyright 2005  Mike Hearn, Rob Shearman for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winsvc.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wtypes.h"
#include "wine/unicode.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static void __RPC_STUB dispatch_rpc(RPC_MESSAGE *msg);

/* we only use one function to dispatch calls for all methods - we use the
 * RPC_IF_OLE flag to tell the RPC runtime that this is the case */
static RPC_DISPATCH_FUNCTION rpc_dispatch_table[1] = { dispatch_rpc }; /* (RO) */
static RPC_DISPATCH_TABLE rpc_dispatch = { 1, rpc_dispatch_table }; /* (RO) */

static struct list registered_interfaces = LIST_INIT(registered_interfaces); /* (CS csRegIf) */
static CRITICAL_SECTION csRegIf;
static CRITICAL_SECTION_DEBUG csRegIf_debug =
{
    0, 0, &csRegIf,
    { &csRegIf_debug.ProcessLocksList, &csRegIf_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": dcom registered server interfaces") }
};
static CRITICAL_SECTION csRegIf = { &csRegIf_debug, -1, 0, 0, 0, 0 };

static WCHAR wszPipeTransport[] = {'n','c','a','c','n','_','n','p',0};


struct registered_if
{
    struct list entry;
    DWORD refs; /* ref count */
    RPC_SERVER_INTERFACE If; /* interface registered with the RPC runtime */
};

/* get the pipe endpoint specified of the specified apartment */
static inline void get_rpc_endpoint(LPWSTR endpoint, const OXID *oxid)
{
    /* FIXME: should get endpoint from rpcss */
    static const WCHAR wszEndpointFormat[] = {'\\','p','i','p','e','\\','O','L','E','_','%','0','8','l','x','%','0','8','l','x',0};
    wsprintfW(endpoint, wszEndpointFormat, (DWORD)(*oxid >> 32),(DWORD)*oxid);
}

typedef struct
{
    const IRpcChannelBufferVtbl *lpVtbl;
    LONG                  refs;
} RpcChannelBuffer;

typedef struct
{
    RpcChannelBuffer       super; /* superclass */

    RPC_BINDING_HANDLE     bind; /* handle to the remote server */
} ClientRpcChannelBuffer;

struct dispatch_params
{
    RPCOLEMESSAGE     *msg; /* message */
    IRpcStubBuffer    *stub; /* stub buffer, if applicable */
    IRpcChannelBuffer *chan; /* server channel buffer, if applicable */
    HANDLE             handle; /* handle that will become signaled when call finishes */
    RPC_STATUS         status; /* status (out) */
};

static HRESULT WINAPI RpcChannelBuffer_QueryInterface(LPRPCCHANNELBUFFER iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown))
    {
        *ppv = (LPVOID)iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI RpcChannelBuffer_AddRef(LPRPCCHANNELBUFFER iface)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI ServerRpcChannelBuffer_Release(LPRPCCHANNELBUFFER iface)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->refs);
    if (ref)
        return ref;

    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static ULONG WINAPI ClientRpcChannelBuffer_Release(LPRPCCHANNELBUFFER iface)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->super.refs);
    if (ref)
        return ref;

    RpcBindingFree(&This->bind);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI ServerRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    status = I_RpcGetBuffer(msg);

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_CLIENT_INTERFACE *cif;
    RPC_STATUS status;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    cif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RPC_CLIENT_INTERFACE));
    if (!cif)
        return E_OUTOFMEMORY;

    cif->Length = sizeof(RPC_CLIENT_INTERFACE);
    /* RPC interface ID = COM interface ID */
    cif->InterfaceId.SyntaxGUID = *riid;
    /* COM objects always have a version of 0.0 */
    cif->InterfaceId.SyntaxVersion.MajorVersion = 0;
    cif->InterfaceId.SyntaxVersion.MinorVersion = 0;
    msg->RpcInterfaceInformation = cif;
    msg->Handle = This->bind;
    
    status = I_RpcGetBuffer(msg);

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

/* this thread runs an outgoing RPC */
static DWORD WINAPI rpc_sendreceive_thread(LPVOID param)
{
    struct dispatch_params *data = (struct dispatch_params *) param;
    
    /* FIXME: trap and rethrow RPC exceptions in app thread */
    data->status = I_RpcSendReceive((RPC_MESSAGE *)data->msg);

    TRACE("completed with status 0x%lx\n", data->status);
    
    return 0;
}

static HRESULT WINAPI RpcChannelBuffer_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    HRESULT hr = S_OK;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    DWORD index;
    struct dispatch_params *params;
    DWORD tid;
    IRpcStubBuffer *stub;
    APARTMENT *apt;
    IPID ipid;

    TRACE("(%p) iMethod=%ld\n", olemsg, olemsg->iMethod);

    params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*params));
    if (!params) return E_OUTOFMEMORY;
    
    params->msg = olemsg;
    params->status = RPC_S_OK;

    /* Note: this is an optimization in the Microsoft OLE runtime that we need
     * to copy, as shown by the test_no_couninitialize_client test. without
     * short-circuiting the RPC runtime in the case below, the test will
     * deadlock on the loader lock due to the RPC runtime needing to create
     * a thread to process the RPC when this function is called indirectly
     * from DllMain */

    RpcBindingInqObject(msg->Handle, &ipid);
    stub = ipid_to_apt_and_stubbuffer(&ipid, &apt);
    if (apt && (apt->model & COINIT_APARTMENTTHREADED))
    {
        params->stub = stub;
        params->chan = NULL; /* FIXME: pass server channel */
        params->handle = CreateEventW(NULL, FALSE, FALSE, NULL);

        TRACE("Calling apartment thread 0x%08lx...\n", apt->tid);

        PostMessageW(apt->win, DM_EXECUTERPC, 0, (LPARAM)params);
    }
    else
    {
        if (stub) IRpcStubBuffer_Release(stub);

        /* we use a separate thread here because we need to be able to
         * pump the message loop in the application thread: if we do not,
         * any windows created by this thread will hang and RPCs that try
         * and re-enter this STA from an incoming server thread will
         * deadlock. InstallShield is an example of that.
         */
        params->handle = CreateThread(NULL, 0, rpc_sendreceive_thread, params, 0, &tid);
        if (!params->handle)
        {
            ERR("Could not create RpcSendReceive thread, error %lx\n", GetLastError());
            hr = E_UNEXPECTED;
        }
    }
    if (apt) apartment_release(apt);

    if (hr == S_OK)
        hr = CoWaitForMultipleHandles(0, INFINITE, 1, &params->handle, &index);
    CloseHandle(params->handle);

    status = params->status;
    HeapFree(GetProcessHeap(), 0, params);
    params = NULL;

    if (hr) return hr;
    
    if (pstatus) *pstatus = status;

    TRACE("RPC call status: 0x%lx\n", status);
    if (status == RPC_S_OK)
        hr = S_OK;
    else if (status == RPC_S_CALL_FAILED)
        hr = *(HRESULT *)olemsg->Buffer;
    else
        hr = HRESULT_FROM_WIN32(status);

    TRACE("-- 0x%08lx\n", hr);

    return hr;
}

static HRESULT WINAPI ServerRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;

    TRACE("(%p)\n", msg);

    status = I_RpcFreeBuffer(msg);

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;

    TRACE("(%p)\n", msg);

    status = I_RpcFreeBuffer(msg);

    HeapFree(GetProcessHeap(), 0, msg->RpcInterfaceInformation);
    msg->RpcInterfaceInformation = NULL;

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI RpcChannelBuffer_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    FIXME("(%p,%p), stub!\n", pdwDestContext, ppvDestContext);
    return E_FAIL;
}

static HRESULT WINAPI RpcChannelBuffer_IsConnected(LPRPCCHANNELBUFFER iface)
{
    TRACE("()\n");
    /* native does nothing too */
    return S_OK;
}

static const IRpcChannelBufferVtbl ClientRpcChannelBufferVtbl =
{
    RpcChannelBuffer_QueryInterface,
    RpcChannelBuffer_AddRef,
    ClientRpcChannelBuffer_Release,
    ClientRpcChannelBuffer_GetBuffer,
    RpcChannelBuffer_SendReceive,
    ClientRpcChannelBuffer_FreeBuffer,
    RpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

static const IRpcChannelBufferVtbl ServerRpcChannelBufferVtbl =
{
    RpcChannelBuffer_QueryInterface,
    RpcChannelBuffer_AddRef,
    ServerRpcChannelBuffer_Release,
    ServerRpcChannelBuffer_GetBuffer,
    RpcChannelBuffer_SendReceive,
    ServerRpcChannelBuffer_FreeBuffer,
    RpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

/* returns a channel buffer for proxies */
HRESULT RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid, IRpcChannelBuffer **chan)
{
    ClientRpcChannelBuffer *This;
    WCHAR                   endpoint[200];
    RPC_BINDING_HANDLE      bind;
    RPC_STATUS              status;
    LPWSTR                  string_binding;

    /* connect to the apartment listener thread */
    get_rpc_endpoint(endpoint, oxid);

    TRACE("proxy pipe: connecting to endpoint: %s\n", debugstr_w(endpoint));

    status = RpcStringBindingComposeW(
        NULL,
        wszPipeTransport,
        NULL,
        endpoint,
        NULL,
        &string_binding);
        
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(string_binding, &bind);

        if (status == RPC_S_OK)
        {
            IPID ipid2 = *ipid; /* why can't RpcBindingSetObject take a const? */
            status = RpcBindingSetObject(bind, &ipid2);
            if (status != RPC_S_OK)
                RpcBindingFree(&bind);
        }

        RpcStringFreeW(&string_binding);
    }

    if (status != RPC_S_OK)
    {
        ERR("Couldn't get binding for endpoint %s, status = %ld\n", debugstr_w(endpoint), status);
        return HRESULT_FROM_WIN32(status);
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        RpcBindingFree(&bind);
        return E_OUTOFMEMORY;
    }

    This->super.lpVtbl = &ClientRpcChannelBufferVtbl;
    This->super.refs = 1;
    This->bind = bind;

    *chan = (IRpcChannelBuffer*)This;

    return S_OK;
}

HRESULT RPC_CreateServerChannel(IRpcChannelBuffer **chan)
{
    RpcChannelBuffer *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &ServerRpcChannelBufferVtbl;
    This->refs = 1;
    
    *chan = (IRpcChannelBuffer*)This;

    return S_OK;
}


HRESULT RPC_ExecuteCall(struct dispatch_params *params)
{
    HRESULT hr = IRpcStubBuffer_Invoke(params->stub, params->msg, params->chan);
    IRpcStubBuffer_Release(params->stub);
    if (params->handle) SetEvent(params->handle);
    return hr;
}

static void __RPC_STUB dispatch_rpc(RPC_MESSAGE *msg)
{
    struct dispatch_params *params;
    IRpcStubBuffer     *stub;
    APARTMENT          *apt;
    IPID                ipid;

    RpcBindingInqObject(msg->Handle, &ipid);

    TRACE("ipid = %s, iMethod = %d\n", debugstr_guid(&ipid), msg->ProcNum);

    params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*params));
    if (!params) return RpcRaiseException(E_OUTOFMEMORY);

    stub = ipid_to_apt_and_stubbuffer(&ipid, &apt);
    if (!apt || !stub)
    {
        if (apt) apartment_release(apt);
        ERR("no apartment found for ipid %s\n", debugstr_guid(&ipid));
        return RpcRaiseException(RPC_E_DISCONNECTED);
    }

    params->msg = (RPCOLEMESSAGE *)msg;
    params->stub = stub;
    params->chan = NULL; /* FIXME: pass server channel */
    params->status = RPC_S_OK;

    /* Note: this is the important difference between STAs and MTAs - we
     * always execute RPCs to STAs in the thread that originally created the
     * apartment (i.e. the one that pumps messages to the window) */
    if (apt->model & COINIT_APARTMENTTHREADED)
    {
        params->handle = CreateEventW(NULL, FALSE, FALSE, NULL);

        TRACE("Calling apartment thread 0x%08lx...\n", apt->tid);

        PostMessageW(apt->win, DM_EXECUTERPC, 0, (LPARAM)params);
        WaitForSingleObject(params->handle, INFINITE);
        CloseHandle(params->handle);
    }
    else
        RPC_ExecuteCall(params);

    HeapFree(GetProcessHeap(), 0, params);

    apartment_release(apt);
}

/* stub registration */
HRESULT RPC_RegisterInterface(REFIID riid)
{
    struct registered_if *rif;
    BOOL found = FALSE;
    HRESULT hr = S_OK;
    
    TRACE("(%s)\n", debugstr_guid(riid));

    EnterCriticalSection(&csRegIf);
    LIST_FOR_EACH_ENTRY(rif, &registered_interfaces, struct registered_if, entry)
    {
        if (IsEqualGUID(&rif->If.InterfaceId.SyntaxGUID, riid))
        {
            rif->refs++;
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        TRACE("Creating new interface\n");

        rif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*rif));
        if (rif)
        {
            RPC_STATUS status;

            rif->refs = 1;
            rif->If.Length = sizeof(RPC_SERVER_INTERFACE);
            /* RPC interface ID = COM interface ID */
            rif->If.InterfaceId.SyntaxGUID = *riid;
            rif->If.DispatchTable = &rpc_dispatch;
            /* all other fields are 0, including the version asCOM objects
             * always have a version of 0.0 */
            status = RpcServerRegisterIfEx(
                (RPC_IF_HANDLE)&rif->If,
                NULL, NULL,
                RPC_IF_OLE | RPC_IF_AUTOLISTEN,
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                NULL);
            if (status == RPC_S_OK)
                list_add_tail(&registered_interfaces, &rif->entry);
            else
            {
                ERR("RpcServerRegisterIfEx failed with error %ld\n", status);
                HeapFree(GetProcessHeap(), 0, rif);
                hr = HRESULT_FROM_WIN32(status);
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&csRegIf);
    return hr;
}

/* stub unregistration */
void RPC_UnregisterInterface(REFIID riid)
{
    struct registered_if *rif;
    EnterCriticalSection(&csRegIf);
    LIST_FOR_EACH_ENTRY(rif, &registered_interfaces, struct registered_if, entry)
    {
        if (IsEqualGUID(&rif->If.InterfaceId.SyntaxGUID, riid))
        {
            if (!--rif->refs)
            {
#if 0 /* this is a stub in builtin and spams the console with FIXME's */
                IID iid = *riid; /* RpcServerUnregisterIf doesn't take const IID */
                RpcServerUnregisterIf((RPC_IF_HANDLE)&rif->If, &iid, 0);
                list_remove(&rif->entry);
                HeapFree(GetProcessHeap(), 0, rif);
#endif
            }
            break;
        }
    }
    LeaveCriticalSection(&csRegIf);
}

/* make the apartment reachable by other threads and processes and create the
 * IRemUnknown object */
void RPC_StartRemoting(struct apartment *apt)
{
    if (!InterlockedExchange(&apt->remoting_started, TRUE))
    {
        WCHAR endpoint[200];
        RPC_STATUS status;

        get_rpc_endpoint(endpoint, &apt->oxid);
    
        status = RpcServerUseProtseqEpW(
            wszPipeTransport,
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            endpoint,
            NULL);
        if (status != RPC_S_OK)
            ERR("Couldn't register endpoint %s\n", debugstr_w(endpoint));

        /* FIXME: move remote unknown exporting into this function */
    }
    start_apartment_remote_unknown();
}


static HRESULT create_server(REFCLSID rclsid)
{
    static const WCHAR  wszLocalServer32[] = { 'L','o','c','a','l','S','e','r','v','e','r','3','2',0 };
    static const WCHAR  embedding[] = { ' ', '-','E','m','b','e','d','d','i','n','g',0 };
    HKEY                hkeyclsid;
    HKEY                key;
    HRESULT             hres = E_UNEXPECTED;
    WCHAR               exe[MAX_PATH+1];
    DWORD               exelen = sizeof(exe);
    WCHAR               command[MAX_PATH+sizeof(embedding)/sizeof(WCHAR)];
    STARTUPINFOW        sinfo;
    PROCESS_INFORMATION pinfo;

    hres = HRESULT_FROM_WIN32(COM_OpenKeyForCLSID(rclsid, KEY_READ, &hkeyclsid));
    if (hres != S_OK) {
        ERR("class %s not registered\n", debugstr_guid(rclsid));
        return REGDB_E_READREGDB;
    }

    hres = RegOpenKeyExW(hkeyclsid, wszLocalServer32, 0, KEY_READ, &key);

    if (hres != ERROR_SUCCESS) {
        WARN("class %s not registered as LocalServer32\n", debugstr_guid(rclsid));
        return REGDB_E_READREGDB; /* Probably */
    }

    memset(exe,0,sizeof(exe));
    hres= RegQueryValueExW(key, NULL, NULL, NULL, (LPBYTE)exe, &exelen);
    RegCloseKey(key);
    if (hres) {
        WARN("No default value for LocalServer32 key\n");
        return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }

    memset(&sinfo,0,sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    /* EXE servers are started with the -Embedding switch. MSDN also claims /Embedding is used,
     * 9x does -Embedding, perhaps an 9x/NT difference?
     */

    strcpyW(command, exe);
    strcatW(command, embedding);

    TRACE("activating local server %s for %s\n", debugstr_w(command), debugstr_guid(rclsid));

    if (!CreateProcessW(exe, command, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo, &pinfo)) {
        WARN("failed to run local server %s\n", debugstr_w(exe));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    return S_OK;
}

/*
 * start_local_service()  - start a service given its name and parameters
 */
static DWORD start_local_service(LPCWSTR name, DWORD num, LPWSTR *params)
{
    SC_HANDLE handle, hsvc;
    DWORD     r = ERROR_FUNCTION_FAILED;

    TRACE("Starting service %s %ld params\n", debugstr_w(name), num);

    handle = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!handle)
        return r;
    hsvc = OpenServiceW(handle, name, SC_MANAGER_ALL_ACCESS);
    if (hsvc)
    {
        if(StartServiceW(hsvc, num, (LPCWSTR*)params))
            r = ERROR_SUCCESS;
        else
            r = GetLastError();
        if (r == ERROR_SERVICE_ALREADY_RUNNING)
            r = ERROR_SUCCESS;
        CloseServiceHandle(hsvc);
    }
    CloseServiceHandle(handle);

    TRACE("StartService returned error %ld (%s)\n", r, r?"ok":"failed");

    return r;
}

/*
 * create_local_service()  - start a COM server in a service
 *
 *   To start a Local Service, we read the AppID value under
 * the class's CLSID key, then open the HKCR\\AppId key specified
 * there and check for a LocalService value.
 *
 * Note:  Local Services are not supported under Windows 9x
 */
static HRESULT create_local_service(REFCLSID rclsid)
{
    HRESULT hres = REGDB_E_READREGDB;
    WCHAR buf[CHARS_IN_GUID], keyname[50];
    static const WCHAR szAppId[] = { 'A','p','p','I','d',0 };
    static const WCHAR szAppIdKey[] = { 'A','p','p','I','d','\\',0 };
    static const WCHAR szLocalService[] = { 'L','o','c','a','l','S','e','r','v','i','c','e',0 };
    static const WCHAR szServiceParams[] = {'S','e','r','v','i','c','e','P','a','r','a','m','s',0};
    HKEY hkey;
    LONG r;
    DWORD type, sz;

    TRACE("Attempting to start Local service for %s\n", debugstr_guid(rclsid));

    /* read the AppID value under the class's key */
    r = COM_OpenKeyForCLSID(rclsid, KEY_READ, &hkey);
    if (r!=ERROR_SUCCESS)
        return hres;
    sz = sizeof buf;
    r = RegQueryValueExW(hkey, szAppId, NULL, &type, (LPBYTE)buf, &sz);
    RegCloseKey(hkey);
    if (r!=ERROR_SUCCESS || type!=REG_SZ)
        return hres;

    /* read the LocalService and ServiceParameters values from the AppID key */
    strcpyW(keyname, szAppIdKey);
    strcatW(keyname, buf);
    r = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &hkey);
    if (r!=ERROR_SUCCESS)
        return hres;
    sz = sizeof buf;
    r = RegQueryValueExW(hkey, szLocalService, NULL, &type, (LPBYTE)buf, &sz);
    if (r==ERROR_SUCCESS && type==REG_SZ)
    {
        DWORD num_args = 0;
        LPWSTR args[1] = { NULL };

        /*
         * FIXME: I'm not really sure how to deal with the service parameters.
         *        I suspect that the string returned from RegQueryValueExW
         *        should be split into a number of arguments by spaces.
         *        It would make more sense if ServiceParams contained a
         *        REG_MULTI_SZ here, but it's a REG_SZ for the services
         *        that I'm interested in for the moment.
         */
        r = RegQueryValueExW(hkey, szServiceParams, NULL, &type, NULL, &sz);
        if (r == ERROR_SUCCESS && type == REG_SZ && sz)
        {
            args[0] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz);
            num_args++;
            RegQueryValueExW(hkey, szServiceParams, NULL, &type, (LPBYTE)args[0], &sz);
        }
        r = start_local_service(buf, num_args, args);
        if (r==ERROR_SUCCESS)
            hres = S_OK;
        HeapFree(GetProcessHeap(),0,args[0]);
    }
    RegCloseKey(hkey);
        
    return hres;
}


static void get_localserver_pipe_name(WCHAR *pipefn, REFCLSID rclsid)
{
    static const WCHAR wszPipeRef[] = {'\\','\\','.','\\','p','i','p','e','\\',0};
    strcpyW(pipefn, wszPipeRef);
    StringFromGUID2(rclsid, pipefn + sizeof(wszPipeRef)/sizeof(wszPipeRef[0]) - 1, CHARS_IN_GUID);
}

/* FIXME: should call to rpcss instead */
HRESULT RPC_GetLocalClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    HRESULT        hres;
    HANDLE         hPipe;
    WCHAR          pipefn[100];
    DWORD          res, bufferlen;
    char           marshalbuffer[200];
    IStream       *pStm;
    LARGE_INTEGER  seekto;
    ULARGE_INTEGER newpos;
    int            tries = 0;

    static const int MAXTRIES = 30; /* 30 seconds */

    TRACE("rclsid=%s, iid=%s\n", debugstr_guid(rclsid), debugstr_guid(iid));

    get_localserver_pipe_name(pipefn, rclsid);

    while (tries++ < MAXTRIES) {
        TRACE("waiting for %s\n", debugstr_w(pipefn));
      
        WaitNamedPipeW( pipefn, NMPWAIT_WAIT_FOREVER );
        hPipe = CreateFileW(pipefn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE) {
            if (tries == 1) {
                if ( (hres = create_server(rclsid)) &&
                     (hres = create_local_service(rclsid)) )
                    return hres;
                Sleep(1000);
            } else {
                WARN("Connecting to %s, no response yet, retrying: le is %lx\n", debugstr_w(pipefn), GetLastError());
                Sleep(1000);
            }
            continue;
        }
        bufferlen = 0;
        if (!ReadFile(hPipe,marshalbuffer,sizeof(marshalbuffer),&bufferlen,NULL)) {
            FIXME("Failed to read marshal id from classfactory of %s.\n",debugstr_guid(rclsid));
            Sleep(1000);
            continue;
        }
        TRACE("read marshal id from pipe\n");
        CloseHandle(hPipe);
        break;
    }
    
    if (tries >= MAXTRIES)
        return E_NOINTERFACE;
    
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) return hres;
    hres = IStream_Write(pStm,marshalbuffer,bufferlen,&res);
    if (hres) goto out;
    seekto.u.LowPart = 0;seekto.u.HighPart = 0;
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    
    TRACE("unmarshalling classfactory\n");
    hres = CoUnmarshalInterface(pStm,&IID_IClassFactory,ppv);
out:
    IStream_Release(pStm);
    return hres;
}


struct local_server_params
{
    CLSID clsid;
    IStream *stream;
};

/* FIXME: should call to rpcss instead */
static DWORD WINAPI local_server_thread(LPVOID param)
{
    struct local_server_params * lsp = (struct local_server_params *)param;
    HANDLE		hPipe;
    WCHAR 		pipefn[100];
    HRESULT		hres;
    IStream		*pStm = lsp->stream;
    STATSTG		ststg;
    unsigned char	*buffer;
    int 		buflen;
    LARGE_INTEGER	seekto;
    ULARGE_INTEGER	newpos;
    ULONG		res;

    TRACE("Starting threader for %s.\n",debugstr_guid(&lsp->clsid));

    get_localserver_pipe_name(pipefn, &lsp->clsid);

    HeapFree(GetProcessHeap(), 0, lsp);

    hPipe = CreateNamedPipeW( pipefn, PIPE_ACCESS_DUPLEX,
                              PIPE_TYPE_BYTE|PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                              4096, 4096, 500 /* 0.5 second timeout */, NULL );
    
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        FIXME("pipe creation failed for %s, le is %ld\n", debugstr_w(pipefn), GetLastError());
        return 1;
    }
    
    while (1) {
        if (!ConnectNamedPipe(hPipe,NULL)) {
            ERR("Failure during ConnectNamedPipe %ld, ABORT!\n",GetLastError());
            break;
        }

        TRACE("marshalling IClassFactory to client\n");
        
        hres = IStream_Stat(pStm,&ststg,0);
        if (hres) return hres;

        buflen = ststg.cbSize.u.LowPart;
        buffer = HeapAlloc(GetProcessHeap(),0,buflen);
        seekto.u.LowPart = 0;
        seekto.u.HighPart = 0;
        hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
        if (hres) {
            FIXME("IStream_Seek failed, %lx\n",hres);
            return hres;
        }
        
        hres = IStream_Read(pStm,buffer,buflen,&res);
        if (hres) {
            FIXME("Stream Read failed, %lx\n",hres);
            return hres;
        }
        
        WriteFile(hPipe,buffer,buflen,&res,NULL);
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);

        TRACE("done marshalling IClassFactory\n");
    }
    CloseHandle(hPipe);
    IStream_Release(pStm);
    return 0;
}

void RPC_StartLocalServer(REFCLSID clsid, IStream *stream)
{
    DWORD tid;
    HANDLE thread;
    struct local_server_params *lsp = HeapAlloc(GetProcessHeap(), 0, sizeof(*lsp));

    lsp->clsid = *clsid;
    lsp->stream = stream;

    thread = CreateThread(NULL, 0, local_server_thread, lsp, 0, &tid);
    CloseHandle(thread);
    /* FIXME: failure handling */
}
