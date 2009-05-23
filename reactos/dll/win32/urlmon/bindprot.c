/*
 * Copyright 2007-2009 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct BindProtocol BindProtocol;

struct _task_header_t;

typedef void (*task_proc_t)(BindProtocol*,struct _task_header_t*);

typedef struct _task_header_t {
    task_proc_t proc;
    struct _task_header_t *next;
} task_header_t;

struct BindProtocol {
    const IInternetProtocolVtbl      *lpIInternetProtocolVtbl;
    const IInternetBindInfoVtbl      *lpInternetBindInfoVtbl;
    const IInternetPriorityVtbl      *lpInternetPriorityVtbl;
    const IServiceProviderVtbl       *lpServiceProviderVtbl;
    const IInternetProtocolSinkVtbl  *lpIInternetProtocolSinkVtbl;

    const IInternetProtocolVtbl *lpIInternetProtocolHandlerVtbl;

    LONG ref;

    IInternetProtocol *protocol;
    IInternetProtocol *protocol_handler;
    IInternetBindInfo *bind_info;
    IInternetProtocolSink *protocol_sink;
    IServiceProvider *service_provider;
    IWinInetInfo *wininet_info;

    LONG priority;

    BOOL reported_result;
    BOOL reported_mime;
    BOOL from_urlmon;
    DWORD pi;

    DWORD apartment_thread;
    HWND notif_hwnd;
    DWORD continue_call;

    CRITICAL_SECTION section;
    task_header_t *task_queue_head, *task_queue_tail;

    BYTE *buf;
    DWORD buf_size;
    LPWSTR mime;
    LPWSTR url;
    ProtocolProxy *filter_proxy;
};

#define BINDINFO(x)  ((IInternetBindInfo*) &(x)->lpInternetBindInfoVtbl)
#define PRIORITY(x)  ((IInternetPriority*) &(x)->lpInternetPriorityVtbl)
#define SERVPROV(x)  ((IServiceProvider*)  &(x)->lpServiceProviderVtbl)

#define PROTOCOLHANDLER(x)  ((IInternetProtocol*)  &(x)->lpIInternetProtocolHandlerVtbl)

#define BUFFER_SIZE     2048
#define MIME_TEST_SIZE  255

#define WM_MK_CONTINUE   (WM_USER+101)
#define WM_MK_RELEASE    (WM_USER+102)

static LRESULT WINAPI notif_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_MK_CONTINUE: {
        BindProtocol *This = (BindProtocol*)lParam;
        task_header_t *task;

        while(1) {
            EnterCriticalSection(&This->section);

            task = This->task_queue_head;
            if(task) {
                This->task_queue_head = task->next;
                if(!This->task_queue_head)
                    This->task_queue_tail = NULL;
            }

            LeaveCriticalSection(&This->section);

            if(!task)
                break;

            This->continue_call++;
            task->proc(This, task);
            This->continue_call--;
        }

        IInternetProtocol_Release(PROTOCOL(This));
        return 0;
    }
    case WM_MK_RELEASE: {
        tls_data_t *data = get_tls_data();

        if(!--data->notif_hwnd_cnt) {
            DestroyWindow(hwnd);
            data->notif_hwnd = NULL;
        }
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND get_notif_hwnd(void)
{
    static ATOM wnd_class = 0;
    tls_data_t *tls_data;

    static const WCHAR wszURLMonikerNotificationWindow[] =
        {'U','R','L',' ','M','o','n','i','k','e','r',' ',
         'N','o','t','i','f','i','c','a','t','i','o','n',' ','W','i','n','d','o','w',0};

    tls_data = get_tls_data();
    if(!tls_data)
        return NULL;

    if(tls_data->notif_hwnd_cnt) {
        tls_data->notif_hwnd_cnt++;
        return tls_data->notif_hwnd;
    }

    if(!wnd_class) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass), 0,
            notif_wnd_proc, 0, 0,
            NULL, NULL, NULL, NULL, NULL,
            wszURLMonikerNotificationWindow,
            NULL
        };

        wndclass.hInstance = URLMON_hInstance;

        wnd_class = RegisterClassExW(&wndclass);
        if (!wnd_class && GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            wnd_class = 1;
    }

    tls_data->notif_hwnd = CreateWindowExW(0, wszURLMonikerNotificationWindow,
            wszURLMonikerNotificationWindow, 0, 0, 0, 0, 0, HWND_MESSAGE,
            NULL, URLMON_hInstance, NULL);
    if(tls_data->notif_hwnd)
        tls_data->notif_hwnd_cnt++;

    TRACE("hwnd = %p\n", tls_data->notif_hwnd);

    return tls_data->notif_hwnd;
}

void release_notif_hwnd(HWND hwnd)
{
    tls_data_t *data = get_tls_data();

    if(!data)
        return;

    if(data->notif_hwnd != hwnd) {
        PostMessageW(data->notif_hwnd, WM_MK_RELEASE, 0, 0);
        return;
    }

    if(!--data->notif_hwnd_cnt) {
        DestroyWindow(data->notif_hwnd);
        data->notif_hwnd = NULL;
    }
}

static void push_task(BindProtocol *This, task_header_t *task, task_proc_t proc)
{
    BOOL do_post = FALSE;

    task->proc = proc;
    task->next = NULL;

    EnterCriticalSection(&This->section);

    if(This->task_queue_tail) {
        This->task_queue_tail->next = task;
        This->task_queue_tail = task;
    }else {
        This->task_queue_tail = This->task_queue_head = task;
        do_post = TRUE;
    }

    LeaveCriticalSection(&This->section);

    if(do_post) {
        IInternetProtocol_AddRef(PROTOCOL(This));
        PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);
    }
}

static inline BOOL do_direct_notif(BindProtocol *This)
{
    return !(This->pi & PI_APARTMENTTHREADED) || (This->apartment_thread == GetCurrentThreadId() && !This->continue_call);
}

static HRESULT handle_mime_filter(BindProtocol *This, IInternetProtocol *mime_filter, LPCWSTR mime)
{
    PROTOCOLFILTERDATA filter_data = { sizeof(PROTOCOLFILTERDATA), NULL, NULL, NULL, 0 };
    IInternetProtocolSink *protocol_sink, *old_sink;
    ProtocolProxy *filter_proxy;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(mime_filter, &IID_IInternetProtocolSink, (void**)&protocol_sink);
    if(FAILED(hres))
        return hres;

    hres = create_protocol_proxy(PROTOCOLHANDLER(This), This->protocol_sink, &filter_proxy);
    if(FAILED(hres)) {
        IInternetProtocolSink_Release(protocol_sink);
        return hres;
    }

    old_sink = This->protocol_sink;
    This->protocol_sink = protocol_sink;
    This->filter_proxy = filter_proxy;

    IInternetProtocol_AddRef(mime_filter);
    This->protocol_handler = mime_filter;

    filter_data.pProtocol = PROTOCOL(filter_proxy);
    hres = IInternetProtocol_Start(mime_filter, mime, PROTSINK(filter_proxy), BINDINFO(This),
            PI_FILTER_MODE|PI_FORCE_ASYNC, (HANDLE_PTR)&filter_data);
    if(FAILED(hres)) {
        IInternetProtocolSink_Release(old_sink);
        return hres;
    }

    IInternetProtocolSink_ReportProgress(old_sink, BINDSTATUS_LOADINGMIMEHANDLER, NULL);
    IInternetProtocolSink_Release(old_sink);

    This->pi &= ~PI_MIMEVERIFICATION; /* FIXME: more tests */
    return S_OK;
}

static void mime_available(BindProtocol *This, LPCWSTR mime, BOOL verified)
{
    IInternetProtocol *mime_filter;
    HRESULT hres;

    heap_free(This->mime);
    This->mime = NULL;

    mime_filter = get_mime_filter(mime);
    if(mime_filter) {
        TRACE("Got mime filter for %s\n", debugstr_w(mime));

        hres = handle_mime_filter(This, mime_filter, mime);
        IInternetProtocol_Release(mime_filter);
        if(FAILED(hres))
            FIXME("MIME filter failed: %08x\n", hres);
    }else {
        This->mime = heap_strdupW(mime);

        if(verified || !(This->pi & PI_MIMEVERIFICATION)) {
            This->reported_mime = TRUE;

            if(This->protocol_sink)
                IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        }
    }
}

#define PROTOCOL_THIS(iface) DEFINE_THIS(BindProtocol, IInternetProtocol, iface)

static HRESULT WINAPI BindProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINFO(This);
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = PRIORITY(This);
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        FIXME("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindProtocol_AddRef(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI BindProtocol_Release(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->wininet_info)
            IWinInetInfo_Release(This->wininet_info);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->bind_info)
            IInternetBindInfo_Release(This->bind_info);
        if(This->protocol_handler && This->protocol_handler != PROTOCOLHANDLER(This))
            IInternetProtocol_Release(This->protocol_handler);
        if(This->filter_proxy)
            IInternetProtocol_Release(PROTOCOL(This->filter_proxy));

        set_binding_sink(PROTOCOL(This), NULL);

        if(This->notif_hwnd)
            release_notif_hwnd(This->notif_hwnd);
        DeleteCriticalSection(&This->section);

        heap_free(This->mime);
        heap_free(This->url);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI BindProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    return IInternetProtocol_Start(This->protocol_handler, szUrl, pOIProtSink, pOIBindInfo, grfPI, dwReserved);
}

static HRESULT WINAPI BindProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocol_Continue(This->protocol_handler, pProtocolData);
}

static HRESULT WINAPI BindProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_Terminate(This->protocol_handler, dwOptions);
}

static HRESULT WINAPI BindProtocol_Suspend(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Resume(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(pcbRead)
        *pcbRead = 0;
    return IInternetProtocol_Read(This->protocol_handler, pv, cb, pcbRead);
}

static HRESULT WINAPI BindProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol_handler, dwOptions);
}

static HRESULT WINAPI BindProtocol_UnlockRequest(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return IInternetProtocol_UnlockRequest(This->protocol_handler);
}

void set_binding_sink(IInternetProtocol *bind_protocol, IInternetProtocolSink *sink)
{
    BindProtocol *This = PROTOCOL_THIS(bind_protocol);
    IInternetProtocolSink *prev_sink;
    IServiceProvider *service_provider = NULL;

    if(sink)
        IInternetProtocolSink_AddRef(sink);
    prev_sink = InterlockedExchangePointer((void**)&This->protocol_sink, sink);
    if(prev_sink)
        IInternetProtocolSink_Release(prev_sink);

    if(sink)
        IInternetProtocolSink_QueryInterface(sink, &IID_IServiceProvider, (void**)&service_provider);
    service_provider = InterlockedExchangePointer((void**)&This->service_provider, service_provider);
    if(service_provider)
        IServiceProvider_Release(service_provider);
}

IWinInetInfo *get_wininet_info(IInternetProtocol *bind_protocol)
{
    BindProtocol *This = PROTOCOL_THIS(bind_protocol);

    return This->wininet_info;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl BindProtocolVtbl = {
    BindProtocol_QueryInterface,
    BindProtocol_AddRef,
    BindProtocol_Release,
    BindProtocol_Start,
    BindProtocol_Continue,
    BindProtocol_Abort,
    BindProtocol_Terminate,
    BindProtocol_Suspend,
    BindProtocol_Resume,
    BindProtocol_Read,
    BindProtocol_Seek,
    BindProtocol_LockRequest,
    BindProtocol_UnlockRequest
};

#define PROTOCOLHANDLER_THIS(iface) DEFINE_THIS(BindProtocol, IInternetProtocolHandler, iface)

static HRESULT WINAPI ProtocolHandler_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    ERR("should not be called\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolHandler_AddRef(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI ProtocolHandler_Release(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI ProtocolHandler_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    IInternetProtocol *protocol = NULL;
    IInternetPriority *priority;
    IServiceProvider *service_provider;
    BOOL urlmon_protocol = FALSE;
    CLSID clsid = IID_NULL;
    LPOLESTR clsid_str;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    if(!szUrl || !pOIProtSink || !pOIBindInfo)
        return E_INVALIDARG;

    This->pi = grfPI;
    This->url = heap_strdupW(szUrl);

    hres = IInternetProtocolSink_QueryInterface(pOIProtSink, &IID_IServiceProvider,
                                                (void**)&service_provider);
    if(SUCCEEDED(hres)) {
        /* FIXME: What's protocol CLSID here? */
        IServiceProvider_QueryService(service_provider, &IID_IInternetProtocol,
                                      &IID_IInternetProtocol, (void**)&protocol);
        IServiceProvider_Release(service_provider);
    }

    if(!protocol) {
        IClassFactory *cf;
        IUnknown *unk;

        hres = get_protocol_handler(szUrl, &clsid, &urlmon_protocol, &cf);
        if(FAILED(hres))
            return hres;

        if(This->from_urlmon) {
            hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&protocol);
            IClassFactory_Release(cf);
            if(FAILED(hres))
                return hres;
        }else {
            hres = IClassFactory_CreateInstance(cf, (IUnknown*)BINDINFO(This),
                    &IID_IUnknown, (void**)&unk);
            IClassFactory_Release(cf);
            if(FAILED(hres))
                return hres;

            hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocol, (void**)&protocol);
            IUnknown_Release(unk);
            if(FAILED(hres))
                return hres;
        }
    }

    StringFromCLSID(&clsid, &clsid_str);
    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_PROTOCOLCLASSID, clsid_str);
    CoTaskMemFree(clsid_str);

    This->protocol = protocol;

    if(urlmon_protocol)
        IInternetProtocol_QueryInterface(protocol, &IID_IWinInetInfo, (void**)&This->wininet_info);

    IInternetBindInfo_AddRef(pOIBindInfo);
    This->bind_info = pOIBindInfo;

    set_binding_sink(PROTOCOL(This), pOIProtSink);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    if(SUCCEEDED(hres)) {
        IInternetPriority_SetPriority(priority, This->priority);
        IInternetPriority_Release(priority);
    }

    return IInternetProtocol_Start(protocol, szUrl, PROTSINK(This), BINDINFO(This), 0, 0);
}

static HRESULT WINAPI ProtocolHandler_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocol_Continue(This->protocol, pProtocolData);
}

static HRESULT WINAPI ProtocolHandler_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    if(!This->reported_result)
        return E_FAIL;

    IInternetProtocol_Terminate(This->protocol, 0);

    if(This->filter_proxy) {
        IInternetProtocol_Release(PROTOCOL(This->filter_proxy));
        This->filter_proxy = NULL;
    }

    set_binding_sink(PROTOCOL(This), NULL);

    if(This->bind_info) {
        IInternetBindInfo_Release(This->bind_info);
        This->bind_info = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolHandler_Suspend(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Resume(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    ULONG read = 0;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(This->buf) {
        read = min(cb, This->buf_size);
        memcpy(pv, This->buf, read);

        if(read == This->buf_size) {
            heap_free(This->buf);
            This->buf = NULL;
        }else {
            memmove(This->buf, This->buf+cb, This->buf_size-cb);
        }

        This->buf_size -= read;
    }

    if(read < cb) {
        ULONG cread = 0;

        hres = IInternetProtocol_Read(This->protocol, (BYTE*)pv+read, cb-read, &cread);
        read += cread;
    }

    *pcbRead = read;
    return hres;
}

static HRESULT WINAPI ProtocolHandler_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol, dwOptions);
}

static HRESULT WINAPI ProtocolHandler_UnlockRequest(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOLHANDLER_THIS(iface);

    TRACE("(%p)\n", This);

    return IInternetProtocol_UnlockRequest(This->protocol);
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl InternetProtocolHandlerVtbl = {
    ProtocolHandler_QueryInterface,
    ProtocolHandler_AddRef,
    ProtocolHandler_Release,
    ProtocolHandler_Start,
    ProtocolHandler_Continue,
    ProtocolHandler_Abort,
    ProtocolHandler_Terminate,
    ProtocolHandler_Suspend,
    ProtocolHandler_Resume,
    ProtocolHandler_Read,
    ProtocolHandler_Seek,
    ProtocolHandler_LockRequest,
    ProtocolHandler_UnlockRequest
};

#define BINDINFO_THIS(iface) DEFINE_THIS(BindProtocol, InternetBindInfo, iface)

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IBinding_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IBinding_Release(PROTOCOL(This));
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    hres = IInternetBindInfo_GetBindInfo(This->bind_info, grfBINDF, pbindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return hres;
    }

    *grfBINDF |= BINDF_FROMURLMON;
    return hres;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BindProtocol *This = BINDINFO_THIS(iface);

    TRACE("(%p)->(%d %p %d %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    return IInternetBindInfo_GetBindString(This->bind_info, ulStringType, ppwzStr, cEl, pcElFetched);
}

#undef BINDFO_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

#define PRIORITY_THIS(iface) DEFINE_THIS(BindProtocol, InternetPriority, iface)

static HRESULT WINAPI InternetPriority_QueryInterface(IInternetPriority *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI InternetPriority_AddRef(IInternetPriority *iface)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI InternetPriority_Release(IInternetPriority *iface)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI InternetPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    BindProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI InternetPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    BindProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

#undef PRIORITY_THIS

static const IInternetPriorityVtbl InternetPriorityVtbl = {
    InternetPriority_QueryInterface,
    InternetPriority_AddRef,
    InternetPriority_Release,
    InternetPriority_SetPriority,
    InternetPriority_GetPriority

};

#define PROTSINK_THIS(iface) DEFINE_THIS(BindProtocol, IInternetProtocolSink, iface)

static HRESULT WINAPI BPInternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BPInternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BPInternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

typedef struct {
    task_header_t header;
    PROTOCOLDATA data;
} switch_task_t;

static void switch_proc(BindProtocol *bind, task_header_t *t)
{
    switch_task_t *task = (switch_task_t*)t;

    IInternetProtocol_Continue(bind->protocol_handler, &task->data);

    heap_free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    TRACE("flags %x state %x data %p cb %u\n", pProtocolData->grfFlags, pProtocolData->dwState,
          pProtocolData->pData, pProtocolData->cbData);

    if(!do_direct_notif(This)) {
        switch_task_t *task;

        task = heap_alloc(sizeof(switch_task_t));
        if(!task)
            return E_OUTOFMEMORY;

        task->data = *pProtocolData;

        push_task(This, &task->header, switch_proc);
        return S_OK;
    }

    if(!This->protocol_sink) {
        IInternetProtocol_Continue(This->protocol_handler, pProtocolData);
        return S_OK;
    }

    return IInternetProtocolSink_Switch(This->protocol_sink, pProtocolData);
}

static void report_progress(BindProtocol *This, ULONG status_code, LPCWSTR status_text)
{
    switch(status_code) {
    case BINDSTATUS_FINDINGRESOURCE:
    case BINDSTATUS_CONNECTING:
    case BINDSTATUS_BEGINDOWNLOADDATA:
    case BINDSTATUS_SENDINGREQUEST:
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
    case BINDSTATUS_DIRECTBIND:
    case BINDSTATUS_ACCEPTRANGES:
        if(This->protocol_sink)
            IInternetProtocolSink_ReportProgress(This->protocol_sink, status_code, status_text);
        break;

    case BINDSTATUS_MIMETYPEAVAILABLE:
        mime_available(This, status_text, FALSE);
        break;

    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        mime_available(This, status_text, TRUE);
        break;

    default:
        FIXME("unsupported ulStatusCode %u\n", status_code);
    }
}

typedef struct {
    task_header_t header;

    ULONG status_code;
    LPWSTR status_text;
} on_progress_task_t;

static void on_progress_proc(BindProtocol *This, task_header_t *t)
{
    on_progress_task_t *task = (on_progress_task_t*)t;

    report_progress(This, task->status_code, task->status_text);

    heap_free(task->status_text);
    heap_free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    if(do_direct_notif(This)) {
        report_progress(This, ulStatusCode, szStatusText);
    }else {
        on_progress_task_t *task;

        task = heap_alloc(sizeof(on_progress_task_t));

        task->status_code = ulStatusCode;
        task->status_text = heap_strdupW(szStatusText);

        push_task(This, &task->header, on_progress_proc);
    }

    return S_OK;
}

static HRESULT report_data(BindProtocol *This, DWORD bscf, ULONG progress, ULONG progress_max)
{
    if(!This->protocol_sink)
        return S_OK;

    if((This->pi & PI_MIMEVERIFICATION) && !This->reported_mime) {
        DWORD read = 0;
        LPWSTR mime;
        HRESULT hres;

        if(!This->buf) {
            This->buf = heap_alloc(BUFFER_SIZE);
            if(!This->buf)
                return E_OUTOFMEMORY;
        }

        do {
            read = 0;
            hres = IInternetProtocol_Read(This->protocol, This->buf+This->buf_size,
                    BUFFER_SIZE-This->buf_size, &read);
            if(FAILED(hres) && hres != E_PENDING)
                return hres;
            This->buf_size += read;
        }while(This->buf_size < MIME_TEST_SIZE && hres == S_OK);

        if(This->buf_size < MIME_TEST_SIZE && hres != S_FALSE)
            return S_OK;

        hres = FindMimeFromData(NULL, This->url, This->buf, min(This->buf_size, MIME_TEST_SIZE),
                This->mime, 0, &mime, 0);
        if(FAILED(hres))
            return hres;

        mime_available(This, mime, TRUE);
        CoTaskMemFree(mime);
    }

    return IInternetProtocolSink_ReportData(This->protocol_sink, bscf, progress, progress_max);
}

typedef struct {
    task_header_t header;
    DWORD bscf;
    ULONG progress;
    ULONG progress_max;
} report_data_task_t;

static void report_data_proc(BindProtocol *This, task_header_t *t)
{
    report_data_task_t *task = (report_data_task_t*)t;

    report_data(This, task->bscf, task->progress, task->progress_max);
    heap_free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);

    if(!This->protocol_sink)
        return S_OK;

    if(!do_direct_notif(This)) {
        report_data_task_t *task;

        task = heap_alloc(sizeof(report_data_task_t));
        if(!task)
            return E_OUTOFMEMORY;

        task->bscf = grfBSCF;
        task->progress = ulProgress;
        task->progress_max = ulProgressMax;

        push_task(This, &task->header, report_data_proc);
        return S_OK;
    }

    return report_data(This, grfBSCF, ulProgress, ulProgressMax);
}

typedef struct {
    task_header_t header;

    HRESULT hres;
    DWORD err;
    LPWSTR str;
} report_result_task_t;

static void report_result_proc(BindProtocol *This, task_header_t *t)
{
    report_result_task_t *task = (report_result_task_t*)t;

    if(This->protocol_sink)
        IInternetProtocolSink_ReportResult(This->protocol_sink, task->hres, task->err, task->str);

    heap_free(task->str);
    heap_free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    if(!This->protocol_sink)
        return E_FAIL;

    This->reported_result = TRUE;

    if(!do_direct_notif(This)) {
        report_result_task_t *task;

        task = heap_alloc(sizeof(report_result_task_t));
        if(!task)
            return E_OUTOFMEMORY;

        task->hres = hrResult;
        task->err = dwError;
        task->str = heap_strdupW(szResult);

        push_task(This, &task->header, report_result_proc);
        return S_OK;
    }

    return IInternetProtocolSink_ReportResult(This->protocol_sink, hrResult, dwError, szResult);
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    BPInternetProtocolSink_QueryInterface,
    BPInternetProtocolSink_AddRef,
    BPInternetProtocolSink_Release,
    BPInternetProtocolSink_Switch,
    BPInternetProtocolSink_ReportProgress,
    BPInternetProtocolSink_ReportData,
    BPInternetProtocolSink_ReportResult
};

#define SERVPROV_THIS(iface) DEFINE_THIS(BindProtocol, ServiceProvider, iface)

static HRESULT WINAPI BPServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BPServiceProvider_AddRef(IServiceProvider *iface)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BPServiceProvider_Release(IServiceProvider *iface)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI BPServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BindProtocol *This = SERVPROV_THIS(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->service_provider)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(This->service_provider, guidService, riid, ppv);
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BPServiceProvider_QueryInterface,
    BPServiceProvider_AddRef,
    BPServiceProvider_Release,
    BPServiceProvider_QueryService
};

HRESULT create_binding_protocol(LPCWSTR url, BOOL from_urlmon, IInternetProtocol **protocol)
{
    BindProtocol *ret = heap_alloc_zero(sizeof(BindProtocol));

    ret->lpIInternetProtocolVtbl        = &BindProtocolVtbl;
    ret->lpInternetBindInfoVtbl         = &InternetBindInfoVtbl;
    ret->lpInternetPriorityVtbl         = &InternetPriorityVtbl;
    ret->lpServiceProviderVtbl          = &ServiceProviderVtbl;
    ret->lpIInternetProtocolSinkVtbl    = &InternetProtocolSinkVtbl;
    ret->lpIInternetProtocolHandlerVtbl = &InternetProtocolHandlerVtbl;

    ret->ref = 1;
    ret->from_urlmon = from_urlmon;
    ret->apartment_thread = GetCurrentThreadId();
    ret->notif_hwnd = get_notif_hwnd();
    ret->protocol_handler = PROTOCOLHANDLER(ret);
    InitializeCriticalSection(&ret->section);

    URLMON_LockModule();

    *protocol = PROTOCOL(ret);
    return S_OK;
}
