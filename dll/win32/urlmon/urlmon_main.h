/*
 * Copyright 2002 Huw D M Davies for CodeWeavers
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#ifndef __WINE_URLMON_MAIN_H
#define __WINE_URLMON_MAIN_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <oleauto.h>
#include <urlmon.h>
#include <wininet.h>
#include <advpub.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

extern HINSTANCE hProxyDll DECLSPEC_HIDDEN;
extern HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT StdURLMoniker_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT HttpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT HttpSProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT FtpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT GopherProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT MkProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT MimeFilter_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT Uri_Construct(IUnknown *pUnkOuter, LPVOID *ppobj) DECLSPEC_HIDDEN;

extern BOOL WINAPI URLMON_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) DECLSPEC_HIDDEN;
extern HRESULT WINAPI URLMON_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI URLMON_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI URLMON_DllUnregisterServer(void) DECLSPEC_HIDDEN;

extern GUID const CLSID_PSFactoryBuffer DECLSPEC_HIDDEN;
extern GUID const CLSID_CUri DECLSPEC_HIDDEN;

/**********************************************************************
 * Dll lifetime tracking declaration for urlmon.dll
 */
extern LONG URLMON_refCount DECLSPEC_HIDDEN;
static inline void URLMON_LockModule(void) { InterlockedIncrement( &URLMON_refCount ); }
static inline void URLMON_UnlockModule(void) { InterlockedDecrement( &URLMON_refCount ); }

extern HINSTANCE urlmon_instance;

IInternetProtocolInfo *get_protocol_info(LPCWSTR) DECLSPEC_HIDDEN;
HRESULT get_protocol_handler(IUri*,CLSID*,BOOL*,IClassFactory**) DECLSPEC_HIDDEN;
IInternetProtocol *get_mime_filter(LPCWSTR) DECLSPEC_HIDDEN;
BOOL is_registered_protocol(LPCWSTR) DECLSPEC_HIDDEN;
HRESULT register_namespace(IClassFactory*,REFIID,LPCWSTR,BOOL) DECLSPEC_HIDDEN;
HINTERNET get_internet_session(IInternetBindInfo*) DECLSPEC_HIDDEN;
LPWSTR get_useragent(void) DECLSPEC_HIDDEN;
void free_session(void) DECLSPEC_HIDDEN;

HRESULT bind_to_storage(IUri*,IBindCtx*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT bind_to_object(IMoniker*,IUri*,IBindCtx*,REFIID,void**ppv) DECLSPEC_HIDDEN;

HRESULT create_default_callback(IBindStatusCallback**) DECLSPEC_HIDDEN;
HRESULT wrap_callback(IBindStatusCallback*,IBindStatusCallback**) DECLSPEC_HIDDEN;
IBindStatusCallback *bsc_from_bctx(IBindCtx*) DECLSPEC_HIDDEN;

typedef HRESULT (*stop_cache_binding_proc_t)(void*,const WCHAR*,HRESULT,const WCHAR*);
HRESULT download_to_cache(IUri*,stop_cache_binding_proc_t,void*,IBindStatusCallback*) DECLSPEC_HIDDEN;

typedef struct ProtocolVtbl ProtocolVtbl;

typedef struct {
    const ProtocolVtbl *vtbl;

    IInternetProtocol *protocol;
    IInternetProtocolSink *protocol_sink;

    DWORD bindf;
    BINDINFO bind_info;

    HINTERNET request;
    HINTERNET connection;
    DWORD flags;
    HANDLE lock;

    ULONG current_position;
    ULONG content_length;
    ULONG available_bytes;
    ULONG query_available;

    IStream *post_stream;

    LONG priority;
} Protocol;

struct ProtocolVtbl {
    HRESULT (*open_request)(Protocol*,IUri*,DWORD,HINTERNET,IInternetBindInfo*);
    HRESULT (*end_request)(Protocol*);
    HRESULT (*start_downloading)(Protocol*);
    void (*close_connection)(Protocol*);
    void (*on_error)(Protocol*,DWORD);
};

/* Flags are needed for, among other things, return HRESULTs from the Read function
 * to conform to native. For example, Read returns:
 *
 * 1. E_PENDING if called before the request has completed,
 *        (flags = 0)
 * 2. S_FALSE after all data has been read and S_OK has been reported,
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_ALL_DATA_READ | FLAG_RESULT_REPORTED)
 * 3. INET_E_DATA_NOT_AVAILABLE if InternetQueryDataAvailable fails. The first time
 *    this occurs, INET_E_DATA_NOT_AVAILABLE will also be reported to the sink,
 *        (flags = FLAG_REQUEST_COMPLETE)
 *    but upon subsequent calls to Read no reporting will take place, yet
 *    InternetQueryDataAvailable will still be called, and, on failure,
 *    INET_E_DATA_NOT_AVAILABLE will still be returned.
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_RESULT_REPORTED)
 *
 * FLAG_FIRST_DATA_REPORTED and FLAG_LAST_DATA_REPORTED are needed for proper
 * ReportData reporting. For example, if OnResponse returns S_OK, Continue will
 * report BSCF_FIRSTDATANOTIFICATION, and when all data has been read Read will
 * report BSCF_INTERMEDIATEDATANOTIFICATION|BSCF_LASTDATANOTIFICATION. However,
 * if OnResponse does not return S_OK, Continue will not report data, and Read
 * will report BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION when all
 * data has been read.
 */
#define FLAG_REQUEST_COMPLETE         0x0001
#define FLAG_FIRST_CONTINUE_COMPLETE  0x0002
#define FLAG_FIRST_DATA_REPORTED      0x0004
#define FLAG_ALL_DATA_READ            0x0008
#define FLAG_LAST_DATA_REPORTED       0x0010
#define FLAG_RESULT_REPORTED          0x0020
#define FLAG_ERROR                    0x0040
#define FLAG_SYNC_READ                0x0080

HRESULT protocol_start(Protocol*,IInternetProtocol*,IUri*,IInternetProtocolSink*,IInternetBindInfo*) DECLSPEC_HIDDEN;
HRESULT protocol_continue(Protocol*,PROTOCOLDATA*) DECLSPEC_HIDDEN;
HRESULT protocol_read(Protocol*,void*,ULONG,ULONG*) DECLSPEC_HIDDEN;
HRESULT protocol_lock_request(Protocol*) DECLSPEC_HIDDEN;
HRESULT protocol_unlock_request(Protocol*) DECLSPEC_HIDDEN;
HRESULT protocol_abort(Protocol*,HRESULT) DECLSPEC_HIDDEN;
HRESULT protocol_syncbinding(Protocol*) DECLSPEC_HIDDEN;
void protocol_close_connection(Protocol*) DECLSPEC_HIDDEN;

void find_domain_name(const WCHAR*,DWORD,INT*) DECLSPEC_HIDDEN;

typedef struct _task_header_t task_header_t;

typedef struct {
    IInternetProtocolEx   IInternetProtocolEx_iface;
    IInternetBindInfo     IInternetBindInfo_iface;
    IInternetPriority     IInternetPriority_iface;
    IServiceProvider      IServiceProvider_iface;
    IInternetProtocolSink IInternetProtocolSink_iface;
    IWinInetHttpInfo      IWinInetHttpInfo_iface;

    LONG ref;

    IInternetProtocol *protocol;
    IInternetBindInfo *bind_info;
    IInternetProtocolSink *protocol_sink;
    IServiceProvider *service_provider;
    IWinInetInfo *wininet_info;
    IWinInetHttpInfo *wininet_http_info;

    struct {
        IInternetProtocol IInternetProtocol_iface;
        IInternetProtocolSink IInternetProtocolSink_iface;
    } default_protocol_handler;
    IInternetProtocol *protocol_handler;
    IInternetProtocolSink *protocol_sink_handler;

    LONG priority;

    BOOL reported_result;
    BOOL reported_mime;
    BOOL from_urlmon;
    DWORD pi;

    DWORD bscf;
    ULONG progress;
    ULONG progress_max;

    DWORD apartment_thread;
    HWND notif_hwnd;
    DWORD continue_call;

    CRITICAL_SECTION section;
    task_header_t *task_queue_head, *task_queue_tail;

    BYTE *buf;
    DWORD buf_size;
    LPWSTR mime;
    IUri *uri;
    BSTR display_uri;
}  BindProtocol;

HRESULT create_binding_protocol(BOOL,BindProtocol**) DECLSPEC_HIDDEN;
void set_binding_sink(BindProtocol*,IInternetProtocolSink*,IInternetBindInfo*) DECLSPEC_HIDDEN;

typedef struct {
    HWND notif_hwnd;
    DWORD notif_hwnd_cnt;

    struct list entry;
} tls_data_t;

tls_data_t *get_tls_data(void) DECLSPEC_HIDDEN;

#ifndef __REACTOS__
void unregister_notif_wnd_class(void) DECLSPEC_HIDDEN;
#endif
HWND get_notif_hwnd(void) DECLSPEC_HIDDEN;
void release_notif_hwnd(HWND) DECLSPEC_HIDDEN;

const char *debugstr_bindstatus(ULONG) DECLSPEC_HIDDEN;

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline void *heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strndupW(LPCWSTR str, int len)
{
    LPWSTR ret = NULL;

    if(str) {
        ret = heap_alloc((len+1)*sizeof(WCHAR));
        if(ret) {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
    }

    return ret;
}

static inline LPWSTR heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *heap_strdupWtoA(const WCHAR *str)
{
    char *ret = NULL;

    if(str) {
        size_t size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(size);
        if(ret)
            WideCharToMultiByte(CP_ACP, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

#endif /* __WINE_URLMON_MAIN_H */
