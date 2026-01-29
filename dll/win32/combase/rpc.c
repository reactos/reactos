/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "servprov.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "combase_private.h"

#include "irpcss.h"

#ifdef __REACTOS__
#include <wine/irot.h>
#endif

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
      0, 0, { (DWORD_PTR)(__FILE__ ": dcom registered server interfaces") }
};
static CRITICAL_SECTION csRegIf = { &csRegIf_debug, -1, 0, 0, 0, 0 };

static struct list channel_hooks = LIST_INIT(channel_hooks); /* (CS csChannelHook) */
static CRITICAL_SECTION csChannelHook;
static CRITICAL_SECTION_DEBUG csChannelHook_debug =
{
    0, 0, &csChannelHook,
    { &csChannelHook_debug.ProcessLocksList, &csChannelHook_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": channel hooks") }
};
static CRITICAL_SECTION csChannelHook = { &csChannelHook_debug, -1, 0, 0, 0, 0 };

static WCHAR rpctransportW[] = L"ncalrpc";

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
    wsprintfW(endpoint, L"\\pipe\\OLE_%016I64x", *oxid);
}

typedef struct
{
    IRpcChannelBuffer IRpcChannelBuffer_iface;
    LONG refs;

    DWORD dest_context; /* returned from GetDestCtx */
    void *dest_context_data; /* returned from GetDestCtx */
} RpcChannelBuffer;

typedef struct
{
    RpcChannelBuffer       super; /* superclass */

    RPC_BINDING_HANDLE     bind; /* handle to the remote server */
    OXID                   oxid; /* apartment in which the channel is valid */
    DWORD                  server_pid; /* id of server process */
    HANDLE                 event; /* cached event handle */
    IID                    iid; /* IID of the proxy this belongs to */
} ClientRpcChannelBuffer;

struct dispatch_params
{
    RPCOLEMESSAGE     *msg; /* message */
    IRpcStubBuffer    *stub; /* stub buffer, if applicable */
    IRpcChannelBuffer *chan; /* server channel buffer, if applicable */
    IID                iid; /* ID of interface being called */
    IUnknown          *iface; /* interface being called */
    HANDLE             handle; /* handle that will become signaled when call finishes */
    BOOL               bypass_rpcrt; /* bypass RPC runtime? */
    RPC_STATUS         status; /* status (out) */
    HRESULT            hr; /* hresult (out) */
};

struct message_state
{
    RPC_BINDING_HANDLE binding_handle;
    ULONG prefix_data_len;
    SChannelHookCallInfo channel_hook_info;
    BOOL bypass_rpcrt;

    /* client only */
    HWND target_hwnd;
    DWORD target_tid;
    struct dispatch_params params;
};

typedef struct
{
    ULONG conformance; /* NDR */
    GUID id;
    ULONG size;
    /* [size_is((size+7)&~7)] */ unsigned char data[1];
} WIRE_ORPC_EXTENT;

typedef struct
{
    ULONG size;
    ULONG reserved;
    unsigned char extent[1];
} WIRE_ORPC_EXTENT_ARRAY;

typedef struct
{
    ULONG version;
    ULONG flags;
    ULONG reserved1;
    GUID  cid;
    unsigned char extensions[1];
} WIRE_ORPCTHIS;

typedef struct
{
    ULONG flags;
    unsigned char extensions[1];
} WIRE_ORPCTHAT;

struct channel_hook_entry
{
    struct list entry;
    GUID id;
    IChannelHook *hook;
};

struct channel_hook_buffer_data
{
    GUID id;
    ULONG extension_size;
};
void * __RPC_USER MIDL_user_allocate(SIZE_T size)
{
    return malloc(size);
}

void __RPC_USER MIDL_user_free(void *p)
{
    free(p);
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

static BOOL start_rpcss(void)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW(NULL, NULL, 0)))
    {
        ERR("Failed to open service manager\n");
        return FALSE;
    }

    if (!(service = OpenServiceW(scm, L"RpcSs", SERVICE_START | SERVICE_QUERY_STATUS)))
    {
        ERR("Failed to open RpcSs service\n");
        CloseServiceHandle( scm );
        return FALSE;
    }

    if (StartServiceW(service, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount64();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(status), &dummy))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount64() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN("RpcSs failed to start %lu\n", status.dwCurrentState);
    }
    else
        ERR("Failed to start RpcSs service\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ret;
}

static RPC_BINDING_HANDLE get_rpc_handle(unsigned short *protseq, unsigned short *endpoint)
{
    RPC_BINDING_HANDLE handle = NULL;
    RPC_STATUS status;
    RPC_WSTR binding;

    status = RpcStringBindingComposeW(NULL, protseq, NULL, endpoint, NULL, &binding);
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(binding, &handle);
        RpcStringFreeW(&binding);
    }

    return handle;
}

static RPC_BINDING_HANDLE get_irpcss_handle(void)
{
    static RPC_BINDING_HANDLE irpcss_handle;

    if (!irpcss_handle)
    {
        unsigned short protseq[] = IRPCSS_PROTSEQ;
        unsigned short endpoint[] = IRPCSS_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irpcss_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irpcss_handle;
}

static RPC_BINDING_HANDLE get_irot_handle(void)
{
    static RPC_BINDING_HANDLE irot_handle;

    if (!irot_handle)
    {
        unsigned short protseq[] = IROT_PROTSEQ;
        unsigned short endpoint[] = IROT_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irot_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irot_handle;
}

#define RPCSS_CALL_START \
    HRESULT hr; \
    for (;;) { \
        __TRY {

#define RPCSS_CALL_END \
        } __EXCEPT(rpc_filter) { \
            hr = HRESULT_FROM_WIN32(GetExceptionCode()); \
        } \
        __ENDTRY \
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)) { \
            if (start_rpcss()) \
                continue; \
        } \
        break; \
    } \
    return hr;

HRESULT rpcss_get_next_seqid(DWORD *id)
{
    RPCSS_CALL_START
    hr = irpcss_get_thread_seq_id(get_irpcss_handle(), id);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotRegister(const MonikerComparisonData *moniker_data,
        const InterfaceData *object, const InterfaceData *moniker,
        const FILETIME *time, DWORD flags, IrotCookie *cookie, IrotContextHandle *ctxt_handle)
{
    RPCSS_CALL_START
    hr = IrotRegister(get_irot_handle(), moniker_data, object, moniker, time, flags, cookie, ctxt_handle);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotIsRunning(const MonikerComparisonData *moniker_data)
{
    RPCSS_CALL_START
    hr = IrotIsRunning(get_irot_handle(), moniker_data);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotGetObject(const MonikerComparisonData *moniker_data, PInterfaceData *obj,
        IrotCookie *cookie)
{
    RPCSS_CALL_START
    hr = IrotGetObject(get_irot_handle(), moniker_data, obj, cookie);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotNoteChangeTime(IrotCookie cookie, const FILETIME *time)
{
    RPCSS_CALL_START
    hr = IrotNoteChangeTime(get_irot_handle(), cookie, time);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotGetTimeOfLastChange(const MonikerComparisonData *moniker_data, FILETIME *time)
{
    RPCSS_CALL_START
    hr = IrotGetTimeOfLastChange(get_irot_handle(), moniker_data, time);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotEnumRunning(PInterfaceList *list)
{
    RPCSS_CALL_START
    hr = IrotEnumRunning(get_irot_handle(), list);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotRevoke(IrotCookie cookie, IrotContextHandle *ctxt_handle, PInterfaceData *object,
        PInterfaceData *moniker)
{
    RPCSS_CALL_START
    hr = IrotRevoke(get_irot_handle(), cookie, ctxt_handle, object, moniker);
    RPCSS_CALL_END
}

static HRESULT rpcss_server_register(REFCLSID clsid, DWORD flags, MInterfacePointer *obj, unsigned int *cookie)
{
    RPCSS_CALL_START
    hr = irpcss_server_register(get_irpcss_handle(), clsid, flags, obj, cookie);
    RPCSS_CALL_END
}

HRESULT rpc_revoke_local_server(unsigned int cookie)
{
    RPCSS_CALL_START
    hr = irpcss_server_revoke(get_irpcss_handle(), cookie);
    RPCSS_CALL_END
}

static HRESULT rpcss_get_class_object(REFCLSID rclsid, PMInterfacePointer *objref)
{
    RPCSS_CALL_START
    hr = irpcss_get_class_object(get_irpcss_handle(), rclsid, objref);
    RPCSS_CALL_END
}

static DWORD start_local_service(const WCHAR *name, DWORD num, LPCWSTR *params)
{
    SC_HANDLE handle, hsvc;
    DWORD r = ERROR_FUNCTION_FAILED;

    TRACE("Starting service %s %ld params\n", debugstr_w(name), num);

    handle = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!handle)
        return r;
    hsvc = OpenServiceW(handle, name, SERVICE_START);
    if (hsvc)
    {
        if(StartServiceW(hsvc, num, params))
            r = ERROR_SUCCESS;
        else
            r = GetLastError();
        if (r == ERROR_SERVICE_ALREADY_RUNNING)
            r = ERROR_SUCCESS;
        CloseServiceHandle(hsvc);
    }
    else
        r = GetLastError();
    CloseServiceHandle(handle);

    TRACE("StartService returned error %lu (%s)\n", r, (r == ERROR_SUCCESS) ? "ok":"failed");

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
    HRESULT hr;
    WCHAR buf[CHARS_IN_GUID];
    HKEY hkey;
    LONG r;
    DWORD type, sz;

    TRACE("Attempting to start Local service for %s\n", debugstr_guid(rclsid));

    hr = open_appidkey_from_clsid(rclsid, KEY_READ, &hkey);
    if (FAILED(hr))
        return hr;

    /* read the LocalService and ServiceParameters values from the AppID key */
    sz = sizeof buf;
    r = RegQueryValueExW(hkey, L"LocalService", NULL, &type, (LPBYTE)buf, &sz);
    if (r == ERROR_SUCCESS && type == REG_SZ)
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
        r = RegQueryValueExW(hkey, L"ServiceParams", NULL, &type, NULL, &sz);
        if (r == ERROR_SUCCESS && type == REG_SZ && sz)
        {
            args[0] = calloc(1, sz);
            num_args++;
            RegQueryValueExW(hkey, L"ServiceParams", NULL, &type, (LPBYTE)args[0], &sz);
        }
        r = start_local_service(buf, num_args, (LPCWSTR *)args);
        if (r != ERROR_SUCCESS)
            hr = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
        free(args[0]);
    }
    else
    {
        WARN("No LocalService value\n");
        hr = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }
    RegCloseKey(hkey);

    return hr;
}

static HRESULT create_server(REFCLSID rclsid, HANDLE *process)
{
    static const WCHAR  embeddingW[] = L" -Embedding";
    HKEY                key;
    int                 arch = (sizeof(void *) > sizeof(int)) ? 64 : 32;
    REGSAM              opposite = (arch == 64) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL                is_wow64 = FALSE, is_opposite = FALSE;
    HRESULT             hr;
    WCHAR               command[MAX_PATH + ARRAY_SIZE(embeddingW)];
    DWORD               size = (MAX_PATH+1) * sizeof(WCHAR);
    STARTUPINFOW        sinfo;
    PROCESS_INFORMATION pinfo;
    LONG ret;

    TRACE("Attempting to start server for %s\n", debugstr_guid(rclsid));

    hr = open_key_for_clsid(rclsid, L"LocalServer32", KEY_READ, &key);
    if (FAILED(hr) && (arch == 64 || (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
    {
        hr = open_key_for_clsid(rclsid, L"LocalServer32", opposite | KEY_READ, &key);
        is_opposite = TRUE;
    }
    if (FAILED(hr))
    {
        ERR("class %s not registered\n", debugstr_guid(rclsid));
        return hr;
    }

    ret = RegQueryValueExW(key, NULL, NULL, NULL, (LPBYTE)command, &size);
    RegCloseKey(key);
    if (ret)
    {
        WARN("No default value for LocalServer32 key\n");
        return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    /* EXE servers are started with the -Embedding switch. */

    lstrcatW(command, embeddingW);

    TRACE("activating local server %s for %s\n", debugstr_w(command), debugstr_guid(rclsid));

    /* FIXME: Win2003 supports a ServerExecutable value that is passed into
     * CreateProcess */
    if (is_opposite)
    {
        void *cookie;
        Wow64DisableWow64FsRedirection(&cookie);
        if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &sinfo, &pinfo))
        {
            WARN("failed to run local server %s\n", debugstr_w(command));
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        Wow64RevertWow64FsRedirection(cookie);
        if (FAILED(hr)) return hr;
    }
    else if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &sinfo, &pinfo))
    {
        WARN("failed to run local server %s\n", debugstr_w(command));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *process = pinfo.hProcess;
    CloseHandle(pinfo.hThread);

    return S_OK;
}

static HRESULT create_surrogate_server(REFCLSID rclsid, HANDLE *process)
{
    static const WCHAR processidW[] = L" /PROCESSID:";
    HKEY key;
    int arch = (sizeof(void *) > sizeof(int)) ? 64 : 32;
    REGSAM opposite = (arch == 64) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL is_wow64 = FALSE, is_opposite = FALSE;
    HRESULT hr;
    WCHAR command[MAX_PATH + ARRAY_SIZE(processidW) + CHARS_IN_GUID];
    DWORD size;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    LONG ret;

    TRACE("Attempting to start surrogate server for %s\n", debugstr_guid(rclsid));

    hr = open_key_for_clsid(rclsid, NULL, KEY_READ, &key);
    if (FAILED(hr) && (arch == 64 || (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
        hr = open_key_for_clsid(rclsid, NULL, opposite | KEY_READ, &key);
    if (FAILED(hr)) return hr;
    RegCloseKey(key);

    hr = open_appidkey_from_clsid(rclsid, KEY_READ, &key);
    if (FAILED(hr) && (arch == 64 || (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
    {
        hr = open_appidkey_from_clsid(rclsid, opposite | KEY_READ, &key);
        if (FAILED(hr)) return hr;
        is_opposite = TRUE;
    }

    size = (MAX_PATH + 1) * sizeof(WCHAR);
    ret = RegQueryValueExW(key, L"DllSurrogate", NULL, NULL, (LPBYTE)command, &size);
    RegCloseKey(key);
    if (ret || !size || !command[0])
    {
        TRACE("No value for DllSurrogate key\n");

        if ((sizeof(void *) == 8 || is_wow64) && opposite == KEY_WOW64_32KEY)
            GetSystemWow64DirectoryW(command, MAX_PATH - ARRAY_SIZE(L"\\dllhost.exe"));
        else
            GetSystemDirectoryW(command, MAX_PATH - ARRAY_SIZE(L"\\dllhost.exe"));

        wcscat(command, L"\\dllhost.exe");
    }

    /* Surrogate EXE servers are started with the /PROCESSID:{GUID} switch. */
    wcscat(command, processidW);
    StringFromGUID2(rclsid, command + wcslen(command), CHARS_IN_GUID);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    TRACE("Activating surrogate local server %s\n", debugstr_w(command));

    if (is_opposite)
    {
        void *cookie;
        Wow64DisableWow64FsRedirection(&cookie);
        if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
        {
            WARN("failed to run surrogate local server %s\n", debugstr_w(command));
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        Wow64RevertWow64FsRedirection(cookie);
    }
    else if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
    {
        WARN("failed to run surrogate local server %s\n", debugstr_w(command));
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr)) return hr;

    *process = pi.hProcess;
    CloseHandle(pi.hThread);

    return S_OK;
}

HRESULT rpc_get_local_class_object(REFCLSID rclsid, REFIID riid, void **obj)
{
    PMInterfacePointer objref = NULL;
    IServiceProvider *local_server;
    IStream *stream = NULL;
    ULARGE_INTEGER newpos;
    LARGE_INTEGER seekto;
    int tries = 0;
    ULONG length;
    HRESULT hr;
    static const int MAXTRIES = 30; /* 30 seconds */

    TRACE("clsid %s, riid %s\n", debugstr_guid(rclsid), debugstr_guid(riid));

    while (tries++ < MAXTRIES)
    {
        DWORD index, start_ticks;
        HANDLE process = 0;

        if (SUCCEEDED(hr = rpcss_get_class_object(rclsid, &objref)))
            break;

        if (tries == 1)
        {
            if ((hr = create_local_service(rclsid)) && (hr = create_server(rclsid, &process)) &&
                (hr = create_surrogate_server(rclsid, &process)) )
                return hr;
        }

        /* Wait for one second, even if messages arrive. */
        start_ticks = GetTickCount();
        do
        {
            if (SUCCEEDED(CoWaitForMultipleHandles(0, 1000, (process != 0), &process, &index)) && process && !index)
            {
                WARN("Server for %s failed to start.\n", debugstr_guid(rclsid));
                CloseHandle(process);
                return E_NOINTERFACE;
            }
        } while (GetTickCount() - start_ticks < 1000);

        if (process) CloseHandle(process);
    }

    if (!objref || tries >= MAXTRIES)
        return E_NOINTERFACE;

    if (SUCCEEDED(hr = CreateStreamOnHGlobal(0, TRUE, &stream)))
        hr = IStream_Write(stream, objref->abData, objref->ulCntData, &length);

    MIDL_user_free(objref);

    if (SUCCEEDED(hr))
    {
        seekto.QuadPart = 0;
        IStream_Seek(stream, seekto, STREAM_SEEK_SET, &newpos);

        TRACE("Unmarshalling local server.\n");
        hr = CoUnmarshalInterface(stream, &IID_IServiceProvider, (void **)&local_server);
        if (SUCCEEDED(hr))
        {
            hr = IServiceProvider_QueryService(local_server, rclsid, riid, obj);
            IServiceProvider_Release(local_server);
        }
    }

    if (stream)
        IStream_Release(stream);

    return hr;
}

HRESULT rpc_register_local_server(REFCLSID clsid, IStream *stream, DWORD flags, unsigned int *cookie)
{
    MInterfacePointer *obj;
    const void *ptr;
    HGLOBAL hmem;
    SIZE_T size;
    HRESULT hr;

    TRACE("%s, %#lx\n", debugstr_guid(clsid), flags);

    hr = GetHGlobalFromStream(stream, &hmem);
    if (FAILED(hr)) return hr;

    size = GlobalSize(hmem);
    if (!(obj = malloc(FIELD_OFFSET(MInterfacePointer, abData[size]))))
        return E_OUTOFMEMORY;
    obj->ulCntData = size;
    ptr = GlobalLock(hmem);
    memcpy(obj->abData, ptr, size);
    GlobalUnlock(hmem);

    hr = rpcss_server_register(clsid, flags, obj, cookie);

    free(obj);

    return hr;
}

static HRESULT unmarshal_ORPCTHAT(RPC_MESSAGE *msg, ORPCTHAT *orpcthat, ORPC_EXTENT_ARRAY *orpc_ext_array,
        WIRE_ORPC_EXTENT **first_wire_orpc_extent);

/* Channel Hook Functions */

static ULONG ChannelHooks_ClientGetSize(SChannelHookCallInfo *info, struct channel_hook_buffer_data **data,
        unsigned int *hook_count, ULONG *extension_count)
{
    struct channel_hook_entry *entry;
    ULONG total_size = 0;
    unsigned int hook_index = 0;

    *hook_count = 0;
    *extension_count = 0;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
        (*hook_count)++;

    if (*hook_count)
        *data = malloc(*hook_count * sizeof(struct channel_hook_buffer_data));
    else
        *data = NULL;

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        ULONG extension_size = 0;

        IChannelHook_ClientGetSize(entry->hook, &entry->id, &info->iid, &extension_size);

        TRACE("%s: extension_size = %lu\n", debugstr_guid(&entry->id), extension_size);

        extension_size = (extension_size+7)&~7;
        (*data)[hook_index].id = entry->id;
        (*data)[hook_index].extension_size = extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (extension_size)
        {
            total_size += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[extension_size]);
            (*extension_count)++;
        }

        hook_index++;
    }

    LeaveCriticalSection(&csChannelHook);

    return total_size;
}

static unsigned char * ChannelHooks_ClientFillBuffer(SChannelHookCallInfo *info,
    unsigned char *buffer, struct channel_hook_buffer_data *data,
    unsigned int hook_count)
{
    struct channel_hook_entry *entry;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        unsigned int i;
        ULONG extension_size = 0;
        WIRE_ORPC_EXTENT *wire_orpc_extent = (WIRE_ORPC_EXTENT *)buffer;

        for (i = 0; i < hook_count; i++)
            if (IsEqualGUID(&entry->id, &data[i].id))
                extension_size = data[i].extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (!extension_size)
            continue;

        IChannelHook_ClientFillBuffer(entry->hook, &entry->id, &info->iid,
            &extension_size, buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]));

        TRACE("%s: extension_size = %lu\n", debugstr_guid(&entry->id), extension_size);

        /* FIXME: set unused portion of wire_orpc_extent->data to 0? */

        wire_orpc_extent->conformance = (extension_size+7)&~7;
        wire_orpc_extent->size = extension_size;
        wire_orpc_extent->id = entry->id;
        buffer += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[wire_orpc_extent->conformance]);
    }

    LeaveCriticalSection(&csChannelHook);

    return buffer;
}

static void ChannelHooks_ServerNotify(SChannelHookCallInfo *info,
    DWORD lDataRep, WIRE_ORPC_EXTENT *first_wire_orpc_extent,
    ULONG extension_count)
{
    struct channel_hook_entry *entry;
    ULONG i;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;
        for (i = 0, wire_orpc_extent = first_wire_orpc_extent;
             i < extension_count;
             i++, wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance])
        {
            if (IsEqualGUID(&entry->id, &wire_orpc_extent->id))
                break;
        }
        if (i == extension_count) wire_orpc_extent = NULL;

        IChannelHook_ServerNotify(entry->hook, &entry->id, &info->iid,
            wire_orpc_extent ? wire_orpc_extent->size : 0,
            wire_orpc_extent ? wire_orpc_extent->data : NULL,
            lDataRep);
    }

    LeaveCriticalSection(&csChannelHook);
}

static ULONG ChannelHooks_ServerGetSize(SChannelHookCallInfo *info,
                                        struct channel_hook_buffer_data **data, unsigned int *hook_count,
                                        ULONG *extension_count)
{
    struct channel_hook_entry *entry;
    ULONG total_size = 0;
    unsigned int hook_index = 0;

    *hook_count = 0;
    *extension_count = 0;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
        (*hook_count)++;

    if (*hook_count)
        *data = malloc(*hook_count * sizeof(struct channel_hook_buffer_data));
    else
        *data = NULL;

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        ULONG extension_size = 0;

        IChannelHook_ServerGetSize(entry->hook, &entry->id, &info->iid, S_OK,
                                   &extension_size);

        TRACE("%s: extension_size = %lu\n", debugstr_guid(&entry->id), extension_size);

        extension_size = (extension_size+7)&~7;
        (*data)[hook_index].id = entry->id;
        (*data)[hook_index].extension_size = extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (extension_size)
        {
            total_size += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[extension_size]);
            (*extension_count)++;
        }

        hook_index++;
    }

    LeaveCriticalSection(&csChannelHook);

    return total_size;
}

static unsigned char * ChannelHooks_ServerFillBuffer(SChannelHookCallInfo *info,
                                                     unsigned char *buffer, struct channel_hook_buffer_data *data,
                                                     unsigned int hook_count)
{
    struct channel_hook_entry *entry;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        unsigned int i;
        ULONG extension_size = 0;
        WIRE_ORPC_EXTENT *wire_orpc_extent = (WIRE_ORPC_EXTENT *)buffer;

        for (i = 0; i < hook_count; i++)
            if (IsEqualGUID(&entry->id, &data[i].id))
                extension_size = data[i].extension_size;

        /* an extension is only put onto the wire if it has data to write */
        if (!extension_size)
            continue;

        IChannelHook_ServerFillBuffer(entry->hook, &entry->id, &info->iid,
                                      &extension_size, buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]),
                                      S_OK);

        TRACE("%s: extension_size = %lu\n", debugstr_guid(&entry->id), extension_size);

        /* FIXME: set unused portion of wire_orpc_extent->data to 0? */

        wire_orpc_extent->conformance = (extension_size+7)&~7;
        wire_orpc_extent->size = extension_size;
        wire_orpc_extent->id = entry->id;
        buffer += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[wire_orpc_extent->conformance]);
    }

    LeaveCriticalSection(&csChannelHook);

    return buffer;
}

static void ChannelHooks_ClientNotify(SChannelHookCallInfo *info,
                                      DWORD lDataRep, WIRE_ORPC_EXTENT *first_wire_orpc_extent,
                                      ULONG extension_count, HRESULT hrFault)
{
    struct channel_hook_entry *entry;
    ULONG i;

    EnterCriticalSection(&csChannelHook);

    LIST_FOR_EACH_ENTRY(entry, &channel_hooks, struct channel_hook_entry, entry)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;
        for (i = 0, wire_orpc_extent = first_wire_orpc_extent;
             i < extension_count;
             i++, wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance])
        {
            if (IsEqualGUID(&entry->id, &wire_orpc_extent->id))
                break;
        }
        if (i == extension_count) wire_orpc_extent = NULL;

        IChannelHook_ClientNotify(entry->hook, &entry->id, &info->iid,
                                  wire_orpc_extent ? wire_orpc_extent->size : 0,
                                  wire_orpc_extent ? wire_orpc_extent->data : NULL,
                                  lDataRep, hrFault);
    }

    LeaveCriticalSection(&csChannelHook);
}

HRESULT rpc_register_channel_hook(REFGUID rguid, IChannelHook *hook)
{
    struct channel_hook_entry *entry;

    entry = malloc(sizeof(*entry));
    if (!entry)
        return E_OUTOFMEMORY;

    entry->id = *rguid;
    entry->hook = hook;
    IChannelHook_AddRef(hook);

    EnterCriticalSection(&csChannelHook);
    list_add_tail(&channel_hooks, &entry->entry);
    LeaveCriticalSection(&csChannelHook);

    return S_OK;
}

void rpc_unregister_channel_hooks(void)
{
    struct channel_hook_entry *cursor;
    struct channel_hook_entry *cursor2;

    EnterCriticalSection(&csChannelHook);
    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &channel_hooks, struct channel_hook_entry, entry)
        free(cursor);
    LeaveCriticalSection(&csChannelHook);
    DeleteCriticalSection(&csChannelHook);
    DeleteCriticalSection(&csRegIf);
}

/* RPC Channel Buffer Functions */

static HRESULT WINAPI RpcChannelBuffer_QueryInterface(IRpcChannelBuffer *iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown))
    {
        *ppv = iface;
        IRpcChannelBuffer_AddRef(iface);
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

    free(This);
    return 0;
}

static ULONG WINAPI ClientRpcChannelBuffer_Release(LPRPCCHANNELBUFFER iface)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->super.refs);
    if (ref)
        return ref;

    if (This->event) CloseHandle(This->event);
    RpcBindingFree(&This->bind);
    free(This);
    return 0;
}

static HRESULT WINAPI ServerRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    ORPCTHAT *orpcthat;
    struct message_state *message_state;
    ULONG extensions_size;
    struct channel_hook_buffer_data *channel_hook_data;
    unsigned int channel_hook_count;
    ULONG extension_count;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;

    extensions_size = ChannelHooks_ServerGetSize(&message_state->channel_hook_info,
                                                 &channel_hook_data, &channel_hook_count, &extension_count);

    msg->BufferLength += FIELD_OFFSET(WIRE_ORPCTHAT, extensions) + sizeof(DWORD);
    if (extensions_size)
    {
        msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent[2*sizeof(DWORD) + extensions_size]);
        if (extension_count & 1)
            msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
    }

    if (message_state->bypass_rpcrt)
    {
        msg->Buffer = malloc(msg->BufferLength);
        if (msg->Buffer)
            status = RPC_S_OK;
        else
        {
            free(channel_hook_data);
            return E_OUTOFMEMORY;
        }
    }
    else
        status = I_RpcGetBuffer(msg);

    orpcthat = msg->Buffer;
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPCTHAT, extensions);

    orpcthat->flags = ORPCF_NULL /* FIXME? */;

    /* NDR representation of orpcthat->extensions */
    *(DWORD *)msg->Buffer = extensions_size ? 1 : 0;
    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (extensions_size)
    {
        WIRE_ORPC_EXTENT_ARRAY *orpc_extent_array = msg->Buffer;
        orpc_extent_array->size = extension_count;
        orpc_extent_array->reserved = 0;
        msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent);
        /* NDR representation of orpc_extent_array->extent */
        *(DWORD *)msg->Buffer = 1;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
        /* NDR representation of [size_is] attribute of orpc_extent_array->extent */
        *(DWORD *)msg->Buffer = (extension_count + 1) & ~1;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        msg->Buffer = ChannelHooks_ServerFillBuffer(&message_state->channel_hook_info,
                                                    msg->Buffer, channel_hook_data, channel_hook_count);

        /* we must add a dummy extension if there is an odd extension
         * count to meet the contract specified by the size_is attribute */
        if (extension_count & 1)
        {
            WIRE_ORPC_EXTENT *wire_orpc_extent = msg->Buffer;
            wire_orpc_extent->conformance = 0;
            wire_orpc_extent->id = GUID_NULL;
            wire_orpc_extent->size = 0;
            msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
        }
    }

    free(channel_hook_data);

    /* store the prefixed data length so that we can restore the real buffer
     * later */
    message_state->prefix_data_len = (char *)msg->Buffer - (char *)orpcthat;
    msg->BufferLength -= message_state->prefix_data_len;
    /* save away the message state again */
    msg->Handle = message_state;

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HANDLE ClientRpcChannelBuffer_GetEventHandle(ClientRpcChannelBuffer *This)
{
    HANDLE event = InterlockedExchangePointer(&This->event, NULL);

    /* Note: must be auto-reset event so we can reuse it without a call
    * to ResetEvent */
    if (!event) event = CreateEventW(NULL, FALSE, FALSE, NULL);

    return event;
}

static void ClientRpcChannelBuffer_ReleaseEventHandle(ClientRpcChannelBuffer *This, HANDLE event)
{
    if (InterlockedCompareExchangePointer(&This->event, event, NULL))
        /* already a handle cached in This */
        CloseHandle(event);
}

static HRESULT WINAPI ClientRpcChannelBuffer_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_CLIENT_INTERFACE *cif;
    RPC_STATUS status;
    ORPCTHIS *orpcthis;
    struct message_state *message_state;
    ULONG extensions_size;
    struct channel_hook_buffer_data *channel_hook_data;
    unsigned int channel_hook_count;
    ULONG extension_count;
    IPID ipid;
    HRESULT hr;
    struct apartment *apt = NULL;

    TRACE("(%p)->(%p,%s)\n", This, olemsg, debugstr_guid(riid));

    cif = calloc(1, sizeof(RPC_CLIENT_INTERFACE));
    if (!cif)
        return E_OUTOFMEMORY;

    message_state = malloc(sizeof(*message_state));
    if (!message_state)
    {
        free(cif);
        return E_OUTOFMEMORY;
    }

    cif->Length = sizeof(RPC_CLIENT_INTERFACE);
    /* RPC interface ID = COM interface ID */
    cif->InterfaceId.SyntaxGUID = This->iid;
    /* COM objects always have a version of 0.0 */
    cif->InterfaceId.SyntaxVersion.MajorVersion = 0;
    cif->InterfaceId.SyntaxVersion.MinorVersion = 0;
    msg->Handle = This->bind;
    msg->RpcInterfaceInformation = cif;

    message_state->prefix_data_len = 0;
    message_state->binding_handle = This->bind;

    message_state->channel_hook_info.iid = *riid;
    message_state->channel_hook_info.cbSize = sizeof(message_state->channel_hook_info);
    CoGetCurrentLogicalThreadId(&message_state->channel_hook_info.uCausality);
    message_state->channel_hook_info.dwServerPid = This->server_pid;
    message_state->channel_hook_info.iMethod = msg->ProcNum & ~RPC_FLAGS_VALID_BIT;
    message_state->channel_hook_info.pObject = NULL; /* only present on server-side */
    message_state->target_hwnd = NULL;
    message_state->target_tid = 0;
    memset(&message_state->params, 0, sizeof(message_state->params));

    extensions_size = ChannelHooks_ClientGetSize(&message_state->channel_hook_info,
        &channel_hook_data, &channel_hook_count, &extension_count);

    msg->BufferLength += FIELD_OFFSET(WIRE_ORPCTHIS, extensions) + sizeof(DWORD);
    if (extensions_size)
    {
        msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent[2*sizeof(DWORD) + extensions_size]);
        if (extension_count & 1)
            msg->BufferLength += FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
    }

    RpcBindingInqObject(message_state->binding_handle, &ipid);
    hr = ipid_get_dispatch_params(&ipid, &apt, NULL, &message_state->params.stub,
                                  &message_state->params.chan,
                                  &message_state->params.iid,
                                  &message_state->params.iface);
    if (hr == S_OK)
    {
        /* stub, chan, iface and iid are unneeded in multi-threaded case as we go
         * via the RPC runtime */
        if (apt->multi_threaded)
        {
            IRpcStubBuffer_Release(message_state->params.stub);
            message_state->params.stub = NULL;
            IRpcChannelBuffer_Release(message_state->params.chan);
            message_state->params.chan = NULL;
            message_state->params.iface = NULL;
        }
        else
        {
            message_state->params.bypass_rpcrt = TRUE;
            message_state->target_hwnd = apartment_getwindow(apt);
            message_state->target_tid = apt->tid;
            /* we assume later on that this being non-NULL is the indicator that
             * means call directly instead of going through RPC runtime */
            if (!message_state->target_hwnd)
                ERR("window for apartment %s is NULL\n", wine_dbgstr_longlong(apt->oxid));
        }
    }
    if (apt) apartment_release(apt);
    message_state->params.handle = ClientRpcChannelBuffer_GetEventHandle(This);
    /* Note: message_state->params.msg is initialised in
     * ClientRpcChannelBuffer_SendReceive */

    /* shortcut the RPC runtime */
    if (message_state->target_hwnd)
    {
        msg->Buffer = malloc(msg->BufferLength);
        if (msg->Buffer)
            status = RPC_S_OK;
        else
            status = ERROR_OUTOFMEMORY;
    }
    else
        status = I_RpcGetBuffer(msg);

    msg->Handle = message_state;

    if (status == RPC_S_OK)
    {
        orpcthis = msg->Buffer;
        msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPCTHIS, extensions);

        orpcthis->version.MajorVersion = COM_MAJOR_VERSION;
        orpcthis->version.MinorVersion = COM_MINOR_VERSION;
        orpcthis->flags = message_state->channel_hook_info.dwServerPid ? ORPCF_LOCAL : ORPCF_NULL;
        orpcthis->reserved1 = 0;
        orpcthis->cid = message_state->channel_hook_info.uCausality;

        /* NDR representation of orpcthis->extensions */
        *(DWORD *)msg->Buffer = extensions_size ? 1 : 0;
        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        if (extensions_size)
        {
            ORPC_EXTENT_ARRAY *orpc_extent_array = msg->Buffer;
            orpc_extent_array->size = extension_count;
            orpc_extent_array->reserved = 0;
            msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent);
            /* NDR representation of orpc_extent_array->extent */
            *(DWORD *)msg->Buffer = 1;
            msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
            /* NDR representation of [size_is] attribute of orpc_extent_array->extent */
            *(DWORD *)msg->Buffer = (extension_count + 1) & ~1;
            msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

            msg->Buffer = ChannelHooks_ClientFillBuffer(&message_state->channel_hook_info,
                msg->Buffer, channel_hook_data, channel_hook_count);

            /* we must add a dummy extension if there is an odd extension
             * count to meet the contract specified by the size_is attribute */
            if (extension_count & 1)
            {
                WIRE_ORPC_EXTENT *wire_orpc_extent = msg->Buffer;
                wire_orpc_extent->conformance = 0;
                wire_orpc_extent->id = GUID_NULL;
                wire_orpc_extent->size = 0;
                msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT, data[0]);
            }
        }

        /* store the prefixed data length so that we can restore the real buffer
         * pointer in ClientRpcChannelBuffer_SendReceive. */
        message_state->prefix_data_len = (char *)msg->Buffer - (char *)orpcthis;
        msg->BufferLength -= message_state->prefix_data_len;
    }

    free(channel_hook_data);

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ServerRpcChannelBuffer_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/* this thread runs an outgoing RPC */
static DWORD WINAPI rpc_sendreceive_thread(LPVOID param)
{
    struct dispatch_params *data = param;

    /* Note: I_RpcSendReceive doesn't raise exceptions like the higher-level
     * RPC functions do */
    data->status = I_RpcSendReceive((RPC_MESSAGE *)data->msg);

    TRACE("completed with status %#lx\n", data->status);

    SetEvent(data->handle);

    return 0;
}

static inline HRESULT ClientRpcChannelBuffer_IsCorrectApartment(ClientRpcChannelBuffer *This, const struct apartment *apt)
{
    if (!apt)
        return S_FALSE;
    if (This->oxid != apartment_getoxid(apt))
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI ClientRpcChannelBuffer_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;
    HRESULT hr;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    DWORD index;
    struct message_state *message_state;
    ORPCTHAT orpcthat;
    ORPC_EXTENT_ARRAY orpc_ext_array;
    WIRE_ORPC_EXTENT *first_wire_orpc_extent = NULL;
    HRESULT hrFault = S_OK;
    struct apartment *apt = apartment_get_current_or_mta();
    struct tlsdata *tlsdata;

    TRACE("%p, iMethod %ld\n", olemsg, olemsg->iMethod);

    hr = ClientRpcChannelBuffer_IsCorrectApartment(This, apt);
    if (hr != S_OK)
    {
        ERR("called from wrong apartment, should have been 0x%s\n",
            wine_dbgstr_longlong(This->oxid));
        if (apt) apartment_release(apt);
        return RPC_E_WRONG_THREAD;
    }

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    /* This situation should be impossible in multi-threaded apartments,
     * because the calling thread isn't re-enterable.
     * Note: doing a COM call during the processing of a sent message is
     * only disallowed if a client call is already being waited for
     * completion */
    if (!apt->multi_threaded &&
        tlsdata->pending_call_count_client &&
        InSendMessage())
    {
        ERR("can't make an outgoing COM call in response to a sent message\n");
        apartment_release(apt);
        return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
    }

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

    /* Note: this is an optimization in the Microsoft OLE runtime that we need
     * to copy, as shown by the test_no_couninitialize_client test. without
     * short-circuiting the RPC runtime in the case below, the test will
     * deadlock on the loader lock due to the RPC runtime needing to create
     * a thread to process the RPC when this function is called indirectly
     * from DllMain */

    message_state->params.msg = olemsg;
    if (message_state->params.bypass_rpcrt)
    {
        TRACE("Calling apartment thread %#lx...\n", message_state->target_tid);

        msg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

        if (!PostMessageW(message_state->target_hwnd, DM_EXECUTERPC, 0,
                          (LPARAM)&message_state->params))
        {
            ERR("PostMessage failed with error %lu\n", GetLastError());

            /* Note: message_state->params.iface doesn't have a reference and
             * so doesn't need to be released */

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        /* we use a separate thread here because we need to be able to
         * pump the message loop in the application thread: if we do not,
         * any windows created by this thread will hang and RPCs that try
         * and re-enter this STA from an incoming server thread will
         * deadlock. InstallShield is an example of that.
         */
        if (!QueueUserWorkItem(rpc_sendreceive_thread, &message_state->params, WT_EXECUTEDEFAULT))
        {
            ERR("QueueUserWorkItem failed with error %lu\n", GetLastError());
            hr = E_UNEXPECTED;
        }
        else
            hr = S_OK;
    }

    if (hr == S_OK)
    {
        if (WaitForSingleObject(message_state->params.handle, 0))
        {
            tlsdata->pending_call_count_client++;
            hr = CoWaitForMultipleHandles(0, INFINITE, 1, &message_state->params.handle, &index);
            tlsdata->pending_call_count_client--;
        }
    }
    ClientRpcChannelBuffer_ReleaseEventHandle(This, message_state->params.handle);

    /* for WM shortcut, faults are returned in params->hr */
    if (hr == S_OK)
        hrFault = message_state->params.hr;

    status = message_state->params.status;

    orpcthat.flags = ORPCF_NULL;
    orpcthat.extensions = NULL;

    TRACE("RPC call status: %#lx\n", status);
    if (status != RPC_S_OK)
        hr = HRESULT_FROM_WIN32(status);

    TRACE("hrFault = %#lx\n", hrFault);

    /* FIXME: this condition should be
     * "hr == S_OK && (!hrFault || msg->BufferLength > FIELD_OFFSET(ORPCTHAT, extensions) + 4)"
     * but we don't currently reset the message length for PostMessage
     * dispatched calls */
    if (hr == S_OK && hrFault == S_OK)
    {
        HRESULT hr2;
        char *original_buffer = msg->Buffer;

        /* handle ORPCTHAT and client extensions */

        hr2 = unmarshal_ORPCTHAT(msg, &orpcthat, &orpc_ext_array, &first_wire_orpc_extent);
        if (FAILED(hr2))
            hr = hr2;

        message_state->prefix_data_len = (char *)msg->Buffer - original_buffer;
        msg->BufferLength -= message_state->prefix_data_len;
    }
    else
        message_state->prefix_data_len = 0;

    if (hr == S_OK)
    {
        ChannelHooks_ClientNotify(&message_state->channel_hook_info,
                                  msg->DataRepresentation,
                                  first_wire_orpc_extent,
                                  orpcthat.extensions && first_wire_orpc_extent ? orpcthat.extensions->size : 0,
                                  hrFault);
    }

    /* save away the message state again */
    msg->Handle = message_state;

    if (pstatus) *pstatus = status;

    if (hr == S_OK)
        hr = hrFault;

    TRACE("-- %#lx\n", hr);

    apartment_release(apt);
    return hr;
}

static HRESULT WINAPI ServerRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    struct message_state *message_state;

    TRACE("(%p)\n", msg);

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;
    message_state->prefix_data_len = 0;

    if (message_state->bypass_rpcrt)
    {
        free(msg->Buffer);
        status = RPC_S_OK;
    }
    else
        status = I_RpcFreeBuffer(msg);

    msg->Handle = message_state;

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    RPC_MESSAGE *msg = (RPC_MESSAGE *)olemsg;
    RPC_STATUS status;
    struct message_state *message_state;

    TRACE("(%p)\n", msg);

    message_state = msg->Handle;
    /* restore the binding handle and the real start of data */
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

    if (message_state->params.bypass_rpcrt)
    {
        free(msg->Buffer);
        status = RPC_S_OK;
    }
    else
        status = I_RpcFreeBuffer(msg);

    free(msg->RpcInterfaceInformation);
    msg->RpcInterfaceInformation = NULL;

    if (message_state->params.stub)
        IRpcStubBuffer_Release(message_state->params.stub);
    if (message_state->params.chan)
        IRpcChannelBuffer_Release(message_state->params.chan);
    free(message_state);

    TRACE("-- %ld\n", status);

    return HRESULT_FROM_WIN32(status);
}

static HRESULT WINAPI ClientRpcChannelBuffer_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    ClientRpcChannelBuffer *This = (ClientRpcChannelBuffer *)iface;

    TRACE("(%p,%p)\n", pdwDestContext, ppvDestContext);

    *pdwDestContext = This->super.dest_context;
    *ppvDestContext = This->super.dest_context_data;

    return S_OK;
}

static HRESULT WINAPI ServerRpcChannelBuffer_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* dest_context, void** dest_context_data)
{
    RpcChannelBuffer *This = (RpcChannelBuffer *)iface;

    TRACE("(%p,%p)\n", dest_context, dest_context_data);

    *dest_context = This->dest_context;
    *dest_context_data = This->dest_context_data;
    return S_OK;
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
    ClientRpcChannelBuffer_SendReceive,
    ClientRpcChannelBuffer_FreeBuffer,
    ClientRpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

static const IRpcChannelBufferVtbl ServerRpcChannelBufferVtbl =
{
    RpcChannelBuffer_QueryInterface,
    RpcChannelBuffer_AddRef,
    ServerRpcChannelBuffer_Release,
    ServerRpcChannelBuffer_GetBuffer,
    ServerRpcChannelBuffer_SendReceive,
    ServerRpcChannelBuffer_FreeBuffer,
    ServerRpcChannelBuffer_GetDestCtx,
    RpcChannelBuffer_IsConnected
};

/* returns a channel buffer for proxies */
HRESULT rpc_create_clientchannel(const OXID *oxid, const IPID *ipid,
                                const OXID_INFO *oxid_info, const IID *iid,
                                DWORD dest_context, void *dest_context_data,
                                IRpcChannelBuffer **chan, struct apartment *apt)
{
    ClientRpcChannelBuffer *This;
    WCHAR                   endpoint[200];
    RPC_BINDING_HANDLE      bind;
    RPC_STATUS              status;
    LPWSTR                  string_binding;

    /* FIXME: get the endpoint from oxid_info->psa instead */
    get_rpc_endpoint(endpoint, oxid);

    TRACE("proxy pipe: connecting to endpoint: %s\n", debugstr_w(endpoint));

    status = RpcStringBindingComposeW(
        NULL,
        rpctransportW,
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

    This = malloc(sizeof(*This));
    if (!This)
    {
        RpcBindingFree(&bind);
        return E_OUTOFMEMORY;
    }

    This->super.IRpcChannelBuffer_iface.lpVtbl = &ClientRpcChannelBufferVtbl;
    This->super.refs = 1;
    This->super.dest_context = dest_context;
    This->super.dest_context_data = dest_context_data;
    This->bind = bind;
    This->oxid = apartment_getoxid(apt);
    This->server_pid = oxid_info->dwPid;
    This->event = NULL;
    This->iid = *iid;

    *chan = &This->super.IRpcChannelBuffer_iface;

    return S_OK;
}

HRESULT rpc_create_serverchannel(DWORD dest_context, void *dest_context_data, IRpcChannelBuffer **chan)
{
    RpcChannelBuffer *This = malloc(sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IRpcChannelBuffer_iface.lpVtbl = &ServerRpcChannelBufferVtbl;
    This->refs = 1;
    This->dest_context = dest_context;
    This->dest_context_data = dest_context_data;

    *chan = &This->IRpcChannelBuffer_iface;

    return S_OK;
}

/* unmarshals ORPC_EXTENT_ARRAY according to NDR rules, but doesn't allocate
 * any memory */
static HRESULT unmarshal_ORPC_EXTENT_ARRAY(RPC_MESSAGE *msg, const char *end,
                                           ORPC_EXTENT_ARRAY *extensions,
                                           WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    DWORD pointer_id;
    DWORD i;

    memcpy(extensions, msg->Buffer, FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPC_EXTENT_ARRAY, extent);

    if ((const char *)msg->Buffer + 2 * sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    pointer_id = *(DWORD *)msg->Buffer;
    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);
    extensions->extent = NULL;

    if (pointer_id)
    {
        WIRE_ORPC_EXTENT *wire_orpc_extent;

        /* conformance */
        if (*(DWORD *)msg->Buffer != ((extensions->size+1)&~1))
            return RPC_S_INVALID_BOUND;

        msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

        /* arbitrary limit for security (don't know what native does) */
        if (extensions->size > 256)
        {
            ERR("too many extensions: %ld\n", extensions->size);
            return RPC_S_INVALID_BOUND;
        }

        *first_wire_orpc_extent = wire_orpc_extent = msg->Buffer;
        for (i = 0; i < ((extensions->size+1)&~1); i++)
        {
            if ((const char *)&wire_orpc_extent->data[0] > end)
                return RPC_S_INVALID_BOUND;
            if (wire_orpc_extent->conformance != ((wire_orpc_extent->size+7)&~7))
                return RPC_S_INVALID_BOUND;
            if ((const char *)&wire_orpc_extent->data[wire_orpc_extent->conformance] > end)
                return RPC_S_INVALID_BOUND;
            TRACE("size %lu, guid %s\n", wire_orpc_extent->size, debugstr_guid(&wire_orpc_extent->id));
            wire_orpc_extent = (WIRE_ORPC_EXTENT *)&wire_orpc_extent->data[wire_orpc_extent->conformance];
        }
        msg->Buffer = wire_orpc_extent;
    }

    return S_OK;
}

/* unmarshals ORPCTHIS according to NDR rules, but doesn't allocate any memory */
static HRESULT unmarshal_ORPCTHIS(RPC_MESSAGE *msg, ORPCTHIS *orpcthis,
    ORPC_EXTENT_ARRAY *orpc_ext_array, WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    const char *end = (char *)msg->Buffer + msg->BufferLength;

    *first_wire_orpc_extent = NULL;

    if (msg->BufferLength < FIELD_OFFSET(WIRE_ORPCTHIS, extensions) + sizeof(DWORD))
    {
        ERR("invalid buffer length\n");
        return RPC_E_INVALID_HEADER;
    }

    memcpy(orpcthis, msg->Buffer, FIELD_OFFSET(WIRE_ORPCTHIS, extensions));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPCTHIS, extensions);

    if ((const char *)msg->Buffer + sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    if (*(DWORD *)msg->Buffer)
        orpcthis->extensions = orpc_ext_array;
    else
        orpcthis->extensions = NULL;

    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (orpcthis->extensions)
    {
        HRESULT hr = unmarshal_ORPC_EXTENT_ARRAY(msg, end, orpc_ext_array,
                                                 first_wire_orpc_extent);
        if (FAILED(hr))
            return hr;
    }

    if ((orpcthis->version.MajorVersion != COM_MAJOR_VERSION) ||
        (orpcthis->version.MinorVersion > COM_MINOR_VERSION))
    {
        ERR("COM version {%d, %d} not supported\n",
            orpcthis->version.MajorVersion, orpcthis->version.MinorVersion);
        return RPC_E_VERSION_MISMATCH;
    }

    if (orpcthis->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4))
    {
        ERR("invalid flags %#lx\n", orpcthis->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4));
        return RPC_E_INVALID_HEADER;
    }

    return S_OK;
}

static HRESULT unmarshal_ORPCTHAT(RPC_MESSAGE *msg, ORPCTHAT *orpcthat,
                                  ORPC_EXTENT_ARRAY *orpc_ext_array, WIRE_ORPC_EXTENT **first_wire_orpc_extent)
{
    const char *end = (char *)msg->Buffer + msg->BufferLength;

    *first_wire_orpc_extent = NULL;

    if (msg->BufferLength < FIELD_OFFSET(WIRE_ORPCTHAT, extensions) + sizeof(DWORD))
    {
        ERR("invalid buffer length\n");
        return RPC_E_INVALID_HEADER;
    }

    memcpy(orpcthat, msg->Buffer, FIELD_OFFSET(WIRE_ORPCTHAT, extensions));
    msg->Buffer = (char *)msg->Buffer + FIELD_OFFSET(WIRE_ORPCTHAT, extensions);

    if ((const char *)msg->Buffer + sizeof(DWORD) > end)
        return RPC_E_INVALID_HEADER;

    if (*(DWORD *)msg->Buffer)
        orpcthat->extensions = orpc_ext_array;
    else
        orpcthat->extensions = NULL;

    msg->Buffer = (char *)msg->Buffer + sizeof(DWORD);

    if (orpcthat->extensions)
    {
        HRESULT hr = unmarshal_ORPC_EXTENT_ARRAY(msg, end, orpc_ext_array,
                                                 first_wire_orpc_extent);
        if (FAILED(hr))
            return hr;
    }

    if (orpcthat->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4))
    {
        ERR("invalid flags %#lx\n", orpcthat->flags & ~(ORPCF_LOCAL|ORPCF_RESERVED1|ORPCF_RESERVED2|ORPCF_RESERVED3|ORPCF_RESERVED4));
        return RPC_E_INVALID_HEADER;
    }

    return S_OK;
}

void rpc_execute_call(struct dispatch_params *params)
{
    struct message_state *message_state = NULL;
    RPC_MESSAGE *msg = (RPC_MESSAGE *)params->msg;
    char *original_buffer = msg->Buffer;
    ORPCTHIS orpcthis;
    ORPC_EXTENT_ARRAY orpc_ext_array;
    WIRE_ORPC_EXTENT *first_wire_orpc_extent;
    GUID old_causality_id;
    struct tlsdata *tlsdata;
    struct apartment *apt;

    if (FAILED(com_get_tlsdata(&tlsdata)))
        return;

    apt = com_get_current_apt();

    /* handle ORPCTHIS and server extensions */

    params->hr = unmarshal_ORPCTHIS(msg, &orpcthis, &orpc_ext_array, &first_wire_orpc_extent);
    if (params->hr != S_OK)
    {
        msg->Buffer = original_buffer;
        goto exit;
    }

    message_state = malloc(sizeof(*message_state));
    if (!message_state)
    {
        params->hr = E_OUTOFMEMORY;
        msg->Buffer = original_buffer;
        goto exit;
    }

    message_state->prefix_data_len = (char *)msg->Buffer - original_buffer;
    message_state->binding_handle = msg->Handle;
    message_state->bypass_rpcrt = params->bypass_rpcrt;

    message_state->channel_hook_info.iid = params->iid;
    message_state->channel_hook_info.cbSize = sizeof(message_state->channel_hook_info);
    message_state->channel_hook_info.uCausality = orpcthis.cid;
    message_state->channel_hook_info.dwServerPid = GetCurrentProcessId();
    message_state->channel_hook_info.iMethod = msg->ProcNum;
    message_state->channel_hook_info.pObject = params->iface;

    if (orpcthis.extensions && first_wire_orpc_extent &&
        orpcthis.extensions->size)
        ChannelHooks_ServerNotify(&message_state->channel_hook_info, msg->DataRepresentation, first_wire_orpc_extent, orpcthis.extensions->size);

    msg->Handle = message_state;
    msg->BufferLength -= message_state->prefix_data_len;

    /* call message filter */

    if (apt->filter)
    {
        DWORD handlecall;
        INTERFACEINFO interface_info;
        CALLTYPE calltype;

        interface_info.pUnk = params->iface;
        interface_info.iid = params->iid;
        interface_info.wMethod = msg->ProcNum;

        if (IsEqualGUID(&orpcthis.cid, &tlsdata->causality_id))
            calltype = CALLTYPE_NESTED;
        else if (tlsdata->pending_call_count_server == 0)
            calltype = CALLTYPE_TOPLEVEL;
        else
            calltype = CALLTYPE_TOPLEVEL_CALLPENDING;

        handlecall = IMessageFilter_HandleInComingCall(apt->filter,
                                                       calltype,
                                                       UlongToHandle(GetCurrentProcessId()),
                                                       0 /* FIXME */,
                                                       &interface_info);
        TRACE("IMessageFilter_HandleInComingCall returned %ld\n", handlecall);
        switch (handlecall)
        {
        case SERVERCALL_REJECTED:
            params->hr = RPC_E_CALL_REJECTED;
            goto exit_reset_state;
        case SERVERCALL_RETRYLATER:
#if 0 /* FIXME: handle retries on the client side before enabling this code */
            params->hr = RPC_E_RETRY;
            goto exit_reset_state;
#else
            FIXME("retry call later not implemented\n");
            break;
#endif
        case SERVERCALL_ISHANDLED:
        default:
            break;
        }
    }

    /* invoke the method */

    /* save the old causality ID - note: any calls executed while processing
     * messages received during the SendReceive will appear to originate from
     * this call - this should be checked with what Windows does */
    old_causality_id = tlsdata->causality_id;
    tlsdata->causality_id = orpcthis.cid;
    tlsdata->pending_call_count_server++;
    params->hr = IRpcStubBuffer_Invoke(params->stub, params->msg, params->chan);
    tlsdata->pending_call_count_server--;
    tlsdata->causality_id = old_causality_id;

    /* the invoke allocated a new buffer, so free the old one */
    if (message_state->bypass_rpcrt && original_buffer != msg->Buffer)
        free(original_buffer);

exit_reset_state:
    message_state = msg->Handle;
    msg->Handle = message_state->binding_handle;
    msg->Buffer = (char *)msg->Buffer - message_state->prefix_data_len;
    msg->BufferLength += message_state->prefix_data_len;

exit:
    free(message_state);
    if (params->handle) SetEvent(params->handle);
}

static void __RPC_STUB dispatch_rpc(RPC_MESSAGE *msg)
{
    struct dispatch_params *params;
    struct stub_manager *stub_manager;
    struct apartment *apt;
    IPID ipid;
    HRESULT hr;

    RpcBindingInqObject(msg->Handle, &ipid);

    TRACE("ipid = %s, iMethod = %d\n", debugstr_guid(&ipid), msg->ProcNum);

    params = malloc(sizeof(*params));
    if (!params)
    {
        RpcRaiseException(E_OUTOFMEMORY);
        return;
    }

    hr = ipid_get_dispatch_params(&ipid, &apt, &stub_manager, &params->stub, &params->chan,
                                  &params->iid, &params->iface);
    if (hr != S_OK)
    {
        ERR("no apartment found for ipid %s\n", debugstr_guid(&ipid));
        free(params);
        RpcRaiseException(hr);
        return;
    }

    params->msg = (RPCOLEMESSAGE *)msg;
    params->status = RPC_S_OK;
    params->hr = S_OK;
    params->handle = NULL;
    params->bypass_rpcrt = FALSE;

    /* Note: this is the important difference between STAs and MTAs - we
     * always execute RPCs to STAs in the thread that originally created the
     * apartment (i.e. the one that pumps messages to the window) */
    if (!apt->multi_threaded)
    {
        params->handle = CreateEventW(NULL, FALSE, FALSE, NULL);

        TRACE("Calling apartment thread %#lx...\n", apt->tid);

        if (PostMessageW(apartment_getwindow(apt), DM_EXECUTERPC, 0, (LPARAM)params))
            WaitForSingleObject(params->handle, INFINITE);
        else
        {
            ERR("PostMessage failed with error %lu\n", GetLastError());
            IRpcChannelBuffer_Release(params->chan);
            IRpcStubBuffer_Release(params->stub);
        }
        CloseHandle(params->handle);
    }
    else
    {
        BOOL joined = FALSE;
        struct tlsdata *tlsdata;

        com_get_tlsdata(&tlsdata);

        if (!tlsdata->apt)
        {
            enter_apartment(tlsdata, COINIT_MULTITHREADED);
            joined = TRUE;
        }
        rpc_execute_call(params);
        if (joined)
        {
            leave_apartment(tlsdata);
        }
    }

    hr = params->hr;
    if (params->chan)
        IRpcChannelBuffer_Release(params->chan);
    if (params->stub)
        IRpcStubBuffer_Release(params->stub);
    free(params);

    stub_manager_int_release(stub_manager);
    apartment_release(apt);

    /* if IRpcStubBuffer_Invoke fails, we should raise an exception to tell
     * the RPC runtime that the call failed */
    if (hr != S_OK) RpcRaiseException(hr);
}

/* stub registration */
HRESULT rpc_register_interface(REFIID riid)
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

        rif = calloc(1, sizeof(*rif));
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
                free(rif);
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
void rpc_unregister_interface(REFIID riid, BOOL wait)
{
    struct registered_if *rif;
    EnterCriticalSection(&csRegIf);
    LIST_FOR_EACH_ENTRY(rif, &registered_interfaces, struct registered_if, entry)
    {
        if (IsEqualGUID(&rif->If.InterfaceId.SyntaxGUID, riid))
        {
            if (!--rif->refs)
            {
                RpcServerUnregisterIf((RPC_IF_HANDLE)&rif->If, NULL, wait);
                list_remove(&rif->entry);
                free(rif);
            }
            break;
        }
    }
    LeaveCriticalSection(&csRegIf);
}

/* get the info for an OXID, including the IPID for the rem unknown interface
 * and the string binding */
HRESULT rpc_resolve_oxid(OXID oxid, OXID_INFO *oxid_info)
{
    TRACE("%s\n", wine_dbgstr_longlong(oxid));

    oxid_info->dwTid = 0;
    oxid_info->dwPid = 0;
    oxid_info->dwAuthnHint = RPC_C_AUTHN_LEVEL_NONE;
    /* FIXME: this is a hack around not having an OXID resolver yet -
     * this function should contact the machine's OXID resolver and then it
     * should give us the IPID of the IRemUnknown interface */
    oxid_info->ipidRemUnknown.Data1 = 0xffffffff;
    oxid_info->ipidRemUnknown.Data2 = 0xffff;
    oxid_info->ipidRemUnknown.Data3 = 0xffff;
    memcpy(oxid_info->ipidRemUnknown.Data4, &oxid, sizeof(OXID));
    oxid_info->psa = NULL /* FIXME */;

    return S_OK;
}

/* make the apartment reachable by other threads and processes and create the
 * IRemUnknown object */
void rpc_start_remoting(struct apartment *apt)
{
    if (!InterlockedExchange(&apt->remoting_started, TRUE))
    {
        WCHAR endpoint[200];
        RPC_STATUS status;

        get_rpc_endpoint(endpoint, &apt->oxid);

        status = RpcServerUseProtseqEpW(
            rpctransportW,
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            endpoint,
            NULL);
        if (status != RPC_S_OK)
            ERR("Couldn't register endpoint %s\n", debugstr_w(endpoint));

        /* FIXME: move remote unknown exporting into this function */
    }
    start_apartment_remote_unknown(apt);
}

/******************************************************************************
 *            DllDebugObjectRPCHook    (combase.@)
 */
BOOL WINAPI DllDebugObjectRPCHook(BOOL trace, /* ORPC_INIT_ARGS * */ void *args)
{
    FIXME("%d, %p: stub\n", trace, args);

    return TRUE;
}

/******************************************************************************
 *            CoDecodeProxy    (combase.@)
 */
HRESULT WINAPI CoDecodeProxy(DWORD client_pid, UINT64 proxy_addr, ServerInformation *server_info)
{
    FIXME("%lx, %s, %p.\n", client_pid, wine_dbgstr_longlong(proxy_addr), server_info);
    return E_NOTIMPL;
}
