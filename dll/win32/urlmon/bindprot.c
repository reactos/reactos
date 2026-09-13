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

typedef void (*task_proc_t)(BindProtocol*,task_header_t*);

struct _task_header_t {
    task_proc_t proc;
    task_header_t *next;
};

#define BUFFER_SIZE     2048
#define MIME_TEST_SIZE  255

#define WM_MK_CONTINUE   (WM_USER+101)
#define WM_MK_RELEASE    (WM_USER+102)

static void process_tasks(BindProtocol *This)
{
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
}

static LRESULT WINAPI notif_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_MK_CONTINUE: {
        BindProtocol *This = (BindProtocol*)lParam;

        process_tasks(This);

        IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
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

static ATOM notif_wnd_class;

static BOOL WINAPI register_notif_wnd_class(INIT_ONCE *once, void *param, void **context)
{
    static WNDCLASSEXW wndclass = {
        sizeof(wndclass), 0, notif_wnd_proc, 0, 0,
        NULL, NULL, NULL, NULL, NULL,
        L"URL Moniker Notification Window", NULL
    };

    wndclass.hInstance = hProxyDll;
    notif_wnd_class = RegisterClassExW(&wndclass);
    return TRUE;
}

void unregister_notif_wnd_class(void)
{
    if(notif_wnd_class)
        UnregisterClassW(MAKEINTRESOURCEW(notif_wnd_class), hProxyDll);
}

HWND get_notif_hwnd(void)
{
    tls_data_t *tls_data;

    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

    tls_data = get_tls_data();
    if(!tls_data)
        return NULL;

    if(tls_data->notif_hwnd_cnt) {
        tls_data->notif_hwnd_cnt++;
        return tls_data->notif_hwnd;
    }

    InitOnceExecuteOnce(&init_once, register_notif_wnd_class, NULL, NULL);
    if(!notif_wnd_class)
        return NULL;

    tls_data->notif_hwnd = CreateWindowExW(0, MAKEINTRESOURCEW(notif_wnd_class),
            L"URL Moniker Notification Window", 0, 0, 0, 0, 0, HWND_MESSAGE,
            NULL, hProxyDll, NULL);
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
        do_post = !This->continue_call;
    }

    LeaveCriticalSection(&This->section);

    if(do_post) {
        IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
        PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);
    }
}

static inline BOOL is_apartment_thread(BindProtocol *This)
{
    return This->apartment_thread == GetCurrentThreadId();
}

static inline BOOL do_direct_notif(BindProtocol *This)
{
    return !(This->pi & PI_APARTMENTTHREADED) || (is_apartment_thread(This) && !This->continue_call);
}

static HRESULT handle_mime_filter(BindProtocol *This, IInternetProtocol *mime_filter)
{
    PROTOCOLFILTERDATA filter_data = { sizeof(PROTOCOLFILTERDATA), NULL, NULL, NULL, 0 };
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(mime_filter, &IID_IInternetProtocolSink, (void**)&This->protocol_sink_handler);
    if(FAILED(hres)) {
        This->protocol_sink_handler = &This->default_protocol_handler.IInternetProtocolSink_iface;
        return hres;
    }

    IInternetProtocol_AddRef(mime_filter);
    This->protocol_handler = mime_filter;

    filter_data.pProtocol = &This->default_protocol_handler.IInternetProtocol_iface;
    hres = IInternetProtocol_Start(mime_filter, This->mime, &This->default_protocol_handler.IInternetProtocolSink_iface,
            &This->IInternetBindInfo_iface, PI_FILTER_MODE|PI_FORCE_ASYNC,
            (HANDLE_PTR)&filter_data);
    if(FAILED(hres)) {
        IInternetProtocolSink_Release(This->protocol_sink_handler);
        IInternetProtocol_Release(This->protocol_handler);
        This->protocol_sink_handler = &This->default_protocol_handler.IInternetProtocolSink_iface;
        This->protocol_handler = &This->default_protocol_handler.IInternetProtocol_iface;
        return hres;
    }

    /* NOTE: IE9 calls it on the new protocol_sink. It doesn't make sense so it seems to be a bug there. */
    IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_LOADINGMIMEHANDLER, NULL);

    return S_OK;
}

static void mime_available(BindProtocol *This, LPCWSTR mime, BOOL verified)
{
    IInternetProtocol *mime_filter;
    HRESULT hres;

    free(This->mime);
    This->mime = wcsdup(mime);

    if(This->protocol_handler==&This->default_protocol_handler.IInternetProtocol_iface
            && (mime_filter = get_mime_filter(mime))) {
        TRACE("Got mime filter for %s\n", debugstr_w(mime));

        hres = handle_mime_filter(This, mime_filter);
        IInternetProtocol_Release(mime_filter);
        if(FAILED(hres))
            FIXME("MIME filter failed: %08lx\n", hres);
    }

    if(This->reported_mime || verified || !(This->pi & PI_MIMEVERIFICATION)) {
        This->reported_mime = TRUE;
        IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
    }
}

static inline BindProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, IInternetProtocolEx_iface);
}

static HRESULT WINAPI BindProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolEx %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = &This->IInternetBindInfo_iface;
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = &This->IInternetPriority_iface;
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        FIXME("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolSink_iface;
    }else if(This->protocol_unk) {
        HRESULT hres;
        hres = IUnknown_QueryInterface(This->protocol_unk, riid, ppv);
        TRACE("(%p) aggregated handler returned %08lx for %s\n", This, hres, debugstr_guid(riid));
        return hres;
    }else {
        WARN("not supported interface %s\n", debugstr_guid(riid));
    }

    if(!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BindProtocol_AddRef(IInternetProtocolEx *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static void release_protocol_handler(BindProtocol *This)
{
    if(This->protocol) {
        IInternetProtocol_Release(This->protocol);
        This->protocol = NULL;
    }
    if(This->protocol_handler && This->protocol_handler != &This->default_protocol_handler.IInternetProtocol_iface) {
        IInternetProtocol_Release(This->protocol_handler);
        This->protocol_handler = &This->default_protocol_handler.IInternetProtocol_iface;
    }
    if(This->protocol_sink_handler &&
       This->protocol_sink_handler != &This->default_protocol_handler.IInternetProtocolSink_iface) {
        IInternetProtocolSink_Release(This->protocol_sink_handler);
        This->protocol_sink_handler = &This->default_protocol_handler.IInternetProtocolSink_iface;
    }
    if(This->protocol_unk) {
        IUnknown_Release(This->protocol_unk);
        This->protocol_unk = NULL;
    }
}

static ULONG WINAPI BindProtocol_Release(IInternetProtocolEx *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_protocol_handler(This);
        if(This->redirect_callback)
            IBindCallbackRedirect_Release(This->redirect_callback);
        if(This->bind_info)
            IInternetBindInfo_Release(This->bind_info);
        if(This->uri)
            IUri_Release(This->uri);
        SysFreeString(This->display_uri);

        set_binding_sink(This, NULL, NULL);

        if(This->notif_hwnd)
            release_notif_hwnd(This->notif_hwnd);
        This->section.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->section);

        free(This->mime);
        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI BindProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, Uri_CREATE_FILE_USE_DOS_PATH, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = IInternetProtocolEx_StartEx(&This->IInternetProtocolEx_iface, uri, pOIProtSink,
            pOIBindInfo, grfPI, (HANDLE*)dwReserved);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI BindProtocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocol_Continue(This->protocol_handler, pProtocolData);
}

static HRESULT WINAPI BindProtocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);

    return IInternetProtocol_Abort(This->protocol_handler, hrReason, dwOptions);
}

static HRESULT WINAPI BindProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return IInternetProtocol_Terminate(This->protocol_handler, dwOptions);
}

static HRESULT WINAPI BindProtocol_Suspend(IInternetProtocolEx *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Resume(IInternetProtocolEx *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if(pcbRead)
        *pcbRead = 0;
    return IInternetProtocol_Read(This->protocol_handler, pv, cb, pcbRead);
}

static HRESULT WINAPI BindProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol_handler, dwOptions);
}

static HRESULT WINAPI BindProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)\n", This);

    return IInternetProtocol_UnlockRequest(This->protocol_handler);
}

static HRESULT WINAPI BindProtocol_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    BindProtocol *This = impl_from_IInternetProtocolEx(iface);
    IInternetProtocol *protocol = NULL;
    IInternetProtocolEx *protocolex;
    IInternetPriority *priority;
    IServiceProvider *service_provider;
    CLSID clsid = IID_NULL;
    IUnknown *protocol_unk = NULL;
    LPOLESTR clsid_str;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %08lx %p)\n", This, pUri, pOIProtSink, pOIBindInfo, grfPI, dwReserved);

    if(!pUri || !pOIProtSink || !pOIBindInfo)
        return E_INVALIDARG;

    This->pi = grfPI;

    if(This->uri) {
        SysFreeString(This->display_uri);
        IUri_Release(This->uri);
    }
    IUri_AddRef(pUri);
    This->uri = pUri;

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

        hres = get_protocol_handler(pUri, &clsid, &cf);
        if(FAILED(hres))
            return hres;

        hres = IClassFactory_CreateInstance(cf, (IUnknown*)&This->IInternetBindInfo_iface,
                &IID_IUnknown, (void**)&protocol_unk);
        if(SUCCEEDED(hres)) {
            hres = IUnknown_QueryInterface(protocol_unk, &IID_IInternetProtocol, (void**)&protocol);
            if(SUCCEEDED(hres))
                This->protocol_unk = protocol_unk;
            else
                IUnknown_Release(protocol_unk);
        }
        else if(hres == CLASS_E_NOAGGREGATION)
            hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&protocol);

        IClassFactory_Release(cf);
        if(FAILED(hres))
            return hres;
    }

    StringFromCLSID(&clsid, &clsid_str);
    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_PROTOCOLCLASSID, clsid_str);
    CoTaskMemFree(clsid_str);

    This->protocol_unk = protocol_unk;
    This->protocol = protocol;

    if(!protocol_unk)
        protocol_unk = (IUnknown*)protocol;

    set_binding_sink(This, pOIProtSink, pOIBindInfo);

    hres = IUnknown_QueryInterface(protocol_unk, &IID_IInternetPriority, (void**)&priority);
    if(SUCCEEDED(hres)) {
        IInternetPriority_SetPriority(priority, This->priority);
        IInternetPriority_Release(priority);
    }

    hres = IUnknown_QueryInterface(protocol_unk, &IID_IInternetProtocolEx, (void**)&protocolex);
    if(SUCCEEDED(hres)) {
        hres = IInternetProtocolEx_StartEx(protocolex, pUri, &This->IInternetProtocolSink_iface,
                &This->IInternetBindInfo_iface, 0, NULL);
        IInternetProtocolEx_Release(protocolex);
    }else {
        hres = IUri_GetDisplayUri(pUri, &This->display_uri);
        if(FAILED(hres))
            return hres;

        hres = IInternetProtocol_Start(protocol, This->display_uri, &This->IInternetProtocolSink_iface,
                &This->IInternetBindInfo_iface, 0, 0);
    }

    if(SUCCEEDED(hres))
        process_tasks(This);
    return hres;
}

void set_binding_sink(BindProtocol *This, IInternetProtocolSink *sink, IInternetBindInfo *bind_info)
{
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

    if(bind_info)
        IInternetBindInfo_AddRef(bind_info);
    bind_info = InterlockedExchangePointer((void**)&This->bind_info, bind_info);
    if(bind_info)
        IInternetBindInfo_Release(bind_info);
}

static const IInternetProtocolExVtbl BindProtocolVtbl = {
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
    BindProtocol_UnlockRequest,
    BindProtocol_StartEx
};

static inline BindProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, default_protocol_handler.IInternetProtocol_iface);
}

static HRESULT WINAPI ProtocolHandler_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->default_protocol_handler.IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->default_protocol_handler.IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->default_protocol_handler.IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = &This->default_protocol_handler.IInternetProtocolSink_iface;
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolHandler_AddRef(IInternetProtocol *iface)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI ProtocolHandler_Release(IInternetProtocol *iface)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI ProtocolHandler_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    ERR("Should not be called\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    IInternetProtocol *protocol = NULL;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    /* FIXME: This should not be needed. */
    if(!This->protocol) {
        if(!This->protocol_unk)
            return E_FAIL;
        hres = IUnknown_QueryInterface(This->protocol_unk, &IID_IInternetProtocol, (void**)&protocol);
        if(FAILED(hres))
            return E_FAIL;
    }

    hres = IInternetProtocol_Continue(protocol ? protocol : This->protocol, pProtocolData);

    free(pProtocolData);
    if(protocol)
        IInternetProtocol_Release(protocol);
    return hres;
}

static HRESULT WINAPI ProtocolHandler_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);

    if(This->protocol && !This->reported_result)
        return IInternetProtocol_Abort(This->protocol, hrReason, dwOptions);

    return S_OK;
}

static HRESULT WINAPI ProtocolHandler_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    if(!This->reported_result)
        return E_FAIL;

    /* This may get released in Terminate call. */
    IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);

    if(This->protocol) {
        IInternetProtocol_Terminate(This->protocol, 0);

        if (This->protocol_unk) {
            IInternetProtocol_Release(This->protocol);
            This->protocol = NULL;
        }
    }

    set_binding_sink(This, NULL, NULL);

    if(This->bind_info) {
        IInternetBindInfo_Release(This->bind_info);
        This->bind_info = NULL;
    }

    if(This->redirect_callback) {
        IBindCallbackRedirect_Release(This->redirect_callback);
        This->redirect_callback = NULL;
    }

    IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
    return S_OK;
}

static HRESULT WINAPI ProtocolHandler_Suspend(IInternetProtocol *iface)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Resume(IInternetProtocol *iface)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    ULONG read = 0;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if(This->buf_size) {
        read = min(cb, This->buf_size);
        memcpy(pv, This->buf, read);

        if(read == This->buf_size) {
            free(This->buf);
            This->buf = NULL;
        }else {
            memmove(This->buf, This->buf+cb, This->buf_size-cb);
        }

        This->buf_size -= read;
    }

    if(read < cb) {
        IInternetProtocol *protocol;
        ULONG cread = 0;

        /* FIXME: We shouldn't need it, but out binding code currently depends on it. */
        if(!This->protocol && This->protocol_unk) {
            hres = IUnknown_QueryInterface(This->protocol_unk, &IID_IInternetProtocol,
                                           (void**)&protocol);
            if(FAILED(hres))
                return E_ABORT;
        }else {
            protocol = This->protocol;
        }

        if(is_apartment_thread(This))
            This->continue_call++;
        hres = IInternetProtocol_Read(protocol, (BYTE*)pv+read, cb-read, &cread);
        if(is_apartment_thread(This))
            This->continue_call--;
        read += cread;

        if(!This->protocol)
            IInternetProtocol_Release(protocol);
    }

    *pcbRead = read;
    return hres;
}

static HRESULT WINAPI ProtocolHandler_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolHandler_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol, dwOptions);
}

static HRESULT WINAPI ProtocolHandler_UnlockRequest(IInternetProtocol *iface)
{
    BindProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)\n", This);

    if (This->protocol)
        return IInternetProtocol_UnlockRequest(This->protocol);
    return S_OK;
}

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

static inline BindProtocol *impl_from_IInternetProtocolSinkHandler(IInternetProtocolSink *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, default_protocol_handler.IInternetProtocolSink_iface);
}

static HRESULT WINAPI ProtocolSinkHandler_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppvObject)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);
    return IInternetProtocol_QueryInterface(&This->default_protocol_handler.IInternetProtocol_iface,
            riid, ppvObject);
}

static ULONG WINAPI ProtocolSinkHandler_AddRef(IInternetProtocolSink *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI ProtocolSinkHandler_Release(IInternetProtocolSink *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI ProtocolSinkHandler_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    if(!This->protocol_sink) {
        IInternetProtocol_Continue(This->protocol_handler, pProtocolData);
        return S_OK;
    }

    return IInternetProtocolSink_Switch(This->protocol_sink, pProtocolData);
}

static HRESULT WINAPI ProtocolSinkHandler_ReportProgress(IInternetProtocolSink *iface,
        ULONG status_code, LPCWSTR status_text)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);

    TRACE("(%p)->(%s %s)\n", This, debugstr_bindstatus(status_code), debugstr_w(status_text));

    if(!This->protocol_sink)
        return S_OK;

    switch(status_code) {
    case BINDSTATUS_FINDINGRESOURCE:
    case BINDSTATUS_CONNECTING:
    case BINDSTATUS_REDIRECTING:
    case BINDSTATUS_SENDINGREQUEST:
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
    case BINDSTATUS_DIRECTBIND:
    case BINDSTATUS_ACCEPTRANGES:
    case BINDSTATUS_DECODING:
        IInternetProtocolSink_ReportProgress(This->protocol_sink, status_code, status_text);
        break;

    case BINDSTATUS_BEGINDOWNLOADDATA:
        IInternetProtocolSink_ReportData(This->protocol_sink, This->bscf, This->progress, This->progress_max);
        break;

    case BINDSTATUS_MIMETYPEAVAILABLE:
        mime_available(This, status_text, FALSE);
        break;

    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        mime_available(This, status_text, TRUE);
        break;

    default:
        FIXME("unsupported ulStatusCode %lu\n", status_code);
    }

    return S_OK;
}

static HRESULT report_data(BindProtocol *This)
{
    DWORD bscf = This->bscf;
    HRESULT hres;

    if((This->pi & PI_MIMEVERIFICATION) && !This->reported_mime) {
        BYTE buf[BUFFER_SIZE];
        DWORD read = 0;
        LPWSTR mime;

        do {
            read = 0;
            hres = IInternetProtocol_Read(This->protocol, buf,
                    sizeof(buf)-This->buf_size, &read);
            if(FAILED(hres) && hres != E_PENDING)
                return hres;

            if(!This->buf) {
                This->buf = malloc(BUFFER_SIZE);
                if(!This->buf)
                    return E_OUTOFMEMORY;
            }else if(read + This->buf_size > BUFFER_SIZE) {
                BYTE *tmp;

                tmp = realloc(This->buf, read + This->buf_size);
                if(!tmp)
                    return E_OUTOFMEMORY;
                This->buf = tmp;
            }

            memcpy(This->buf+This->buf_size, buf, read);
            This->buf_size += read;
        }while(This->buf_size < MIME_TEST_SIZE && hres == S_OK);

        if(This->buf_size < MIME_TEST_SIZE && hres != S_FALSE)
            return S_OK;

        bscf = BSCF_FIRSTDATANOTIFICATION;
        if(hres == S_FALSE)
            bscf |= BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE;

        if(!This->reported_mime) {
            BSTR raw_uri;

            hres = IUri_GetRawUri(This->uri, &raw_uri);
            if(FAILED(hres))
                return hres;

            hres = FindMimeFromData(NULL, raw_uri, This->buf, min(This->buf_size, MIME_TEST_SIZE),
                    This->mime, 0, &mime, 0);
            SysFreeString(raw_uri);
            if(FAILED(hres))
                return hres;

            free(This->mime);
            This->mime = wcsdup(mime);
            CoTaskMemFree(mime);
            This->reported_mime = TRUE;
            if(This->protocol_sink)
                IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_MIMETYPEAVAILABLE, This->mime);
        }
    }

    if(!This->protocol_sink)
        return S_OK;

    return IInternetProtocolSink_ReportData(This->protocol_sink, bscf, This->progress, This->progress_max);
}

static HRESULT WINAPI ProtocolSinkHandler_ReportData(IInternetProtocolSink *iface,
        DWORD bscf, ULONG progress, ULONG progress_max)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);
    HRESULT hres;

    TRACE("(%p)->(%lx %lu %lu)\n", This, bscf, progress, progress_max);

    This->bscf = bscf;
    This->progress = progress;
    This->progress_max = progress_max;

    if(!This->protocol_sink)
        return S_OK;

    if(is_apartment_thread(This))
        This->continue_call++;

    hres = report_data(This);

    if(is_apartment_thread(This)) {
        This->continue_call--;
        process_tasks(This);
    }
    return hres;
}

static HRESULT handle_redirect(BindProtocol *This, const WCHAR *url)
{
    HRESULT hres;

    if(This->redirect_callback) {
        VARIANT_BOOL cancel = VARIANT_FALSE;
        IBindCallbackRedirect_Redirect(This->redirect_callback, url, &cancel);
        if(cancel)
            return INET_E_REDIRECT_FAILED;
    }

    if(This->protocol_sink) {
        hres = IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_REDIRECTING, url);
        if(FAILED(hres))
            return hres;
    }

    IInternetProtocol_Terminate(This->protocol, 0); /* should this be done in StartEx? */
    release_protocol_handler(This);

    return IInternetProtocolEx_Start(&This->IInternetProtocolEx_iface, url, This->protocol_sink, This->bind_info, This->pi, 0);
}

static HRESULT WINAPI ProtocolSinkHandler_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    BindProtocol *This = impl_from_IInternetProtocolSinkHandler(iface);

    TRACE("(%p)->(%08lx %ld %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    if(hrResult == INET_E_REDIRECT_FAILED) {
        hrResult = handle_redirect(This, szResult);
        if(hrResult == S_OK)
            return S_OK;
        szResult = NULL;
    }

    if(This->protocol_sink)
        return IInternetProtocolSink_ReportResult(This->protocol_sink, hrResult, dwError, szResult);
    return S_OK;
}

static const IInternetProtocolSinkVtbl InternetProtocolSinkHandlerVtbl = {
    ProtocolSinkHandler_QueryInterface,
    ProtocolSinkHandler_AddRef,
    ProtocolSinkHandler_Release,
    ProtocolSinkHandler_Switch,
    ProtocolSinkHandler_ReportProgress,
    ProtocolSinkHandler_ReportData,
    ProtocolSinkHandler_ReportResult
};

static inline BindProtocol *impl_from_IInternetBindInfo(IInternetBindInfo *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, IInternetBindInfo_iface);
}

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IInternetBindInfo(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    BindProtocol *This = impl_from_IInternetBindInfo(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    BindProtocol *This = impl_from_IInternetBindInfo(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindProtocol *This = impl_from_IInternetBindInfo(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    hres = IInternetBindInfo_GetBindInfo(This->bind_info, grfBINDF, pbindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        return hres;
    }

    if((pbindinfo->dwOptions & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS) && !This->redirect_callback) {
        IServiceProvider *service_provider;

        hres = IInternetProtocolSink_QueryInterface(This->protocol_sink, &IID_IServiceProvider, (void**)&service_provider);
        if(SUCCEEDED(hres)) {
            hres = IServiceProvider_QueryService(service_provider, &IID_IBindCallbackRedirect, &IID_IBindCallbackRedirect,
                                                 (void**)&This->redirect_callback);
            IServiceProvider_Release(service_provider);
        }
    }

    *grfBINDF |= BINDF_FROMURLMON;
    return hres;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BindProtocol *This = impl_from_IInternetBindInfo(iface);

    TRACE("(%p)->(%ld %p %ld %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    return IInternetBindInfo_GetBindString(This->bind_info, ulStringType, ppwzStr, cEl, pcElFetched);
}

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

static inline BindProtocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, IInternetPriority_iface);
}

static HRESULT WINAPI InternetPriority_QueryInterface(IInternetPriority *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI InternetPriority_AddRef(IInternetPriority *iface)
{
    BindProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI InternetPriority_Release(IInternetPriority *iface)
{
    BindProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI InternetPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    BindProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%ld)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI InternetPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    BindProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

static const IInternetPriorityVtbl InternetPriorityVtbl = {
    InternetPriority_QueryInterface,
    InternetPriority_AddRef,
    InternetPriority_Release,
    InternetPriority_SetPriority,
    InternetPriority_GetPriority

};

static inline BindProtocol *impl_from_IInternetProtocolSink(IInternetProtocolSink *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, IInternetProtocolSink_iface);
}

static HRESULT WINAPI BPInternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI BPInternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI BPInternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

typedef struct {
    task_header_t header;
    PROTOCOLDATA *data;
} switch_task_t;

static void switch_proc(BindProtocol *bind, task_header_t *t)
{
    switch_task_t *task = (switch_task_t*)t;

    IInternetProtocol_Continue(bind->protocol_handler, task->data);

    free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);
    PROTOCOLDATA *data;

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    TRACE("flags %lx state %lx data %p cb %lu\n", pProtocolData->grfFlags, pProtocolData->dwState,
          pProtocolData->pData, pProtocolData->cbData);

    data = malloc(sizeof(PROTOCOLDATA));
    if(!data)
        return E_OUTOFMEMORY;
    memcpy(data, pProtocolData, sizeof(PROTOCOLDATA));

    if((This->pi&PI_APARTMENTTHREADED && pProtocolData->grfFlags&PI_FORCE_ASYNC)
            || !do_direct_notif(This)) {
        switch_task_t *task;

        task = malloc(sizeof(switch_task_t));
        if(!task)
        {
            free(data);
            return E_OUTOFMEMORY;
        }

        task->data = data;

        push_task(This, &task->header, switch_proc);
        return S_OK;
    }

    return IInternetProtocolSink_Switch(This->protocol_sink_handler, data);
}

typedef struct {
    task_header_t header;

    ULONG status_code;
    LPWSTR status_text;
} on_progress_task_t;

static void on_progress_proc(BindProtocol *This, task_header_t *t)
{
    on_progress_task_t *task = (on_progress_task_t*)t;

    IInternetProtocolSink_ReportProgress(This->protocol_sink_handler, task->status_code, task->status_text);

    free(task->status_text);
    free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);

    TRACE("(%p)->(%lu %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    if(do_direct_notif(This)) {
        IInternetProtocolSink_ReportProgress(This->protocol_sink_handler, ulStatusCode, szStatusText);
    }else {
        on_progress_task_t *task;

        task = malloc(sizeof(on_progress_task_t));

        task->status_code = ulStatusCode;
        task->status_text = wcsdup(szStatusText);

        push_task(This, &task->header, on_progress_proc);
    }

    return S_OK;
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

    IInternetProtocolSink_ReportData(This->protocol_sink_handler,
            task->bscf, task->progress, task->progress_max);

    free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);

    TRACE("(%p)->(%lx %lu %lu)\n", This, grfBSCF, ulProgress, ulProgressMax);

    if(!This->protocol_sink)
        return S_OK;

    if(!do_direct_notif(This)) {
        report_data_task_t *task;

        task = malloc(sizeof(report_data_task_t));
        if(!task)
            return E_OUTOFMEMORY;

        task->bscf = grfBSCF;
        task->progress = ulProgress;
        task->progress_max = ulProgressMax;

        push_task(This, &task->header, report_data_proc);
        return S_OK;
    }

    return IInternetProtocolSink_ReportData(This->protocol_sink_handler,
            grfBSCF, ulProgress, ulProgressMax);
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

    IInternetProtocolSink_ReportResult(This->protocol_sink_handler, task->hres, task->err, task->str);

    free(task->str);
    free(task);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    BindProtocol *This = impl_from_IInternetProtocolSink(iface);

    TRACE("(%p)->(%08lx %ld %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    if(!This->protocol_sink)
        return E_FAIL;
    This->reported_result = TRUE;

    if(!do_direct_notif(This)) {
        report_result_task_t *task;

        task = malloc(sizeof(report_result_task_t));
        if(!task)
            return E_OUTOFMEMORY;

        task->hres = hrResult;
        task->err = dwError;
        task->str = wcsdup(szResult);

        push_task(This, &task->header, report_result_proc);
        return S_OK;
    }

    return IInternetProtocolSink_ReportResult(This->protocol_sink_handler, hrResult, dwError, szResult);
}

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    BPInternetProtocolSink_QueryInterface,
    BPInternetProtocolSink_AddRef,
    BPInternetProtocolSink_Release,
    BPInternetProtocolSink_Switch,
    BPInternetProtocolSink_ReportProgress,
    BPInternetProtocolSink_ReportData,
    BPInternetProtocolSink_ReportResult
};

static inline BindProtocol *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, BindProtocol, IServiceProvider_iface);
}

static HRESULT WINAPI BPServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IServiceProvider(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI BPServiceProvider_AddRef(IServiceProvider *iface)
{
    BindProtocol *This = impl_from_IServiceProvider(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI BPServiceProvider_Release(IServiceProvider *iface)
{
    BindProtocol *This = impl_from_IServiceProvider(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI BPServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BindProtocol *This = impl_from_IServiceProvider(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->service_provider)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(This->service_provider, guidService, riid, ppv);
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BPServiceProvider_QueryInterface,
    BPServiceProvider_AddRef,
    BPServiceProvider_Release,
    BPServiceProvider_QueryService
};

HRESULT create_binding_protocol(BindProtocol **protocol)
{
    BindProtocol *ret = calloc(1, sizeof(BindProtocol));

    ret->IInternetProtocolEx_iface.lpVtbl   = &BindProtocolVtbl;
    ret->IInternetBindInfo_iface.lpVtbl     = &InternetBindInfoVtbl;
    ret->IInternetPriority_iface.lpVtbl     = &InternetPriorityVtbl;
    ret->IServiceProvider_iface.lpVtbl      = &ServiceProviderVtbl;
    ret->IInternetProtocolSink_iface.lpVtbl = &InternetProtocolSinkVtbl;

    ret->default_protocol_handler.IInternetProtocol_iface.lpVtbl = &InternetProtocolHandlerVtbl;
    ret->default_protocol_handler.IInternetProtocolSink_iface.lpVtbl = &InternetProtocolSinkHandlerVtbl;

    ret->ref = 1;
    ret->apartment_thread = GetCurrentThreadId();
    ret->notif_hwnd = get_notif_hwnd();
    ret->protocol_handler = &ret->default_protocol_handler.IInternetProtocol_iface;
    ret->protocol_sink_handler = &ret->default_protocol_handler.IInternetProtocolSink_iface;
    InitializeCriticalSectionEx(&ret->section, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    ret->section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BindProtocol.section");

    URLMON_LockModule();

    *protocol = ret;
    return S_OK;
}
