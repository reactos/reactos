/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

typedef struct {
    const IBindStatusCallbackVtbl  *lpBindStatusCallbackVtbl;
    const IServiceProviderVtbl     *lpServiceProviderVtbl;

    LONG ref;

    IBindStatusCallback *callback;
    LPWSTR file_name;
    LPWSTR cache_file;
} DownloadBSC;

#define STATUSCLB(x)     ((IBindStatusCallback*)  &(x)->lpBindStatusCallbackVtbl)
#define SERVPROV(x)      ((IServiceProvider*)     &(x)->lpServiceProviderVtbl)

#define STATUSCLB_THIS(iface) DEFINE_THIS(DownloadBSC, BindStatusCallback, iface)

static HRESULT WINAPI DownloadBSC_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider, %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DownloadBSC_AddRef(IBindStatusCallback *iface)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI DownloadBSC_Release(IBindStatusCallback *iface)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    if(!ref) {
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        heap_free(This->file_name);
        heap_free(This->cache_file);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI DownloadBSC_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%d %p)\n", This, dwReserved, pbind);

    if(This->callback)
        IBindStatusCallback_OnStartBinding(This->callback, dwReserved, pbind);

    return S_OK;
}

static HRESULT WINAPI DownloadBSC_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI DownloadBSC_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static void on_progress(DownloadBSC *This, ULONG progress, ULONG progress_max, ULONG status_code, LPCWSTR status_text)
{
    HRESULT hres;

    if(!This->callback)
        return;

    hres = IBindStatusCallback_OnProgress(This->callback, progress, progress_max, status_code, status_text);
    if(FAILED(hres))
        FIXME("OnProgress failed: %08x\n", hres);
}

static HRESULT WINAPI DownloadBSC_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);

    TRACE("%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
    case BINDSTATUS_DOWNLOADINGDATA:
    case BINDSTATUS_ENDDOWNLOADDATA:
    case BINDSTATUS_SENDINGREQUEST:
    case BINDSTATUS_MIMETYPEAVAILABLE:
        on_progress(This, ulProgress, ulProgressMax, ulStatusCode, szStatusText);
        break;

    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        on_progress(This, ulProgress, ulProgressMax, ulStatusCode, szStatusText);
        This->cache_file = heap_strdupW(szStatusText);
        break;

    case BINDSTATUS_FINDINGRESOURCE:
    case BINDSTATUS_CONNECTING:
        break;

    default:
        FIXME("Unsupported status %u\n", ulStatusCode);
    }

    return S_OK;
}

static HRESULT WINAPI DownloadBSC_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    if(This->cache_file) {
        BOOL b;

        b = CopyFileW(This->cache_file, This->file_name, FALSE);
        if(!b)
            FIXME("CopyFile failed: %u\n", GetLastError());
    }else {
        FIXME("No cache file\n");
    }

    if(This->callback)
        IBindStatusCallback_OnStopBinding(This->callback, hresult, szError);

    return S_OK;
}

static HRESULT WINAPI DownloadBSC_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    DWORD bindf = 0;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    if(This->callback) {
        BINDINFO bindinfo;
        HRESULT hres;

        memset(&bindinfo, 0, sizeof(bindinfo));
        bindinfo.cbSize = sizeof(bindinfo);

        hres = IBindStatusCallback_GetBindInfo(This->callback, &bindf, &bindinfo);
        if(SUCCEEDED(hres))
            ReleaseBindInfo(&bindinfo);
    }

    *grfBINDF = BINDF_PULLDATA | BINDF_NEEDFILE | (bindf & BINDF_ENFORCERESTRICTED);
    return S_OK;
}

static HRESULT WINAPI DownloadBSC_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return S_OK;
}

static HRESULT WINAPI DownloadBSC_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    DownloadBSC *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

#undef STATUSCLB_THIS

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    DownloadBSC_QueryInterface,
    DownloadBSC_AddRef,
    DownloadBSC_Release,
    DownloadBSC_OnStartBinding,
    DownloadBSC_GetPriority,
    DownloadBSC_OnLowResource,
    DownloadBSC_OnProgress,
    DownloadBSC_OnStopBinding,
    DownloadBSC_GetBindInfo,
    DownloadBSC_OnDataAvailable,
    DownloadBSC_OnObjectAvailable
};

#define SERVPROV_THIS(iface) DEFINE_THIS(DownloadBSC, ServiceProvider, iface)

static HRESULT WINAPI DwlServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    DownloadBSC *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI DwlServiceProvider_AddRef(IServiceProvider *iface)
{
    DownloadBSC *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI DwlServiceProvider_Release(IServiceProvider *iface)
{
    DownloadBSC *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI DwlServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    DownloadBSC *This = SERVPROV_THIS(iface);
    IServiceProvider *serv_prov;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->callback)
        return E_NOINTERFACE;

    hres = IBindStatusCallback_QueryInterface(This->callback, riid, ppv);
    if(SUCCEEDED(hres))
        return S_OK;

    hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IServiceProvider, (void**)&serv_prov);
    if(SUCCEEDED(hres)) {
        hres = IServiceProvider_QueryService(serv_prov, guidService, riid, ppv);
        IServiceProvider_Release(serv_prov);
        return hres;
    }

    return E_NOINTERFACE;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    DwlServiceProvider_QueryInterface,
    DwlServiceProvider_AddRef,
    DwlServiceProvider_Release,
    DwlServiceProvider_QueryService
};

static IBindStatusCallback *DownloadBSC_Create(IBindStatusCallback *callback, LPCWSTR file_name)
{
    DownloadBSC *ret = heap_alloc(sizeof(*ret));

    ret->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    ret->lpServiceProviderVtbl    = &ServiceProviderVtbl;
    ret->ref = 1;
    ret->file_name = heap_strdupW(file_name);
    ret->cache_file = NULL;

    if(callback)
        IBindStatusCallback_AddRef(callback);
    ret->callback = callback;

    return STATUSCLB(ret);
}

/***********************************************************************
 *           URLDownloadToFileW (URLMON.@)
 *
 * Downloads URL szURL to file szFileName and call lpfnCB callback to
 * report progress.
 *
 * PARAMS
 *  pCaller    [I] controlling IUnknown interface.
 *  szURL      [I] URL of the file to download
 *  szFileName [I] file name to store the content of the URL
 *  dwReserved [I] reserved - set to 0
 *  lpfnCB     [I] callback for progress report
 *
 * RETURNS
 *  S_OK on success
 */
HRESULT WINAPI URLDownloadToFileW(LPUNKNOWN pCaller, LPCWSTR szURL, LPCWSTR szFileName,
        DWORD dwReserved, LPBINDSTATUSCALLBACK lpfnCB)
{
    IBindStatusCallback *callback;
    IUnknown *unk;
    IMoniker *mon;
    IBindCtx *bindctx;
    HRESULT hres;

    TRACE("(%p %s %s %d %p)\n", pCaller, debugstr_w(szURL), debugstr_w(szFileName), dwReserved, lpfnCB);

    if(pCaller)
        FIXME("pCaller not supported\n");

    callback = DownloadBSC_Create(lpfnCB, szFileName);
    hres = CreateAsyncBindCtx(0, callback, NULL, &bindctx);
    IBindStatusCallback_Release(callback);
    if(FAILED(hres))
        return hres;

    hres = CreateURLMoniker(NULL, szURL, &mon);
    if(FAILED(hres)) {
        IBindCtx_Release(bindctx);
        return hres;
    }

    hres = IMoniker_BindToStorage(mon, bindctx, NULL, &IID_IUnknown, (void**)&unk);
    IMoniker_Release(mon);
    IBindCtx_Release(bindctx);

    if(unk)
        IUnknown_Release(unk);

    return hres == MK_S_ASYNCHRONOUS ? S_OK : hres;
}

/***********************************************************************
 *           URLDownloadToFileA (URLMON.@)
 *
 * Downloads URL szURL to rile szFileName and call lpfnCB callback to
 * report progress.
 *
 * PARAMS
 *  pCaller    [I] controlling IUnknown interface.
 *  szURL      [I] URL of the file to download
 *  szFileName [I] file name to store the content of the URL
 *  dwReserved [I] reserved - set to 0
 *  lpfnCB     [I] callback for progress report
 *
 * RETURNS
 *  S_OK on success
 */
HRESULT WINAPI URLDownloadToFileA(LPUNKNOWN pCaller, LPCSTR szURL, LPCSTR szFileName, DWORD dwReserved,
        LPBINDSTATUSCALLBACK lpfnCB)
{
    LPWSTR urlW, file_nameW;
    HRESULT hres;

    TRACE("(%p %s %s %d %p)\n", pCaller, debugstr_a(szURL), debugstr_a(szFileName), dwReserved, lpfnCB);

    urlW = heap_strdupAtoW(szURL);
    file_nameW = heap_strdupAtoW(szFileName);

    hres = URLDownloadToFileW(pCaller, urlW, file_nameW, dwReserved, lpfnCB);

    heap_free(urlW);
    heap_free(file_nameW);

    return hres;
}
