/*
 * Copyright 2006-2010 Jacek Caban for CodeWeavers
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
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "hlguids.h"
#include "shlguid.h"
#include "wininet.h"
#include "shlwapi.h"
#include "htiface.h"
#include "shdeprecated.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlscript.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct nsProtocolStream {
    nsIInputStream nsIInputStream_iface;

    LONG ref;

    char buf[1024];
    DWORD buf_size;
};

static inline nsProtocolStream *impl_from_nsIInputStream(nsIInputStream *iface)
{
    return CONTAINING_RECORD(iface, nsProtocolStream, nsIInputStream_iface);
}

static nsresult NSAPI nsInputStream_QueryInterface(nsIInputStream *iface, nsIIDRef riid,
        void **result)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result  = &This->nsIInputStream_iface;
    }else if(IsEqualGUID(&IID_nsIInputStream, riid)) {
        TRACE("(%p)->(IID_nsIInputStream %p)\n", This, result);
        *result  = &This->nsIInputStream_iface;
    }

    if(*result) {
        nsIInputStream_AddRef(&This->nsIInputStream_iface);
        return NS_OK;
    }

    WARN("unsupported interface %s\n", debugstr_guid(riid));
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsInputStream_AddRef(nsIInputStream *iface)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}


static nsrefcnt NSAPI nsInputStream_Release(nsIInputStream *iface)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static nsresult NSAPI nsInputStream_Close(nsIInputStream *iface)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    FIXME("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Available(nsIInputStream *iface, UINT64 *_retval)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Read(nsIInputStream *iface, char *aBuf, UINT32 aCount,
                                         UINT32 *_retval)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    DWORD read = aCount;

    TRACE("(%p)->(%p %d %p)\n", This, aBuf, aCount, _retval);

    if(read > This->buf_size)
        read = This->buf_size;

    if(read) {
        memcpy(aBuf, This->buf, read);
        if(read < This->buf_size)
            memmove(This->buf, This->buf+read, This->buf_size-read);
        This->buf_size -= read;
    }

    *_retval = read;
    return NS_OK;
}

static nsresult NSAPI nsInputStream_ReadSegments(nsIInputStream *iface,
        nsresult (WINAPI *aWriter)(nsIInputStream*,void*,const char*,UINT32,UINT32,UINT32*),
        void *aClousure, UINT32 aCount, UINT32 *_retval)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    UINT32 written = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p %d %p)\n", This, aWriter, aClousure, aCount, _retval);

    if(!This->buf_size)
        return S_OK;

    if(aCount > This->buf_size)
        aCount = This->buf_size;

    nsres = aWriter(&This->nsIInputStream_iface, aClousure, This->buf, 0, aCount, &written);
    if(NS_FAILED(nsres))
        TRACE("aWriter failed: %08lx\n", nsres);
    else if(written != This->buf_size)
        FIXME("written %d != buf_size %ld\n", written, This->buf_size);

    This->buf_size -= written; 

    *_retval = written;
    return nsres;
}

static nsresult NSAPI nsInputStream_IsNonBlocking(nsIInputStream *iface, cpp_bool *_retval)
{
    nsProtocolStream *This = impl_from_nsIInputStream(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIInputStreamVtbl nsInputStreamVtbl = {
    nsInputStream_QueryInterface,
    nsInputStream_AddRef,
    nsInputStream_Release,
    nsInputStream_Close,
    nsInputStream_Available,
    nsInputStream_Read,
    nsInputStream_ReadSegments,
    nsInputStream_IsNonBlocking
};

static nsProtocolStream *create_nsprotocol_stream(void)
{
    nsProtocolStream *ret;

    ret = malloc(sizeof(nsProtocolStream));
    if(!ret)
        return NULL;

    ret->nsIInputStream_iface.lpVtbl = &nsInputStreamVtbl;
    ret->ref = 1;
    ret->buf_size = 0;

    return ret;
}

static void release_request_data(request_data_t *request_data)
{
    if(request_data->post_stream)
        nsIInputStream_Release(request_data->post_stream);
    free(request_data->headers);
    if(request_data->post_data)
        GlobalFree(request_data->post_data);
}

static inline BSCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, BSCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = &This->IHttpNegotiate2_iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate2 %p)\n", This, ppv);
        *ppv = &This->IHttpNegotiate2_iface;
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = &This->IInternetBindInfo_iface;
    }else if(IsEqualGUID(&IID_IBindCallbackRedirect, riid)) {
        TRACE("(%p)->(IID_IBindCallbackRedirect %p)\n", This, ppv);
        *ppv = &This->IBindCallbackRedirect_iface;
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    if(!ref) {
        release_request_data(&This->request_data);
        if(This->mon)
            IMoniker_Release(This->mon);
        if(This->binding)
            IBinding_Release(This->binding);
        list_remove(&This->entry);
        list_init(&This->entry);

        This->vtbl->destroy(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwReserved, pbind);

    IBinding_AddRef(pbind);
    This->binding = pbind;

    if(This->window)
        list_add_head(&This->window->bindings, &This->entry);

    return This->vtbl->start_binding(This);
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%ld)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("%p)->(%lu %lu %lu %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    return This->vtbl->on_progress(This, ulProgress, ulProgressMax, ulStatusCode, szStatusText);
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    HRESULT hres;

    TRACE("(%p)->(%08lx %s)\n", This, hresult, debugstr_w(szError));

    /* NOTE: IE7 calls GetBindResult here */

    hres = This->vtbl->stop_binding(This, hresult);

    unlink_ref(&This->binding);
    unlink_ref(&This->mon);

    list_remove(&This->entry);
    list_init(&This->entry);
    This->window = NULL;

    return hres;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    DWORD size;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    if(!This->bindinfo_ready) {
        HRESULT hres;

        hres = This->vtbl->init_bindinfo(This);
        if(FAILED(hres))
            return hres;

        This->bindinfo_ready = TRUE;
    }

    *grfBINDF = This->bindf;

    size = pbindinfo->cbSize;
    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;

    pbindinfo->cbstgmedData = This->request_data.post_data_len;
    pbindinfo->dwCodePage = CP_UTF8;
    pbindinfo->dwOptions = This->bindinfo_options;

    if(This->request_data.post_data_len) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.hGlobal = This->request_data.post_data;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)&This->IBindStatusCallback_iface;
        IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%08lx %ld %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return This->vtbl->read_data(This, pstgmed->pstm);
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BSCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    BindStatusCallback_QueryInterface,
    BindStatusCallback_AddRef,
    BindStatusCallback_Release,
    BindStatusCallback_OnStartBinding,
    BindStatusCallback_GetPriority,
    BindStatusCallback_OnLowResource,
    BindStatusCallback_OnProgress,
    BindStatusCallback_OnStopBinding,
    BindStatusCallback_GetBindInfo,
    BindStatusCallback_OnDataAvailable,
    BindStatusCallback_OnObjectAvailable
};

static inline BSCallback *impl_from_IHttpNegotiate2(IHttpNegotiate2 *iface)
{
    return CONTAINING_RECORD(iface, BSCallback, IHttpNegotiate2_iface);
}

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface,
                                                   REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %ld %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    *pszAdditionalHeaders = NULL;

    hres = This->vtbl->beginning_transaction(This, pszAdditionalHeaders);
    if(hres != S_FALSE)
        return hres;

    if(This->request_data.headers) {
        DWORD size;

        size = (lstrlenW(This->request_data.headers)+1)*sizeof(WCHAR);
        *pszAdditionalHeaders = CoTaskMemAlloc(size);
        if(!*pszAdditionalHeaders)
            return E_OUTOFMEMORY;
        memcpy(*pszAdditionalHeaders, This->request_data.headers, size);
    }

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);

    TRACE("(%p)->(%ld %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);

    return This->vtbl->on_response(This, dwResponseCode, szResponseHeaders);
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    BSCallback *This = impl_from_IHttpNegotiate2(iface);
    TRACE("(%p)->(%p %p %Id)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL; /* FIXME */
}

static const IHttpNegotiate2Vtbl HttpNegotiate2Vtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

static inline BSCallback *impl_from_IInternetBindInfo(IInternetBindInfo *iface)
{
    return CONTAINING_RECORD(iface, BSCallback, IInternetBindInfo_iface);
}

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
                                                      REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IInternetBindInfo(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    BSCallback *This = impl_from_IInternetBindInfo(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    BSCallback *This = impl_from_IInternetBindInfo(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
                                                   DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = impl_from_IInternetBindInfo(iface);
    FIXME("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BSCallback *This = impl_from_IInternetBindInfo(iface);
    FIXME("(%p)->(%lu %p %lu %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);
    return E_NOTIMPL;
}

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

static inline BSCallback *impl_from_IBindCallbackRedirect(IBindCallbackRedirect *iface)
{
    return CONTAINING_RECORD(iface, BSCallback, IBindCallbackRedirect_iface);
}

static HRESULT WINAPI BindCallbackRedirect_QueryInterface(IBindCallbackRedirect *iface, REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IBindCallbackRedirect(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI BindCallbackRedirect_AddRef(IBindCallbackRedirect *iface)
{
    BSCallback *This = impl_from_IBindCallbackRedirect(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI BindCallbackRedirect_Release(IBindCallbackRedirect *iface)
{
    BSCallback *This = impl_from_IBindCallbackRedirect(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI BindCallbackRedirect_Redirect(IBindCallbackRedirect *iface, const WCHAR *url, VARIANT_BOOL *vbCancel)
{
    BSCallback *This = impl_from_IBindCallbackRedirect(iface);
    GeckoBrowser *browser;
    BOOL cancel = FALSE;
    BSTR frame_name = NULL;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(url), vbCancel);

    if(This->window && This->window->base.outer_window && (browser = This->window->base.outer_window->browser)
       && browser->doc->doc_object_service) {
        if(is_main_content_window(This->window->base.outer_window)) {
            hres = IHTMLWindow2_get_name(&This->window->base.IHTMLWindow2_iface, &frame_name);
            if(FAILED(hres))
                return hres;
        }

        hres = IDocObjectService_FireBeforeNavigate2(browser->doc->doc_object_service, NULL, url, 0x40,
                                                     frame_name, NULL, 0, NULL, TRUE, &cancel);
        SysFreeString(frame_name);
    }

    *vbCancel = variant_bool(cancel);
    return hres;
}

static const IBindCallbackRedirectVtbl BindCallbackRedirectVtbl = {
    BindCallbackRedirect_QueryInterface,
    BindCallbackRedirect_AddRef,
    BindCallbackRedirect_Release,
    BindCallbackRedirect_Redirect
};

static inline BSCallback *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, BSCallback, IServiceProvider_iface);
}

static HRESULT WINAPI BSCServiceProvider_QueryInterface(IServiceProvider *iface,
                                                        REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI BSCServiceProvider_AddRef(IServiceProvider *iface)
{
    BSCallback *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI BSCServiceProvider_Release(IServiceProvider *iface)
{
    BSCallback *This = impl_from_IServiceProvider(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI BSCServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BSCallback *This = impl_from_IServiceProvider(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->window && IsEqualGUID(guidService, &IID_IWindowForBindingUI))
        return IServiceProvider_QueryService(&This->window->base.IServiceProvider_iface, guidService, riid, ppv);
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BSCServiceProvider_QueryInterface,
    BSCServiceProvider_AddRef,
    BSCServiceProvider_Release,
    BSCServiceProvider_QueryService
};

void init_bscallback(BSCallback *This, const BSCallbackVtbl *vtbl, IMoniker *mon, DWORD bindf)
{
    This->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    This->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    This->IHttpNegotiate2_iface.lpVtbl = &HttpNegotiate2Vtbl;
    This->IInternetBindInfo_iface.lpVtbl = &InternetBindInfoVtbl;
    This->IBindCallbackRedirect_iface.lpVtbl = &BindCallbackRedirectVtbl;
    This->vtbl = vtbl;
    This->ref = 1;
    This->bindf = bindf;
    This->bindinfo_options = BINDINFO_OPTIONS_USE_IE_ENCODING;
    This->bom = BOM_NONE;

    list_init(&This->entry);

    if(mon)
        IMoniker_AddRef(mon);
    This->mon = mon;
}

HRESULT read_stream(BSCallback *This, IStream *stream, void *buf, DWORD size, DWORD *ret_size)
{
    DWORD read_size = 0, skip=0;
    BYTE *data = buf;
    HRESULT hres;

    hres = IStream_Read(stream, buf, size, &read_size);

    if(!This->read && This->bom == BOM_NONE) {
        if(read_size >= 2 && data[0] == 0xff && data[1] == 0xfe) {
            This->bom = BOM_UTF16;
            skip = 2;
        }else if(read_size >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf) {
            This->bom = BOM_UTF8;
            skip = 3;
        }
        if(skip) {
            read_size -= skip;
            if(read_size)
                memmove(data, data+skip, read_size);
        }
    }

    This->read += read_size;
    *ret_size = read_size;
    return hres;
}

static void parse_content_type(nsChannelBSC *This, const WCHAR *value)
{
    const WCHAR *ptr, *beg, *end;
    size_t len = wcslen(value);
    char *content_type;

    static const WCHAR charsetW[] = {'c','h','a','r','s','e','t','='};

    ptr = wcschr(value, ';');

    if(!This->nschannel->content_type || !(This->nschannel->load_flags & LOAD_CALL_CONTENT_SNIFFERS)) {
        for(end = ptr ? ptr : value + len; end > value; end--)
            if(!iswspace(end[-1]))
                break;
        for(beg = value; beg < end; beg++)
            if(!iswspace(*beg))
                break;

        if((content_type = strndupWtoU(beg, end - beg))) {
            free(This->nschannel->content_type);
            This->nschannel->content_type = content_type;
            strlwr(content_type);
        }
    }

    if(!ptr)
        return;

    ptr++;
    while(*ptr && iswspace(*ptr))
        ptr++;

    if(ptr + ARRAY_SIZE(charsetW) < value+len && !wcsnicmp(ptr, charsetW, ARRAY_SIZE(charsetW))) {
        size_t charset_len, lena;
        nsACString charset_str;
        const WCHAR *charset;
        char *charseta;

        ptr += ARRAY_SIZE(charsetW);

        if(*ptr == '\'') {
            FIXME("Quoted value\n");
            return;
        }else {
            charset = ptr;
            while(*ptr && *ptr != ',')
                ptr++;
            charset_len = ptr-charset;
        }

        lena = WideCharToMultiByte(CP_ACP, 0, charset, charset_len, NULL, 0, NULL, NULL);
        charseta = malloc(lena + 1);
        if(!charseta)
            return;

        WideCharToMultiByte(CP_ACP, 0, charset, charset_len, charseta, lena, NULL, NULL);
        charseta[lena] = 0;

        nsACString_InitDepend(&charset_str, charseta);
        nsIHttpChannel_SetContentCharset(&This->nschannel->nsIHttpChannel_iface, &charset_str);
        nsACString_Finish(&charset_str);
        free(charseta);
    }else {
        FIXME("unhandled: %s\n", debugstr_wn(ptr, len - (ptr-value)));
    }
}

static HRESULT parse_headers(const WCHAR *headers, struct list *headers_list)
{
    const WCHAR *header, *header_end, *colon, *value;
    HRESULT hres;

    header = headers;
    while(*header) {
        if(header[0] == '\r' && header[1] == '\n' && !header[2])
            break;
        for(colon = header; *colon && *colon != ':' && *colon != '\r'; colon++);
        if(*colon != ':')
            return E_FAIL;

        value = colon+1;
        while(*value == ' ')
            value++;
        if(!*value)
            return E_FAIL;

        for(header_end = value+1; *header_end && *header_end != '\r'; header_end++);

        hres = set_http_header(headers_list, header, colon-header, value, header_end-value);
        if(FAILED(hres))
            return hres;

        header = header_end;
        if(header[0] == '\r' && header[1] == '\n')
            header += 2;
    }

    return S_OK;
}

static HRESULT process_response_headers(nsChannelBSC *This, const WCHAR *headers)
{
    http_header_t *iter;
    HRESULT hres;

    hres = parse_headers(headers, &This->nschannel->response_headers);
    if(FAILED(hres))
        return hres;

    LIST_FOR_EACH_ENTRY(iter, &This->nschannel->response_headers, http_header_t, entry) {
        if(!wcsicmp(iter->header, L"content-type"))
            parse_content_type(This, iter->data);
    }

    return S_OK;
}

static void query_http_info(nsChannelBSC *This, IWinInetHttpInfo *wininet_info)
{
    const WCHAR *ptr;
    DWORD len = 0;
    WCHAR *buf;

    IWinInetHttpInfo_QueryInfo(wininet_info, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &len, NULL, NULL);
    if(!len)
        return;

    buf = malloc(len);
    if(!buf)
        return;

    IWinInetHttpInfo_QueryInfo(wininet_info, HTTP_QUERY_RAW_HEADERS_CRLF, buf, &len, NULL, NULL);
    if(!len) {
        free(buf);
        return;
    }

    ptr = wcschr(buf, '\r');
    if(ptr && ptr[1] == '\n') {
        ptr += 2;
        process_response_headers(This, ptr);
    }

    free(buf);
}

HRESULT start_binding(HTMLInnerWindow *inner_window, BSCallback *bscallback, IBindCtx *bctx)
{
    IStream *str = NULL;
    HRESULT hres;

    TRACE("(%p %p %p)\n", inner_window, bscallback, bctx);

    bscallback->window = inner_window;

    /* NOTE: IE7 calls IsSystemMoniker here*/

    if(bctx) {
        hres = RegisterBindStatusCallback(bctx, &bscallback->IBindStatusCallback_iface, NULL, 0);
        if(SUCCEEDED(hres))
            IBindCtx_AddRef(bctx);
    }else {
        hres = CreateAsyncBindCtx(0, &bscallback->IBindStatusCallback_iface, NULL, &bctx);
    }

    if(FAILED(hres)) {
        bscallback->window = NULL;
        return hres;
    }

    hres = IMoniker_BindToStorage(bscallback->mon, bctx, NULL, &IID_IStream, (void**)&str);
    IBindCtx_Release(bctx);
    if(FAILED(hres)) {
        WARN("BindToStorage failed: %08lx\n", hres);
        bscallback->window = NULL;
        return hres;
    }

    if(str)
        IStream_Release(str);

    return S_OK;
}

static HRESULT read_post_data_stream(nsIInputStream *stream, BOOL contains_headers, struct list *headers_list,
        request_data_t *request_data)
{
    nsISeekableStream *seekable_stream;
    UINT64 available = 0;
    UINT32 data_len = 0;
    char *data, *post_data;
    nsresult nsres;
    HRESULT hres = S_OK;

    if(!stream)
        return S_OK;

    nsres =  nsIInputStream_Available(stream, &available);
    if(NS_FAILED(nsres))
        return E_FAIL;

    post_data = data = GlobalAlloc(0, available+1);
    if(!data)
        return E_OUTOFMEMORY;

    nsres = nsIInputStream_Read(stream, data, available, &data_len);
    if(NS_FAILED(nsres)) {
        GlobalFree(data);
        return E_FAIL;
    }

    if(contains_headers) {
        if(data_len >= 2 && data[0] == '\r' && data[1] == '\n') {
            post_data = data+2;
            data_len -= 2;
        }else {
            WCHAR *headers;
            DWORD size;
            char *ptr;

            post_data += data_len;
            for(ptr = data; ptr+4 < data+data_len; ptr++) {
                if(!memcmp(ptr, "\r\n\r\n", 4)) {
                    ptr += 2;
                    post_data = ptr+2;
                    break;
                }
            }

            data_len -= post_data-data;

            size = MultiByteToWideChar(CP_ACP, 0, data, ptr-data, NULL, 0);
            headers = malloc((size + 1) * sizeof(WCHAR));
            if(headers) {
                MultiByteToWideChar(CP_ACP, 0, data, ptr-data, headers, size);
                headers[size] = 0;
                if(headers_list)
                    hres = parse_headers(headers, headers_list);
                if(SUCCEEDED(hres))
                    request_data->headers = headers;
                else
                    free(headers);
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    if(FAILED(hres)) {
        GlobalFree(data);
        return hres;
    }

    if(!data_len) {
        GlobalFree(data);
        post_data = NULL;
    }else if(post_data != data) {
        char *new_data;

        new_data = GlobalAlloc(0, data_len+1);
        if(new_data)
            memcpy(new_data, post_data, data_len);
        GlobalFree(data);
        if(!new_data)
            return E_OUTOFMEMORY;
        post_data = new_data;
    }

    if(post_data)
        post_data[data_len] = 0;
    request_data->post_data = post_data;
    request_data->post_data_len = data_len;

    nsres = nsIInputStream_QueryInterface(stream, &IID_nsISeekableStream, (void**)&seekable_stream);
    assert(nsres == NS_OK);

    nsres = nsISeekableStream_Seek(seekable_stream, NS_SEEK_SET, 0);
    assert(nsres == NS_OK);

    nsISeekableStream_Release(seekable_stream);

    nsIInputStream_AddRef(stream);
    request_data->post_stream = stream;
    TRACE("post_data = %s\n", debugstr_an(request_data->post_data, request_data->post_data_len));
    return S_OK;
}

static HRESULT on_start_nsrequest(nsChannelBSC *This)
{
    nsresult nsres;

    /* Async request can be cancelled before we got to it */
    if(NS_FAILED(This->nschannel->status))
        return E_ABORT; /* FIXME: map status to HRESULT */

    This->nschannel->binding = This;

    /* FIXME: it's needed for http connections from BindToObject. */
    if(!This->nschannel->response_status)
        This->nschannel->response_status = 200;

    nsres = nsIStreamListener_OnStartRequest(This->nslistener,
            (nsIRequest*)&This->nschannel->nsIHttpChannel_iface, This->nscontext);
    if(NS_FAILED(nsres)) {
        FIXME("OnStartRequest failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(This->is_doc_channel) {
        HRESULT hres;

        if(!This->bsc.window)
            return E_ABORT; /* Binding aborted in OnStartRequest call. */
        hres = update_window_doc(This->bsc.window);
        if(FAILED(hres))
            return hres;

        if(This->bsc.binding)
            process_document_response_headers(This->bsc.window->doc, This->bsc.binding);
        if(This->bsc.window->base.outer_window->readystate != READYSTATE_LOADING &&
           This->bsc.window->base.outer_window->browser->doc)
            set_ready_state(This->bsc.window->base.outer_window, READYSTATE_LOADING);
    }

    return S_OK;
}

static void on_stop_nsrequest(nsChannelBSC *This, HRESULT result)
{
    nsresult nsres, request_result;

    switch(result) {
    case S_OK:
        request_result = NS_OK;
        break;
    case E_ABORT:
        request_result = NS_BINDING_ABORTED;
        break;
    default:
        request_result = NS_ERROR_FAILURE;
    }

    if(This->nslistener) {
        nsres = nsIStreamListener_OnStopRequest(This->nslistener,
                 (nsIRequest*)&This->nschannel->nsIHttpChannel_iface, This->nscontext,
                 request_result);
        if(NS_FAILED(nsres))
            WARN("OnStopRequest failed: %08lx\n", nsres);
    }

    if(This->nschannel) {
        if(This->nschannel->load_group) {
            nsres = nsILoadGroup_RemoveRequest(This->nschannel->load_group,
                    (nsIRequest*)&This->nschannel->nsIHttpChannel_iface, NULL, request_result);
            if(NS_FAILED(nsres))
                ERR("RemoveRequest failed: %08lx\n", nsres);
        }
        if(This->nschannel->binding == This)
            This->nschannel->binding = NULL;
    }
}

static void notify_progress(nsChannelBSC *This)
{
    nsChannel *nschannel = This->nschannel;
    nsIProgressEventSink *sink = NULL;
    nsresult nsres;

    if(!nschannel)
        return;

    if(nschannel->notif_callback)
        if(NS_FAILED(nsIInterfaceRequestor_GetInterface(nschannel->notif_callback, &IID_nsIProgressEventSink, (void**)&sink)))
            sink = NULL;

    if(!sink && nschannel->load_group) {
        nsIRequestObserver *req_observer;

        if(NS_SUCCEEDED(nsILoadGroup_GetGroupObserver(nschannel->load_group, &req_observer)) && req_observer) {
            nsres = nsIRequestObserver_QueryInterface(req_observer, &IID_nsIProgressEventSink, (void**)&sink);
            nsIRequestObserver_Release(req_observer);
            if(NS_FAILED(nsres))
                sink = NULL;
        }
    }

    if(sink) {
        nsIProgressEventSink_OnProgress(sink, (nsIRequest*)&nschannel->nsIHttpChannel_iface, This->nscontext,
                                        This->progress, (This->total == ~0) ? -1 : This->total);
        nsIProgressEventSink_Release(sink);
    }
}

static HRESULT read_stream_data(nsChannelBSC *This, IStream *stream)
{
    DWORD read;
    nsresult nsres;
    HRESULT hres;

    if(!This->response_processed) {
        IWinInetHttpInfo *wininet_info;

        if(This->is_doc_channel)
            This->bsc.window->response_start_time = get_time_stamp();

        This->response_processed = TRUE;
        if(This->bsc.binding) {
            hres = IBinding_QueryInterface(This->bsc.binding, &IID_IWinInetHttpInfo, (void**)&wininet_info);
            if(SUCCEEDED(hres)) {
                query_http_info(This, wininet_info);
                IWinInetHttpInfo_Release(wininet_info);
            }
        }
    }

    if(!This->nschannel)
        return S_OK;

    if(!This->nslistener) {
        BYTE buf[1024];

        do {
            hres = read_stream(&This->bsc, stream, buf, sizeof(buf), &read);
        }while(hres == S_OK && read);

        return S_OK;
    }

    if(!This->nsstream) {
        This->nsstream = create_nsprotocol_stream();
        if(!This->nsstream)
            return E_OUTOFMEMORY;
    }

    do {
        BOOL first_read = !This->bsc.read;

        hres = read_stream(&This->bsc, stream, This->nsstream->buf+This->nsstream->buf_size,
                sizeof(This->nsstream->buf)-This->nsstream->buf_size, &read);
        if(!read)
            break;

        This->nsstream->buf_size += read;

        if(first_read) {
            switch(This->bsc.bom) {
            case BOM_UTF8:
                This->nschannel->charset = strdup("utf-8");
                break;
            case BOM_UTF16:
                This->nschannel->charset = strdup("utf-16");
            case BOM_NONE:
                /* FIXME: Get charset from HTTP headers */;
            }

            if(!This->nschannel->content_type) {
                WCHAR *mime;

                hres = FindMimeFromData(NULL, NULL, This->nsstream->buf, This->nsstream->buf_size,
                        This->is_doc_channel ? L"text/html" : NULL, 0, &mime, 0);
                if(FAILED(hres))
                    return hres;

                TRACE("Found MIME %s\n", debugstr_w(mime));

                This->nschannel->content_type = strdupWtoA(mime);
                CoTaskMemFree(mime);
                if(!This->nschannel->content_type)
                    return E_OUTOFMEMORY;
            }

            hres = on_start_nsrequest(This);
            if(FAILED(hres))
                return hres;
        }

        notify_progress(This);

        nsres = nsIStreamListener_OnDataAvailable(This->nslistener,
                (nsIRequest*)&This->nschannel->nsIHttpChannel_iface, This->nscontext,
                &This->nsstream->nsIInputStream_iface, This->bsc.read-This->nsstream->buf_size,
                This->nsstream->buf_size);
        if(NS_FAILED(nsres)) {
            WARN("OnDataAvailable failed: %08lx\n", nsres);
            return map_nsresult(nsres);
        }

        if(This->nsstream->buf_size == sizeof(This->nsstream->buf)) {
            ERR("buffer is full\n");
            break;
        }
    }while(hres == S_OK);

    return S_OK;
}

typedef struct {
    nsIAsyncVerifyRedirectCallback nsIAsyncVerifyRedirectCallback_iface;

    LONG ref;

    nsChannel *nschannel;
    nsChannelBSC *bsc;
} nsRedirectCallback;

static nsRedirectCallback *impl_from_nsIAsyncVerifyRedirectCallback(nsIAsyncVerifyRedirectCallback *iface)
{
    return CONTAINING_RECORD(iface, nsRedirectCallback, nsIAsyncVerifyRedirectCallback_iface);
}

static nsresult NSAPI nsAsyncVerifyRedirectCallback_QueryInterface(nsIAsyncVerifyRedirectCallback *iface,
        nsIIDRef riid, void **result)
{
    nsRedirectCallback *This = impl_from_nsIAsyncVerifyRedirectCallback(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIAsyncVerifyRedirectCallback_iface;
    }else if(IsEqualGUID(&IID_nsIAsyncVerifyRedirectCallback, riid)) {
        TRACE("(%p)->(IID_nsIAsyncVerifyRedirectCallback %p)\n", This, result);
        *result = &This->nsIAsyncVerifyRedirectCallback_iface;
    }else {
        *result = NULL;
        WARN("unimplemented iface %s\n", debugstr_guid(riid));
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsAsyncVerifyRedirectCallback_AddRef(nsIAsyncVerifyRedirectCallback *iface)
{
    nsRedirectCallback *This = impl_from_nsIAsyncVerifyRedirectCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsAsyncVerifyRedirectCallback_Release(nsIAsyncVerifyRedirectCallback *iface)
{
    nsRedirectCallback *This = impl_from_nsIAsyncVerifyRedirectCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IBindStatusCallback_Release(&This->bsc->bsc.IBindStatusCallback_iface);
        nsIHttpChannel_Release(&This->nschannel->nsIHttpChannel_iface);
        free(This);
    }

    return ref;
}

static nsresult NSAPI nsAsyncVerifyRedirectCallback_OnRedirectVerifyCallback(nsIAsyncVerifyRedirectCallback *iface, nsresult result)
{
    nsRedirectCallback *This = impl_from_nsIAsyncVerifyRedirectCallback(iface);
    nsChannel *old_nschannel;
    nsresult nsres;

    TRACE("(%p)->(%08lx)\n", This, result);

    old_nschannel = This->bsc->nschannel;
    nsIHttpChannel_AddRef(&This->nschannel->nsIHttpChannel_iface);
    This->bsc->nschannel = This->nschannel;

    if(This->nschannel->load_group) {
        nsres = nsILoadGroup_AddRequest(This->nschannel->load_group, (nsIRequest*)&This->nschannel->nsIHttpChannel_iface,
                NULL);
        if(NS_FAILED(nsres))
            ERR("AddRequest failed: %08lx\n", nsres);
    }

    if(This->bsc->is_doc_channel && This->bsc->bsc.window && This->bsc->bsc.window->base.outer_window) {
        IUri *uri = nsuri_get_uri(This->nschannel->uri);

        if(uri) {
            set_current_uri(This->bsc->bsc.window->base.outer_window, uri);
            IUri_Release(uri);
        }else {
            WARN("Could not get IUri from nsWineURI\n");
        }
    }

    if(old_nschannel) {
        if(old_nschannel->load_group) {
            nsres = nsILoadGroup_RemoveRequest(old_nschannel->load_group,
                    (nsIRequest*)&old_nschannel->nsIHttpChannel_iface, NULL, NS_OK);
            if(NS_FAILED(nsres))
                ERR("RemoveRequest failed: %08lx\n", nsres);
        }
        nsIHttpChannel_Release(&old_nschannel->nsIHttpChannel_iface);
    }

    return NS_OK;
}

static const nsIAsyncVerifyRedirectCallbackVtbl nsAsyncVerifyRedirectCallbackVtbl = {
    nsAsyncVerifyRedirectCallback_QueryInterface,
    nsAsyncVerifyRedirectCallback_AddRef,
    nsAsyncVerifyRedirectCallback_Release,
    nsAsyncVerifyRedirectCallback_OnRedirectVerifyCallback
};

static HRESULT create_redirect_callback(nsChannel *nschannel, nsChannelBSC *bsc, nsRedirectCallback **ret)
{
    nsRedirectCallback *callback;

    callback = malloc(sizeof(*callback));
    if(!callback)
        return E_OUTOFMEMORY;

    callback->nsIAsyncVerifyRedirectCallback_iface.lpVtbl = &nsAsyncVerifyRedirectCallbackVtbl;
    callback->ref = 1;

    nsIHttpChannel_AddRef(&nschannel->nsIHttpChannel_iface);
    callback->nschannel = nschannel;

    IBindStatusCallback_AddRef(&bsc->bsc.IBindStatusCallback_iface);
    callback->bsc = bsc;

    *ret = callback;
    return S_OK;
}

static inline nsChannelBSC *nsChannelBSC_from_BSCallback(BSCallback *iface)
{
    return CONTAINING_RECORD(iface, nsChannelBSC, bsc);
}

static void nsChannelBSC_destroy(BSCallback *bsc)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);

    if(This->nschannel) {
        if(This->nschannel->binding == This)
            This->nschannel->binding = NULL;
        nsIHttpChannel_Release(&This->nschannel->nsIHttpChannel_iface);
    }
    if(This->nslistener)
        nsIStreamListener_Release(This->nslistener);
    if(This->nscontext)
        nsISupports_Release(This->nscontext);
    if(This->nsstream)
        nsIInputStream_Release(&This->nsstream->nsIInputStream_iface);
    free(This);
}

static HRESULT nsChannelBSC_start_binding(BSCallback *bsc)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);

    if(This->is_doc_channel) {
        DWORD flags = This->bsc.window->base.outer_window->load_flags;

        if(flags & BINDING_FROMHIST)
            This->bsc.window->navigation_type = 2;  /* TYPE_BACK_FORWARD */
        if(flags & BINDING_REFRESH)
            This->bsc.window->navigation_type = 1;  /* TYPE_RELOAD */

        This->bsc.window->base.outer_window->base.inner_window->doc->skip_mutation_notif = FALSE;
        This->bsc.window->navigation_start_time = get_time_stamp();
    }

    return S_OK;
}

static HRESULT nsChannelBSC_init_bindinfo(BSCallback *bsc)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);
    nsChannel *nschannel = This->nschannel;
    GeckoBrowser *browser;
    HRESULT hres;

    if(This->is_doc_channel && This->bsc.window && This->bsc.window->base.outer_window
       && (browser = This->bsc.window->base.outer_window->browser) && browser->doc) {
        if(browser->doc->hostinfo.dwFlags & DOCHOSTUIFLAG_ENABLE_REDIRECT_NOTIFICATION)
            This->bsc.bindinfo_options |= BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS;
    }

    if(nschannel && nschannel->post_data_stream) {
        hres = read_post_data_stream(nschannel->post_data_stream, nschannel->post_data_contains_headers,
                &nschannel->request_headers, &This->bsc.request_data);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

typedef struct {
    task_t header;
    nsChannelBSC *bsc;
} stop_request_task_t;

static void stop_request_proc(task_t *_task)
{
    stop_request_task_t *task = (stop_request_task_t*)_task;

    TRACE("(%p)\n", task->bsc);

    list_remove(&task->bsc->bsc.entry);
    list_init(&task->bsc->bsc.entry);
    on_stop_nsrequest(task->bsc, S_OK);
}

static void stop_request_task_destr(task_t *_task)
{
    stop_request_task_t *task = (stop_request_task_t*)_task;

    IBindStatusCallback_Release(&task->bsc->bsc.IBindStatusCallback_iface);
}

static HRESULT async_stop_request(nsChannelBSC *This)
{
    HTMLInnerWindow *window = This->bsc.window;
    stop_request_task_t *task;
    HRESULT hres;

    task = malloc(sizeof(*task));
    if(!task)
        return E_OUTOFMEMORY;

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    if(!This->bsc.read) {
        TRACE("No data read, calling OnStartRequest\n");
        on_start_nsrequest(This);
    }

    IBindStatusCallback_AddRef(&This->bsc.IBindStatusCallback_iface);
    task->bsc = This;

    hres = push_task(&task->header, stop_request_proc, stop_request_task_destr, window->task_magic);
    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return hres;
}

static void handle_navigation_error(nsChannelBSC *This, DWORD result)
{
    HTMLOuterWindow *outer_window;
    HTMLDocumentObj *doc;
    BOOL is_error_url;
    SAFEARRAY *sa;
    SAFEARRAYBOUND bound;
    VARIANT var, varOut;
    LONG ind;
    BSTR unk;
    HRESULT hres;

    if(!This->is_doc_channel || !This->bsc.window || !This->bsc.window->base.outer_window
       || !This->bsc.window->base.outer_window->browser)
        return;

    outer_window = This->bsc.window->base.outer_window;
    doc = outer_window->browser->doc;
    if(!doc->doc_object_service || !doc->client)
        return;

    hres = IDocObjectService_IsErrorUrl(doc->doc_object_service,
            outer_window->url, &is_error_url);
    if(FAILED(hres) || is_error_url)
        return;

    if(!doc->client_cmdtrg)
        return;

    bound.lLbound = 0;
    bound.cElements = 8;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
    if(!sa)
        return;

    ind = 0;
    V_VT(&var) = VT_I4;
    V_I4(&var) = result;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 1;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = outer_window->url;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 3;
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)&outer_window->base.IHTMLWindow2_iface;
    SafeArrayPutElement(sa, &ind, &var);

    /* FIXME: what are the following fields for? */
    ind = 2;
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = NULL;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 4;
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = FALSE;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 5;
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = FALSE;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 6;
    V_VT(&var) = VT_BSTR;
    unk = SysAllocString(NULL);
    V_BSTR(&var) = unk;
    SafeArrayPutElement(sa, &ind, &var);

    ind = 7;
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = NULL;
    SafeArrayPutElement(sa, &ind, &var);

    V_VT(&var) = VT_ARRAY;
    V_ARRAY(&var) = sa;
    V_VT(&varOut) = VT_BOOL;
    V_BOOL(&varOut) = VARIANT_TRUE;
    IOleCommandTarget_Exec(doc->client_cmdtrg, &CGID_DocHostCmdPriv, 1, 0, &var, FAILED(hres)?NULL:&varOut);

    SysFreeString(unk);
    SafeArrayDestroy(sa);
}

static HRESULT nsChannelBSC_stop_binding(BSCallback *bsc, HRESULT result)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);

    if(This->is_doc_channel && This->bsc.window) {
        This->bsc.window->response_end_time = get_time_stamp();
        if(result != E_ABORT) {
            if(FAILED(result))
                handle_navigation_error(This, result);
            else if(This->nschannel) {
                result = async_stop_request(This);
                if(SUCCEEDED(result))
                    return S_OK;
            }
        }
    }

    on_stop_nsrequest(This, result);
    return S_OK;
}

static HRESULT nsChannelBSC_read_data(BSCallback *bsc, IStream *stream)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);

    return read_stream_data(This, stream);
}

static HRESULT handle_redirect(nsChannelBSC *This, const WCHAR *new_url)
{
    nsRedirectCallback *callback;
    nsIChannelEventSink *sink;
    nsChannel *new_channel;
    IMoniker *mon;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(new_url));

    hres = CreateURLMoniker(NULL, new_url, &mon);
    if(FAILED(hres))
        return hres;
    IMoniker_Release(This->bsc.mon);
    This->bsc.mon = mon;

    if(!This->nschannel || !This->nschannel->notif_callback)
        return S_OK;

    nsres = nsIInterfaceRequestor_GetInterface(This->nschannel->notif_callback, &IID_nsIChannelEventSink, (void**)&sink);
    if(NS_FAILED(nsres))
        return S_OK;

    hres = create_redirect_nschannel(new_url, This->nschannel, &new_channel);
    if(SUCCEEDED(hres)) {
        TRACE("%p %p->%p\n", This, This->nschannel, new_channel);

        hres = create_redirect_callback(new_channel, This, &callback);
        nsIHttpChannel_Release(&new_channel->nsIHttpChannel_iface);
    }

    if(SUCCEEDED(hres)) {
        nsres = nsIChannelEventSink_AsyncOnChannelRedirect(sink, (nsIChannel*)&This->nschannel->nsIHttpChannel_iface,
                (nsIChannel*)&callback->nschannel->nsIHttpChannel_iface, REDIRECT_TEMPORARY, /* FIXME */
                &callback->nsIAsyncVerifyRedirectCallback_iface);

        if(NS_FAILED(nsres))
            FIXME("AsyncOnChannelRedirect failed: %08lx\n", hres);
        else if(This->nschannel != callback->nschannel)
            FIXME("nschannel not updated\n");

        nsIAsyncVerifyRedirectCallback_Release(&callback->nsIAsyncVerifyRedirectCallback_iface);
    }

    nsIChannelEventSink_Release(sink);
    return hres;
}

static BOOL is_supported_doc_mime(const WCHAR *mime)
{
    char *nscat, *mimea;
    BOOL ret;

    mimea = strdupWtoA(mime);
    if(!mimea)
        return FALSE;

    nscat = get_nscategory_entry("Gecko-Content-Viewers", mimea);

    ret = nscat != NULL && !strcmp(nscat, "@mozilla.org/content/document-loader-factory;1");

    free(mimea);
    nsfree(nscat);
    return ret;
}

static IUri *get_moniker_uri(IMoniker *mon)
{
    IUriContainer *uri_container;
    IUri *ret = NULL;
    HRESULT hres;

    hres = IMoniker_QueryInterface(mon, &IID_IUriContainer, (void**)&uri_container);
    if(SUCCEEDED(hres)) {
        hres = IUriContainer_GetIUri(uri_container, &ret);
        IUriContainer_Release(uri_container);
        if(FAILED(hres))
            return NULL;
    }else {
        FIXME("No IUriContainer\n");
    }

    return ret;
}

static void handle_extern_mime_navigation(nsChannelBSC *This)
{
    IWebBrowserPriv2IE9 *webbrowser_priv;
    IOleCommandTarget *cmdtrg;
    HTMLDocumentObj *doc_obj;
    IBindCtx *bind_ctx;
    IUri *uri;
    VARIANT flags;
    HRESULT hres;

    if(!This->bsc.window || !This->bsc.window->base.outer_window || !This->bsc.window->base.outer_window->browser)
        return;

    doc_obj = This->bsc.window->base.outer_window->browser->doc;
    IUnknown_AddRef(doc_obj->outer_unk);

    hres = IOleClientSite_QueryInterface(doc_obj->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 62, 0, NULL, NULL);
        IOleCommandTarget_Release(cmdtrg);
    }

    set_document_navigation(doc_obj, FALSE);

    if(!doc_obj->webbrowser) {
        FIXME("unimplemented in non-webbrowser mode\n");
        goto done;
    }

    uri = get_moniker_uri(This->bsc.mon);
    if(!uri)
        goto done;

    hres = CreateBindCtx(0, &bind_ctx);
    if(FAILED(hres)) {
        IUri_Release(uri);
        goto done;
    }

    V_VT(&flags) = VT_I4;
    V_I4(&flags) = navHyperlink;

    hres = IUnknown_QueryInterface(doc_obj->webbrowser, &IID_IWebBrowserPriv2IE8, (void**)&webbrowser_priv);
    if(SUCCEEDED(hres)) {
        hres = IWebBrowserPriv2IE9_NavigateWithBindCtx2(webbrowser_priv, uri, &flags, NULL, NULL, NULL, bind_ctx, NULL, 0);
        IWebBrowserPriv2IE9_Release(webbrowser_priv);
    }else {
        IWebBrowserPriv *webbrowser_priv_old;
        VARIANT uriv;

        hres = IUnknown_QueryInterface(doc_obj->webbrowser, &IID_IWebBrowserPriv, (void**)&webbrowser_priv_old);
        if(SUCCEEDED(hres)) {
            V_VT(&uriv) = VT_BSTR;
            V_BSTR(&uriv) = NULL;
            hres = IUri_GetDisplayUri(uri, &V_BSTR(&uriv));

            if(hres == S_OK)
                hres = IWebBrowserPriv_NavigateWithBindCtx(webbrowser_priv_old, &uriv, &flags, NULL, NULL, NULL, bind_ctx, NULL);

            SysFreeString(V_BSTR(&uriv));
            IWebBrowserPriv_Release(webbrowser_priv_old);
        }
    }

    IUri_Release(uri);

done:
    IUnknown_Release(doc_obj->outer_unk);
}

static HRESULT nsChannelBSC_on_progress(BSCallback *bsc, ULONG progress, ULONG total, ULONG status_code, LPCWSTR status_text)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);

    switch(status_code) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        if(This->is_doc_channel && !is_supported_doc_mime(status_text)) {
            FIXME("External MIME: %s\n", debugstr_w(status_text));

            handle_extern_mime_navigation(This);

            This->nschannel = NULL;
        }

        if(!This->nschannel ||
           (This->nschannel->content_type && !(This->nschannel->load_flags & LOAD_CALL_CONTENT_SNIFFERS)))
            return S_OK;

        free(This->nschannel->content_type);
        This->nschannel->content_type = strdupWtoA(status_text);
        break;
    case BINDSTATUS_REDIRECTING:
        if(This->is_doc_channel) {
            This->bsc.window->redirect_count++;
            if(!This->bsc.window->redirect_time)
                This->bsc.window->redirect_time = get_time_stamp();
        }
        return handle_redirect(This, status_text);
    case BINDSTATUS_FINDINGRESOURCE:
        if(This->is_doc_channel && !This->bsc.window->dns_lookup_time)
            This->bsc.window->dns_lookup_time = get_time_stamp();
        break;
    case BINDSTATUS_CONNECTING:
        if(This->is_doc_channel)
            This->bsc.window->connect_time = get_time_stamp();
        break;
    case BINDSTATUS_SENDINGREQUEST:
        if(This->is_doc_channel)
            This->bsc.window->request_time = get_time_stamp();
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA: {
        IWinInetHttpInfo *http_info;
        DWORD status, size = sizeof(DWORD);
        HRESULT hres;

        if(This->bsc.binding) {
            hres = IBinding_QueryInterface(This->bsc.binding, &IID_IWinInetHttpInfo, (void**)&http_info);
            if(SUCCEEDED(hres)) {
                hres = IWinInetHttpInfo_QueryInfo(http_info,
                        HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL, NULL);
                IWinInetHttpInfo_Release(http_info);
                if(SUCCEEDED(hres) && status != HTTP_STATUS_OK)
                    handle_navigation_error(This, status);
            }
        }
        /* fall through */
    }
    case BINDSTATUS_DOWNLOADINGDATA:
    case BINDSTATUS_ENDDOWNLOADDATA:
        /* Defer it to just before calling OnDataAvailable, otherwise it can have wrong state */
        This->progress = progress;
        This->total = total;
        break;
    }

    return S_OK;
}

static HRESULT process_response_status_text(const WCHAR *header, const WCHAR *header_end, char **status_text)
{
    header = wcschr(header + 1, ' ');
    if(!header || header >= header_end)
        return E_FAIL;
    header = wcschr(header + 1, ' ');
    if(!header || header >= header_end)
        header = header_end;
    else
        ++header;

    *status_text = strndupWtoU(header, header_end - header);

    if(!*status_text)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT nsChannelBSC_on_response(BSCallback *bsc, DWORD response_code,
        LPCWSTR response_headers)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);
    char *str;
    HRESULT hres;

    if(This->is_doc_channel)
        This->bsc.window->response_start_time = get_time_stamp();

    This->response_processed = TRUE;
    This->nschannel->response_status = response_code;

    if(response_headers) {
        const WCHAR *headers;

        headers = wcschr(response_headers, '\r');
        hres = process_response_status_text(response_headers, headers, &str);
        if(FAILED(hres)) {
            WARN("parsing headers failed: %08lx\n", hres);
            return hres;
        }

        free(This->nschannel->response_status_text);
        This->nschannel->response_status_text = str;

        if(headers && headers[1] == '\n') {
            headers += 2;
            hres = process_response_headers(This, headers);
            if(FAILED(hres)) {
                WARN("parsing headers failed: %08lx\n", hres);
                return hres;
            }
        }
    }

    return S_OK;
}

static HRESULT nsChannelBSC_beginning_transaction(BSCallback *bsc, WCHAR **additional_headers)
{
    nsChannelBSC *This = nsChannelBSC_from_BSCallback(bsc);
    http_header_t *iter;
    DWORD len = 0;
    WCHAR *ptr;

    if(!This->nschannel)
        return S_FALSE;

    LIST_FOR_EACH_ENTRY(iter, &This->nschannel->request_headers, http_header_t, entry) {
        if(wcscmp(iter->header, L"Content-Length"))
            len += lstrlenW(iter->header) + 2 /* ": " */ + lstrlenW(iter->data) + 2 /* "\r\n" */;
    }

    if(!len)
        return S_OK;

    *additional_headers = ptr = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
    if(!ptr)
        return E_OUTOFMEMORY;

    LIST_FOR_EACH_ENTRY(iter, &This->nschannel->request_headers, http_header_t, entry) {
        if(!wcscmp(iter->header, L"Content-Length"))
            continue;

        len = lstrlenW(iter->header);
        memcpy(ptr, iter->header, len*sizeof(WCHAR));
        ptr += len;

        *ptr++ = ':';
        *ptr++ = ' ';

        len = lstrlenW(iter->data);
        memcpy(ptr, iter->data, len*sizeof(WCHAR));
        ptr += len;

        *ptr++ = '\r';
        *ptr++ = '\n';
    }

    *ptr = 0;

    return S_OK;
}

static const BSCallbackVtbl nsChannelBSCVtbl = {
    nsChannelBSC_destroy,
    nsChannelBSC_init_bindinfo,
    nsChannelBSC_start_binding,
    nsChannelBSC_stop_binding,
    nsChannelBSC_read_data,
    nsChannelBSC_on_progress,
    nsChannelBSC_on_response,
    nsChannelBSC_beginning_transaction
};

HRESULT create_channelbsc(IMoniker *mon, const WCHAR *headers, BYTE *post_data, DWORD post_data_size,
        BOOL is_doc_binding, nsChannelBSC **retval)
{
    nsChannelBSC *ret;
    DWORD bindf;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    if(post_data_size)
        bindf |= BINDF_FORMS_SUBMIT | BINDF_PRAGMA_NO_CACHE | BINDF_HYPERLINK | BINDF_GETNEWESTVERSION;

    init_bscallback(&ret->bsc, &nsChannelBSCVtbl, mon, bindf);
    ret->is_doc_channel = is_doc_binding;

    if(headers) {
        ret->bsc.request_data.headers = wcsdup(headers);
        if(!ret->bsc.request_data.headers) {
            IBindStatusCallback_Release(&ret->bsc.IBindStatusCallback_iface);
            return E_OUTOFMEMORY;
        }
    }

    if(post_data) {
        ret->bsc.request_data.post_data = GlobalAlloc(0, post_data_size+1);
        if(!ret->bsc.request_data.post_data) {
            release_request_data(&ret->bsc.request_data);
            IBindStatusCallback_Release(&ret->bsc.IBindStatusCallback_iface);
            return E_OUTOFMEMORY;
        }

        memcpy(ret->bsc.request_data.post_data, post_data, post_data_size);
        ((BYTE*)ret->bsc.request_data.post_data)[post_data_size] = 0;
        ret->bsc.request_data.post_data_len = post_data_size;
    }

    TRACE("created %p\n", ret);
    *retval = ret;
    return S_OK;
}

typedef struct {
    task_t header;
    DWORD flags;
    HTMLOuterWindow *window;
    HTMLInnerWindow *pending_window;
} start_doc_binding_task_t;

static void start_doc_binding_proc(task_t *_task)
{
    start_doc_binding_task_t *task = (start_doc_binding_task_t*)_task;
    HTMLOuterWindow *window = task->window;

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    set_current_mon(window, task->pending_window->bscallback->bsc.mon, task->flags);
    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);

    start_binding(task->pending_window, &task->pending_window->bscallback->bsc, NULL);
}

static void start_doc_binding_task_destr(task_t *_task)
{
    start_doc_binding_task_t *task = (start_doc_binding_task_t*)_task;

    IHTMLWindow2_Release(&task->pending_window->base.IHTMLWindow2_iface);
}

HRESULT async_start_doc_binding(HTMLOuterWindow *window, HTMLInnerWindow *pending_window, DWORD flags)
{
    start_doc_binding_task_t *task;

    TRACE("%p\n", pending_window);

    task = malloc(sizeof(start_doc_binding_task_t));
    if(!task)
        return E_OUTOFMEMORY;

    task->flags = flags;
    task->window = window;
    task->pending_window = pending_window;
    IHTMLWindow2_AddRef(&pending_window->base.IHTMLWindow2_iface);

    return push_task(&task->header, start_doc_binding_proc, start_doc_binding_task_destr, pending_window->task_magic);
}

void abort_window_bindings(HTMLInnerWindow *window)
{
    BSCallback *iter;

    remove_target_tasks(window->task_magic);

    while(!list_empty(&window->bindings)) {
        iter = LIST_ENTRY(window->bindings.next, BSCallback, entry);

        TRACE("Aborting %p\n", iter);

        IBindStatusCallback_AddRef(&iter->IBindStatusCallback_iface);

        if(iter->binding) {
            IBinding *binding = iter->binding;

            /* Abort can end up calling our OnStopBinding, which releases the binding. */
            IBinding_AddRef(binding);
            IBinding_Abort(binding);
            IBinding_Release(binding);
        }else {
            iter->vtbl->stop_binding(iter, E_ABORT);
        }

        iter->window = NULL;
        list_remove(&iter->entry);
        list_init(&iter->entry);

        IBindStatusCallback_Release(&iter->IBindStatusCallback_iface);
    }

    if(window->bscallback) {
        IBindStatusCallback_Release(&window->bscallback->bsc.IBindStatusCallback_iface);
        window->bscallback = NULL;
    }

    unlink_ref(&window->mon);
}

HRESULT channelbsc_load_stream(HTMLInnerWindow *pending_window, IMoniker *mon, IStream *stream)
{
    HTMLOuterWindow *window = pending_window->base.outer_window;
    nsChannelBSC *bscallback = pending_window->bscallback;
    HRESULT hres = S_OK;

    if(!bscallback->nschannel) {
        ERR("NULL nschannel\n");
        return E_FAIL;
    }

    bscallback->nschannel->content_type = strdup("text/html");
    if(!bscallback->nschannel->content_type)
        return E_OUTOFMEMORY;

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    set_current_mon(window, mon, 0);
    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);

    bscallback->bsc.window = pending_window;
    if(stream)
        hres = read_stream_data(bscallback, stream);
    if(SUCCEEDED(hres))
        hres = async_stop_request(bscallback);
    if(FAILED(hres))
        IBindStatusCallback_OnStopBinding(&bscallback->bsc.IBindStatusCallback_iface, hres, NULL);

    return hres;
}

void channelbsc_set_channel(nsChannelBSC *This, nsChannel *channel, nsIStreamListener *listener, nsISupports *context)
{
    nsIHttpChannel_AddRef(&channel->nsIHttpChannel_iface);
    This->nschannel = channel;

    nsIStreamListener_AddRef(listener);
    This->nslistener = listener;

    if(context) {
        nsISupports_AddRef(context);
        This->nscontext = context;
    }

    if(This->bsc.request_data.headers) {
        HRESULT hres;

        hres = parse_headers(This->bsc.request_data.headers, &channel->request_headers);
        free(This->bsc.request_data.headers);
        This->bsc.request_data.headers = NULL;
        if(FAILED(hres))
            WARN("parse_headers failed: %08lx\n", hres);
    }
}

typedef struct {
    task_t header;
    HTMLOuterWindow *window;
    IUri *uri;
} navigate_javascript_task_t;

static void navigate_javascript_proc(task_t *_task)
{
    navigate_javascript_task_t *task = (navigate_javascript_task_t*)_task;
    HTMLInnerWindow *inner_window = task->window->base.inner_window;
    HTMLOuterWindow *window = task->window;
    HTMLDocumentObj *doc = NULL;
    BSTR code = NULL;
    VARIANT v;
    HRESULT hres;

    window->readystate = READYSTATE_COMPLETE;
    if(window->browser) {
        doc = window->browser->doc;
        IUnknown_AddRef(doc->outer_unk);
    }
    IHTMLWindow2_AddRef(&inner_window->base.IHTMLWindow2_iface);

    hres = IUri_GetPath(task->uri, &code);
    if(hres != S_OK) {
        SysFreeString(code);
        goto done;
    }

    hres = UrlUnescapeW(code, NULL, NULL, URL_UNESCAPE_INPLACE);
    if(FAILED(hres)) {
        SysFreeString(code);
        goto done;
    }

    if(doc)
        set_download_state(doc, 1);

    V_VT(&v) = VT_EMPTY;
    hres = exec_script(inner_window, code, L"jscript", &v);
    SysFreeString(code);
    if(SUCCEEDED(hres) && V_VT(&v) != VT_EMPTY) {
        FIXME("javascirpt URL returned %s\n", debugstr_variant(&v));
        VariantClear(&v);
    }

    if(doc) {
        if(doc->view_sink)
            IAdviseSink_OnViewChange(doc->view_sink, DVASPECT_CONTENT, -1);

        set_download_state(doc, 0);
    }

done:
    IHTMLWindow2_Release(&inner_window->base.IHTMLWindow2_iface);
    if(doc)
        IUnknown_Release(doc->outer_unk);
}

static void navigate_javascript_task_destr(task_t *_task)
{
    navigate_javascript_task_t *task = (navigate_javascript_task_t*)_task;

    IUri_Release(task->uri);
}

typedef struct {
    task_t header;
    HTMLOuterWindow *window;
    nsChannelBSC *bscallback;
    DWORD flags;
    IMoniker *mon;
    IUri *uri;
} navigate_task_t;

static void navigate_proc(task_t *_task)
{
    navigate_task_t *task = (navigate_task_t*)_task;
    HTMLOuterWindow *window = task->window;
    HRESULT hres;

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    hres = set_moniker(window, task->mon, task->uri, NULL, task->bscallback, TRUE);
    if(SUCCEEDED(hres)) {
        set_current_mon(window, task->bscallback->bsc.mon, task->flags);
        set_current_uri(window, task->uri);
        start_binding(window->pending_window, &task->bscallback->bsc, NULL);
    }

    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
}

static void navigate_task_destr(task_t *_task)
{
    navigate_task_t *task = (navigate_task_t*)_task;

    IBindStatusCallback_Release(&task->bscallback->bsc.IBindStatusCallback_iface);
    IMoniker_Release(task->mon);
    IUri_Release(task->uri);
}

static HRESULT navigate_fragment(HTMLOuterWindow *window, IUri *uri)
{
    nsIDOMLocation *nslocation;
    nsAString nsfrag_str;
    BSTR frag = NULL;
    WCHAR *selector;
    nsresult nsres;
    HRESULT hres;
    static const WCHAR selector_formatW[] = L"a[id=\"%s\"]";

    set_current_uri(window, uri);

    nsres = nsIDOMWindow_GetLocation(window->nswindow, &nslocation);
    if(FAILED(nsres) || !nslocation)
        return E_FAIL;

    hres = IUri_GetFragment(uri, &frag);
    if(hres != S_OK) {
        SysFreeString(frag);
        nsIDOMLocation_Release(nslocation);
        return FAILED(hres) ? hres : S_OK;
    }

    nsAString_InitDepend(&nsfrag_str, frag);
    nsres = nsIDOMLocation_SetHash(nslocation, &nsfrag_str);
    nsAString_Finish(&nsfrag_str);
    nsIDOMLocation_Release(nslocation);
    if(NS_FAILED(nsres))
        ERR("SetHash failed: %08lx\n", nsres);

    /*
     * IE supports scrolling to anchor elements with "#hash" ids (note that '#' is part of the id),
     * while Gecko scrolls only to elements with "hash" ids. We scroll the page ourselves if
     * a[id="#hash"] element can be found.
     */
    selector = malloc(sizeof(selector_formatW) + SysStringLen(frag) * sizeof(WCHAR));
    if(selector) {
        nsIDOMElement *nselem = NULL;
        nsAString selector_str;

        swprintf(selector, ARRAY_SIZE(selector_formatW)+SysStringLen(frag), selector_formatW, frag);
        nsAString_InitDepend(&selector_str, selector);
        /* NOTE: Gecko doesn't set result to NULL if there is no match, so nselem must be initialized */
        nsres = nsIDOMDocument_QuerySelector(window->base.inner_window->doc->dom_document, &selector_str, &nselem);
        nsAString_Finish(&selector_str);
        free(selector);
        if(NS_SUCCEEDED(nsres) && nselem) {
            nsIDOMHTMLElement *html_elem;

            nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLElement, (void**)&html_elem);
            nsIDOMElement_Release(nselem);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMHTMLElement_ScrollIntoView(html_elem, TRUE, 1);
                nsIDOMHTMLElement_Release(html_elem);
            }
        }
    }

    SysFreeString(frag);

    if(window->browser->doc->doc_object_service) {
        HTMLDocumentObj *doc_obj = window->browser->doc;

        IUnknown_AddRef(doc_obj->outer_unk);
        IDocObjectService_FireNavigateComplete2(doc_obj->doc_object_service, &window->base.IHTMLWindow2_iface, 0x10);
        IDocObjectService_FireDocumentComplete(doc_obj->doc_object_service, &window->base.IHTMLWindow2_iface, 0);
        IUnknown_Release(doc_obj->outer_unk);
    }

    return S_OK;
}

HRESULT super_navigate(HTMLOuterWindow *window, IUri *uri, DWORD flags, const WCHAR *headers, BYTE *post_data, DWORD post_data_size)
{
    HTMLDocumentObj *doc_obj;
    nsChannelBSC *bsc;
    IUri *uri_nofrag;
    IMoniker *mon;
    DWORD scheme;
    HRESULT hres;

    uri_nofrag = get_uri_nofrag(uri);
    if(!uri_nofrag)
        return E_FAIL;

    doc_obj = window->browser->doc;
    IUnknown_AddRef(doc_obj->outer_unk);

    if(doc_obj->client && !(flags & BINDING_REFRESH)) {
        IOleCommandTarget *cmdtrg;

        hres = IOleClientSite_QueryInterface(doc_obj->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
        if(SUCCEEDED(hres)) {
            VARIANT in, out;
            BSTR url_str;

            hres = IUri_GetDisplayUri(uri_nofrag, &url_str);
            if(SUCCEEDED(hres)) {
                V_VT(&in) = VT_BSTR;
                V_BSTR(&in) = url_str;
                V_VT(&out) = VT_BOOL;
                V_BOOL(&out) = VARIANT_TRUE;
                hres = IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 67, 0, &in, &out);
                IOleCommandTarget_Release(cmdtrg);
                if(SUCCEEDED(hres))
                    VariantClear(&out);
                SysFreeString(url_str);
            }
        }
    }

    if(!(flags & BINDING_NOFRAG) && window->uri_nofrag && !post_data_size) {
        BOOL eq;

        hres = IUri_IsEqual(uri_nofrag, window->uri_nofrag, &eq);
        if(SUCCEEDED(hres) && eq) {
            IUri_Release(uri_nofrag);
            TRACE("fragment navigate\n");
            hres = navigate_fragment(window, uri);
            goto done;
        }
    }

    hres = CreateURLMonikerEx2(NULL, uri_nofrag, &mon, URL_MK_UNIFORM);
    IUri_Release(uri_nofrag);
    if(FAILED(hres))
        goto done;

    /* FIXME: Why not set_ready_state? */
    window->readystate = READYSTATE_UNINITIALIZED;

    hres = create_channelbsc(mon, headers, post_data, post_data_size, TRUE, &bsc);
    if(FAILED(hres)) {
        IMoniker_Release(mon);
        goto done;
    }

    prepare_for_binding(doc_obj, mon, flags);

    hres = IUri_GetScheme(uri, &scheme);
    if(hres == S_OK && scheme == URL_SCHEME_JAVASCRIPT) {
        navigate_javascript_task_t *task;

        IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
        IMoniker_Release(mon);

        task = malloc(sizeof(*task));
        if(!task) {
            hres = E_OUTOFMEMORY;
            goto done;
        }

        /* Why silently? */
        window->readystate = READYSTATE_COMPLETE;
        if(!(flags & BINDING_FROMHIST))
            call_docview_84(doc_obj);

        IUri_AddRef(uri);
        task->window = window;
        task->uri = uri;
        hres = push_task(&task->header, navigate_javascript_proc, navigate_javascript_task_destr, window->task_magic);
    }else if(flags & BINDING_SUBMIT) {
        hres = set_moniker(window, mon, uri, NULL, bsc, TRUE);
        if(SUCCEEDED(hres))
            hres = start_binding(window->pending_window, &bsc->bsc, NULL);
        IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
        IMoniker_Release(mon);
    }else {
        navigate_task_t *task;

        task = malloc(sizeof(*task));
        if(!task) {
            IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
            IMoniker_Release(mon);
            hres = E_OUTOFMEMORY;
            goto done;
        }

        /* Silently and repeated when real loading starts? */
        window->readystate = READYSTATE_LOADING;
        if(!(flags & (BINDING_FROMHIST|BINDING_REFRESH)))
            call_docview_84(doc_obj);

        task->window = window;
        task->bscallback = bsc;
        task->flags = flags;
        task->mon = mon;

        IUri_AddRef(uri);
        task->uri = uri;
        hres = push_task(&task->header, navigate_proc, navigate_task_destr, window->task_magic);
    }

done:
    IUnknown_Release(doc_obj->outer_unk);
    return hres;
}

HRESULT navigate_new_window(HTMLOuterWindow *window, IUri *uri, const WCHAR *name, request_data_t *request_data, IHTMLWindow2 **ret)
{
    INewWindowManager *new_window_mgr;
    BSTR display_uri, context_url;
    IWebBrowser2 *web_browser;
    IHTMLWindow2 *new_window;
    IBindCtx *bind_ctx;
    nsChannelBSC *bsc;
    HRESULT hres;

    if(!window->browser)
        return E_UNEXPECTED;

    if (window->browser->doc->client) {
        hres = do_query_service((IUnknown*)window->browser->doc->client, &SID_SNewWindowManager,
                                &IID_INewWindowManager, (void**)&new_window_mgr);
        if (FAILED(hres)) {
            FIXME("No INewWindowManager\n");
            return hres;
        }

        hres = IUri_GetDisplayUri(window->uri_nofrag, &context_url);
        if(FAILED(hres))
            return hres;

        hres = IUri_GetDisplayUri(uri, &display_uri);
        if(FAILED(hres)) {
            SysFreeString(context_url);
            return hres;
        }

        hres = INewWindowManager_EvaluateNewWindow(new_window_mgr, display_uri, name, context_url,
                NULL, FALSE, window->browser->doc->has_popup ? 0 : NWMF_FIRST, 0);
        window->browser->doc->has_popup = TRUE;
        SysFreeString(display_uri);
        SysFreeString(context_url);
        INewWindowManager_Release(new_window_mgr);
        if(FAILED(hres)) {
            if(ret)
                *ret = NULL;
            return S_OK;
        }
    }


    if(request_data)
        hres = create_channelbsc(NULL, request_data->headers,
                request_data->post_data, request_data->post_data_len, FALSE,
                &bsc);
    else
        hres = create_channelbsc(NULL, NULL, NULL, 0, FALSE, &bsc);
    if(FAILED(hres))
        return hres;

    hres = CreateAsyncBindCtx(0, &bsc->bsc.IBindStatusCallback_iface, NULL, &bind_ctx);
    if(FAILED(hres)) {
        IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
        return hres;
    }

    hres = CoCreateInstance(&CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER,
            &IID_IWebBrowser2, (void**)&web_browser);
    if(SUCCEEDED(hres)) {
        ITargetFramePriv2 *target_frame_priv;

        hres = IWebBrowser2_QueryInterface(web_browser, &IID_ITargetFramePriv2, (void**)&target_frame_priv);
        if(SUCCEEDED(hres)) {
            hres = ITargetFramePriv2_AggregatedNavigation2(target_frame_priv,
                    HLNF_DISABLEWINDOWRESTRICTIONS|HLNF_OPENINNEWWINDOW, bind_ctx, &bsc->bsc.IBindStatusCallback_iface,
                    name, uri, L"");
            ITargetFramePriv2_Release(target_frame_priv);

            if(SUCCEEDED(hres))
                hres = do_query_service((IUnknown*)web_browser, &SID_SHTMLWindow, &IID_IHTMLWindow2, (void**)&new_window);
        }
        if(FAILED(hres)) {
            IWebBrowser2_Quit(web_browser);
            IWebBrowser2_Release(web_browser);
        }
    }else {
        WARN("Could not create InternetExplorer instance: %08lx\n", hres);
    }

    IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
    IBindCtx_Release(bind_ctx);
    if(FAILED(hres))
        return hres;

    IWebBrowser2_put_Visible(web_browser, VARIANT_TRUE);
    IWebBrowser2_Release(web_browser);

    if(ret)
        *ret = new_window;
    else
        IHTMLWindow2_Release(new_window);
    return S_OK;
}

HRESULT hlink_frame_navigate(HTMLDocumentObj *doc, LPCWSTR url, nsChannel *nschannel, DWORD hlnf, BOOL *cancel)
{
    IHlinkFrame *hlink_frame;
    nsChannelBSC *callback;
    IBindCtx *bindctx;
    IMoniker *mon;
    IHlink *hlink;
    HRESULT hres;

    *cancel = FALSE;

    hres = do_query_service((IUnknown*)doc->client, &IID_IHlinkFrame, &IID_IHlinkFrame,
            (void**)&hlink_frame);
    if(FAILED(hres))
        return S_OK;

    hres = create_channelbsc(NULL, NULL, NULL, 0, FALSE, &callback);
    if(FAILED(hres)) {
        IHlinkFrame_Release(hlink_frame);
        return hres;
    }

    if(nschannel)
        read_post_data_stream(nschannel->post_data_stream, nschannel->post_data_contains_headers,
                &nschannel->request_headers, &callback->bsc.request_data);

    hres = CreateAsyncBindCtx(0, &callback->bsc.IBindStatusCallback_iface, NULL, &bindctx);
    if(SUCCEEDED(hres))
       hres = CoCreateInstance(&CLSID_StdHlink, NULL, CLSCTX_INPROC_SERVER,
                &IID_IHlink, (LPVOID*)&hlink);

    if(SUCCEEDED(hres))
        hres = CreateURLMoniker(NULL, url, &mon);

    if(SUCCEEDED(hres)) {
        IHlink_SetMonikerReference(hlink, HLINKSETF_TARGET, mon, NULL);

        if(hlnf & HLNF_OPENINNEWWINDOW) {
            IHlink_SetTargetFrameName(hlink, L"_blank"); /* FIXME */
        }

        hres = IHlinkFrame_Navigate(hlink_frame, hlnf, bindctx,
                &callback->bsc.IBindStatusCallback_iface, hlink);
        IMoniker_Release(mon);
        *cancel = hres == S_OK;
        hres = S_OK;
    }

    IHlinkFrame_Release(hlink_frame);
    IBindCtx_Release(bindctx);
    IBindStatusCallback_Release(&callback->bsc.IBindStatusCallback_iface);
    return hres;
}

static HRESULT navigate_uri(HTMLOuterWindow *window, IUri *uri, const WCHAR *display_uri, const request_data_t *request_data,
        DWORD flags)
{
    DWORD post_data_len = request_data ? request_data->post_data_len : 0;
    nsWineURI *nsuri;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(display_uri));

    if(!window->browser)
        return E_UNEXPECTED;

    if(window->browser->doc->webbrowser) {
        void *post_data = post_data_len ? request_data->post_data : NULL;
        const WCHAR *headers = request_data ? request_data->headers : NULL;

        if(!(flags & BINDING_REFRESH)) {
            BSTR frame_name = NULL;
            BOOL cancel = FALSE;

            if(!is_main_content_window(window)) {
                hres = IHTMLWindow2_get_name(&window->base.IHTMLWindow2_iface, &frame_name);
                if(FAILED(hres))
                    return hres;
            }

            hres = IDocObjectService_FireBeforeNavigate2(window->browser->doc->doc_object_service, NULL, display_uri, 0x40,
                    frame_name, post_data, post_data_len ? post_data_len+1 : 0, headers, TRUE, &cancel);
            SysFreeString(frame_name);
            if(SUCCEEDED(hres) && cancel) {
                TRACE("Navigation canceled\n");
                return S_OK;
            }
        }

        if(!window->browser)
            return S_OK;

        if(is_main_content_window(window))
            return super_navigate(window, uri, flags, headers, post_data, post_data_len);
    }

    if(!(flags & BINDING_NOFRAG) && window->uri_nofrag && !post_data_len &&
       compare_uri_ignoring_frag(window->uri_nofrag, uri))
         return navigate_fragment(window, uri);

    if(is_main_content_window(window)) {
        BOOL cancel;

        hres = hlink_frame_navigate(window->base.inner_window->doc->doc_obj, display_uri, NULL, 0, &cancel);
        if(FAILED(hres))
            return hres;

        if(cancel) {
            TRACE("Navigation handled by hlink frame\n");
            return S_OK;
        }
    }

    hres = create_doc_uri(uri, &nsuri);
    if(FAILED(hres))
        return hres;

    hres = load_nsuri(window, nsuri, request_data ? request_data->post_stream : NULL, NULL,
                      (flags & BINDING_REFRESH) ? LOAD_FLAGS_IS_REFRESH : LOAD_FLAGS_NONE);
    nsISupports_Release((nsISupports*)nsuri);
    return hres;
}

HRESULT load_uri(HTMLOuterWindow *window, IUri *uri, DWORD flags)
{
    BSTR display_uri;
    HRESULT hres;

    hres = IUri_GetDisplayUri(uri, &display_uri);
    if(FAILED(hres))
        return hres;

    hres = navigate_uri(window, uri, display_uri, NULL, flags);
    SysFreeString(display_uri);
    return hres;
}

static HRESULT translate_uri(HTMLOuterWindow *window, IUri *orig_uri, BSTR *ret_display_uri, IUri **ret_uri)
{
    IUri *uri = NULL;
    BSTR display_uri;
    HRESULT hres;

    hres = IUri_GetDisplayUri(orig_uri, &display_uri);
    if(FAILED(hres))
        return hres;

    if(window->browser->doc->hostui) {
        OLECHAR *translated_url = NULL;

        hres = IDocHostUIHandler_TranslateUrl(window->browser->doc->hostui, 0, display_uri,
                &translated_url);
        if(hres == S_OK && translated_url) {
            TRACE("%08lx %s -> %s\n", hres, debugstr_w(display_uri), debugstr_w(translated_url));
            SysFreeString(display_uri);
            hres = create_uri(translated_url, 0, &uri);
            CoTaskMemFree(translated_url);
            if(FAILED(hres))
                return hres;

            hres = IUri_GetDisplayUri(uri, &display_uri);
            if(FAILED(hres)) {
                IUri_Release(uri);
                return hres;
            }
        }
    }

    if(!uri) {
        IUri_AddRef(orig_uri);
        uri = orig_uri;
    }

    *ret_display_uri = display_uri;
    *ret_uri = uri;
    return S_OK;
}

HRESULT submit_form(HTMLOuterWindow *window, const WCHAR *target, IUri *submit_uri, nsIInputStream *post_stream)
{
    request_data_t request_data = {NULL};
    HRESULT hres;

    hres = read_post_data_stream(post_stream, TRUE, NULL, &request_data);
    if(FAILED(hres))
        return hres;

    if(window) {
        IUri *uri;
        BSTR display_uri;

        window->readystate_locked++;

        hres = translate_uri(window, submit_uri, &display_uri, &uri);
        if(SUCCEEDED(hres)) {
            hres = navigate_uri(window, uri, display_uri, &request_data, BINDING_NAVIGATED|BINDING_SUBMIT);
            IUri_Release(uri);
            SysFreeString(display_uri);
        }

        window->readystate_locked--;
    }else
        hres = navigate_new_window(window, submit_uri, target, &request_data, NULL);

    release_request_data(&request_data);
    return hres;
}

HRESULT navigate_url(HTMLOuterWindow *window, const WCHAR *new_url, IUri *base_uri, DWORD flags)
{
    IUri *uri, *nav_uri;
    BSTR display_uri;
    HRESULT hres;

    if(!window->browser)
        return E_UNEXPECTED;

    if(new_url && base_uri)
        hres = CoInternetCombineUrlEx(base_uri, new_url, URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                &nav_uri, 0);
    else
        hres = create_uri(new_url, 0, &nav_uri);
    if(FAILED(hres))
        return hres;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    hres = translate_uri(window, nav_uri, &display_uri, &uri);
    IUri_Release(nav_uri);
    if(SUCCEEDED(hres)) {
        hres = navigate_uri(window, uri, display_uri, NULL, flags);
        IUri_Release(uri);
        SysFreeString(display_uri);
    }

    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return hres;
}
