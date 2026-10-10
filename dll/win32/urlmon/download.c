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
    IBindStatusCallback IBindStatusCallback_iface;
    IServiceProvider    IServiceProvider_iface;

    LONG ref;

    IBindStatusCallback *callback;
    IBinding *binding;
    LPWSTR file_name;
    LPWSTR cache_file;
    DWORD bindf;

    stop_cache_binding_proc_t onstop_proc;
    void *ctx;
} DownloadBSC;

static inline DownloadBSC *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, DownloadBSC, IBindStatusCallback_iface);
}

static inline DownloadBSC *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, DownloadBSC, IServiceProvider_iface);
}

static HRESULT WINAPI DownloadBSC_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider, %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DownloadBSC_AddRef(IBindStatusCallback *iface)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI DownloadBSC_Release(IBindStatusCallback *iface)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    if(!ref) {
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        if(This->binding)
            IBinding_Release(This->binding);
        free(This->file_name);
        free(This->cache_file);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DownloadBSC_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    HRESULT hres = S_OK;

    TRACE("(%p)->(%ld %p)\n", This, dwReserved, pbind);

    if(This->callback) {
        hres = IBindStatusCallback_OnStartBinding(This->callback, dwReserved, pbind);

        IBinding_AddRef(pbind);
        This->binding = pbind;
    }

    /* Windows seems to ignore E_NOTIMPL if it's returned from the client. */
    return hres == E_NOTIMPL ? S_OK : hres;
}

static HRESULT WINAPI DownloadBSC_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI DownloadBSC_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%ld)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT on_progress(DownloadBSC *This, ULONG progress, ULONG progress_max, ULONG status_code, LPCWSTR status_text)
{
    HRESULT hres;

    if(!This->callback)
        return S_OK;

    hres = IBindStatusCallback_OnProgress(This->callback, progress, progress_max, status_code, status_text);
    if(hres == E_ABORT) {
        if(This->binding)
            IBinding_Abort(This->binding);
        else
            FIXME("No binding, not sure what to do!\n");
    }

    return hres;
}

static HRESULT WINAPI DownloadBSC_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    HRESULT hres = S_OK;

    TRACE("%p)->(%lu %lu %lu %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_CONNECTING:
    case BINDSTATUS_BEGINDOWNLOADDATA:
    case BINDSTATUS_DOWNLOADINGDATA:
    case BINDSTATUS_ENDDOWNLOADDATA:
    case BINDSTATUS_SENDINGREQUEST:
    case BINDSTATUS_MIMETYPEAVAILABLE:
        hres = on_progress(This, ulProgress, ulProgressMax, ulStatusCode, szStatusText);
        break;

    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        hres = on_progress(This, ulProgress, ulProgressMax, ulStatusCode, szStatusText);
        This->cache_file = wcsdup(szStatusText);
        break;

    case BINDSTATUS_FINDINGRESOURCE: /* FIXME */
        break;

    default:
        FIXME("Unsupported status %lu\n", ulStatusCode);
    }

    return hres;
}

static HRESULT WINAPI DownloadBSC_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    HRESULT hres = S_OK;

    TRACE("(%p)->(%08lx %s)\n", This, hresult, debugstr_w(szError));

    if(This->file_name) {
        if(This->cache_file) {
            BOOL b;

            b = CopyFileW(This->cache_file, This->file_name, FALSE);
            if(!b)
                FIXME("CopyFile failed: %lu\n", GetLastError());
        }else {
            FIXME("No cache file\n");
        }
    }

    if(This->onstop_proc)
        hres = This->onstop_proc(This->ctx, This->cache_file, hresult, szError);
    else if(This->callback)
        IBindStatusCallback_OnStopBinding(This->callback, hresult, szError);

    if(This->binding) {
        IBinding_Release(This->binding);
        This->binding = NULL;
    }

    return hres;
}

static HRESULT WINAPI DownloadBSC_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
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

    *grfBINDF = BINDF_PULLDATA | BINDF_NEEDFILE | (bindf & BINDF_ENFORCERESTRICTED) | This->bindf;
    return S_OK;
}

static HRESULT WINAPI DownloadBSC_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%08lx %ld %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return S_OK;
}

static HRESULT WINAPI DownloadBSC_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    DownloadBSC *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

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

static HRESULT WINAPI DwlServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    DownloadBSC *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI DwlServiceProvider_AddRef(IServiceProvider *iface)
{
    DownloadBSC *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI DwlServiceProvider_Release(IServiceProvider *iface)
{
    DownloadBSC *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI DwlServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    DownloadBSC *This = impl_from_IServiceProvider(iface);
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

static const IServiceProviderVtbl ServiceProviderVtbl = {
    DwlServiceProvider_QueryInterface,
    DwlServiceProvider_AddRef,
    DwlServiceProvider_Release,
    DwlServiceProvider_QueryService
};

static HRESULT DownloadBSC_Create(IBindStatusCallback *callback, LPCWSTR file_name, DownloadBSC **ret_callback)
{
    DownloadBSC *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    ret->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    ret->ref = 1;

    if(file_name) {
        ret->file_name = wcsdup(file_name);
        if(!ret->file_name) {
            free(ret);
            return E_OUTOFMEMORY;
        }
    }

    if(callback)
        IBindStatusCallback_AddRef(callback);
    ret->callback = callback;

    *ret_callback = ret;
    return S_OK;
}

HRESULT create_default_callback(IBindStatusCallback **ret)
{
    DownloadBSC *callback;
    HRESULT hres;

    hres = DownloadBSC_Create(NULL, NULL, &callback);
    if(FAILED(hres))
        return hres;

    hres = wrap_callback(&callback->IBindStatusCallback_iface, ret);
    IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
    return hres;
}

HRESULT download_to_cache(IUri *uri, stop_cache_binding_proc_t proc, void *ctx, IBindStatusCallback *callback)
{
    DownloadBSC *dwl_bsc;
    IBindCtx *bindctx;
    IMoniker *mon;
    IUnknown *unk;
    HRESULT hres;

    hres = DownloadBSC_Create(callback, NULL, &dwl_bsc);
    if(FAILED(hres))
        return hres;

    dwl_bsc->onstop_proc = proc;
    dwl_bsc->ctx = ctx;
    dwl_bsc->bindf = BINDF_ASYNCHRONOUS;

    hres = CreateAsyncBindCtx(0, &dwl_bsc->IBindStatusCallback_iface, NULL, &bindctx);
    IBindStatusCallback_Release(&dwl_bsc->IBindStatusCallback_iface);
    if(FAILED(hres))
        return hres;

    hres = CreateURLMonikerEx2(NULL, uri, &mon, 0);
    if(FAILED(hres)) {
        IBindCtx_Release(bindctx);
        return hres;
    }

    hres = IMoniker_BindToStorage(mon, bindctx, NULL, &IID_IUnknown, (void**)&unk);
    IMoniker_Release(mon);
    IBindCtx_Release(bindctx);
    if(SUCCEEDED(hres) && unk)
        IUnknown_Release(unk);
    return hres;

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
    DownloadBSC *callback;
    IUnknown *unk;
    IMoniker *mon;
    IBindCtx *bindctx;
    HRESULT hres;

    TRACE("(%p %s %s %ld %p)\n", pCaller, debugstr_w(szURL), debugstr_w(szFileName), dwReserved, lpfnCB);

    if(pCaller)
        FIXME("pCaller not supported\n");

    hres = DownloadBSC_Create(lpfnCB, szFileName, &callback);
    if(FAILED(hres))
        return hres;

    hres = CreateAsyncBindCtx(0, &callback->IBindStatusCallback_iface, NULL, &bindctx);
    IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
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

    TRACE("(%p %s %s %ld %p)\n", pCaller, debugstr_a(szURL), debugstr_a(szFileName), dwReserved, lpfnCB);

    urlW = strdupAtoW(szURL);
    file_nameW = strdupAtoW(szFileName);

    hres = URLDownloadToFileW(pCaller, urlW, file_nameW, dwReserved, lpfnCB);

    free(urlW);
    free(file_nameW);

    return hres;
}
