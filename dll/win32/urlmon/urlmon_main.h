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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#ifdef __REACTOS__
#include <winnls.h>
#endif
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"

#include "wine/list.h"

extern HINSTANCE hProxyDll;
extern HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT StdURLMoniker_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT HttpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT HttpSProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FtpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT GopherProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT MkProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT MimeFilter_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT Uri_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);

extern BOOL WINAPI URLMON_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern HRESULT WINAPI URLMON_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv);
extern HRESULT WINAPI URLMON_DllRegisterServer(void);
extern HRESULT WINAPI URLMON_DllUnregisterServer(void);

extern GUID const CLSID_PSFactoryBuffer;
extern GUID const CLSID_CUri;

/**********************************************************************
 * Dll lifetime tracking declaration for urlmon.dll
 */
extern LONG URLMON_refCount;
static inline void URLMON_LockModule(void) { InterlockedIncrement( &URLMON_refCount ); }
static inline void URLMON_UnlockModule(void) { InterlockedDecrement( &URLMON_refCount ); }

extern HINSTANCE urlmon_instance;

IInternetProtocolInfo *get_protocol_info(LPCWSTR);
HRESULT get_protocol_handler(IUri*,CLSID*,IClassFactory**);
IInternetProtocol *get_mime_filter(LPCWSTR);
BOOL is_registered_protocol(LPCWSTR);
HRESULT register_namespace(IClassFactory*,REFIID,LPCWSTR,BOOL);
HINTERNET get_internet_session(IInternetBindInfo*);
WCHAR *get_useragent(void);
void update_user_agent(WCHAR*);
void free_session(void);

HRESULT find_mime_from_ext(const WCHAR*,WCHAR**);

HRESULT bind_to_storage(IUri*,IBindCtx*,REFIID,void**);
HRESULT bind_to_object(IMoniker*,IUri*,IBindCtx*,REFIID,void**ppv);

HRESULT create_default_callback(IBindStatusCallback**);
HRESULT wrap_callback(IBindStatusCallback*,IBindStatusCallback**);
IBindStatusCallback *bsc_from_bctx(IBindCtx*);

typedef HRESULT (*stop_cache_binding_proc_t)(void*,const WCHAR*,HRESULT,const WCHAR*);
HRESULT download_to_cache(IUri*,stop_cache_binding_proc_t,void*,IBindStatusCallback*);

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

HRESULT protocol_start(Protocol*,IInternetProtocol*,IUri*,IInternetProtocolSink*,IInternetBindInfo*);
HRESULT protocol_continue(Protocol*,PROTOCOLDATA*);
HRESULT protocol_read(Protocol*,void*,ULONG,ULONG*);
HRESULT protocol_lock_request(Protocol*);
HRESULT protocol_unlock_request(Protocol*);
HRESULT protocol_abort(Protocol*,HRESULT);
HRESULT protocol_syncbinding(Protocol*);
void protocol_close_connection(Protocol*);

void find_domain_name(const WCHAR*,DWORD,INT*);

typedef struct _task_header_t task_header_t;

typedef struct {
    IInternetProtocolEx   IInternetProtocolEx_iface;
    IInternetBindInfo     IInternetBindInfo_iface;
    IInternetPriority     IInternetPriority_iface;
    IServiceProvider      IServiceProvider_iface;
    IInternetProtocolSink IInternetProtocolSink_iface;

    LONG ref;

    IUnknown *protocol_unk;
    IInternetProtocol *protocol;

    IInternetBindInfo *bind_info;
    IInternetProtocolSink *protocol_sink;
    IServiceProvider *service_provider;
    IBindCallbackRedirect *redirect_callback;

    struct {
        IInternetProtocol IInternetProtocol_iface;
        IInternetProtocolSink IInternetProtocolSink_iface;
    } default_protocol_handler;
    IInternetProtocol *protocol_handler;
    IInternetProtocolSink *protocol_sink_handler;

    LONG priority;

    BOOL reported_result;
    BOOL reported_mime;
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

HRESULT create_binding_protocol(BindProtocol**);
void set_binding_sink(BindProtocol*,IInternetProtocolSink*,IInternetBindInfo*);

typedef struct {
    HWND notif_hwnd;
    DWORD notif_hwnd_cnt;

    struct list entry;
} tls_data_t;

tls_data_t *get_tls_data(void);

void unregister_notif_wnd_class(void);
HWND get_notif_hwnd(void);
void release_notif_hwnd(HWND);

const char *debugstr_bindstatus(ULONG);

static inline WCHAR *strndupW(LPCWSTR str, int len)
{
    LPWSTR ret = NULL;

    if(str) {
        ret = malloc((len + 1) * sizeof(WCHAR));
        if(ret) {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
    }

    return ret;
}

static inline WCHAR *strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = malloc(len * sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

#endif /* __WINE_URLMON_MAIN_H */
