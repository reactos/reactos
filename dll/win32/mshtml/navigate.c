/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "hlguids.h"
#include "shlguid.h"
#include "wininet.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define CONTENT_LENGTH "Content-Length"
#define UTF16_STR "utf-16"

typedef struct {
    const nsIInputStreamVtbl *lpInputStreamVtbl;

    LONG ref;

    char buf[1024];
    DWORD buf_size;
} nsProtocolStream;

#define NSINSTREAM(x) ((nsIInputStream*) &(x)->lpInputStreamVtbl)

typedef struct {
    void (*destroy)(BSCallback*);
    HRESULT (*init_bindinfo)(BSCallback*);
    HRESULT (*start_binding)(BSCallback*);
    HRESULT (*stop_binding)(BSCallback*,HRESULT);
    HRESULT (*read_data)(BSCallback*,IStream*);
    HRESULT (*on_progress)(BSCallback*,ULONG,LPCWSTR);
    HRESULT (*on_response)(BSCallback*,DWORD);
} BSCallbackVtbl;

struct BSCallback {
    const IBindStatusCallbackVtbl *lpBindStatusCallbackVtbl;
    const IServiceProviderVtbl    *lpServiceProviderVtbl;
    const IHttpNegotiate2Vtbl     *lpHttpNegotiate2Vtbl;
    const IInternetBindInfoVtbl   *lpInternetBindInfoVtbl;

    const BSCallbackVtbl          *vtbl;

    LONG ref;

    LPWSTR headers;
    HGLOBAL post_data;
    ULONG post_data_len;
    ULONG readed;
    DWORD bindf;
    BOOL bindinfo_ready;

    IMoniker *mon;
    IBinding *binding;

    HTMLDocumentNode *doc;

    struct list entry;
};

#define NSINSTREAM_THIS(iface) DEFINE_THIS(nsProtocolStream, InputStream, iface)

static nsresult NSAPI nsInputStream_QueryInterface(nsIInputStream *iface, nsIIDRef riid,
                                                   nsQIResult result)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result  = NSINSTREAM(This);
    }else if(IsEqualGUID(&IID_nsIInputStream, riid)) {
        TRACE("(%p)->(IID_nsIInputStream %p)\n", This, result);
        *result  = NSINSTREAM(This);
    }

    if(*result) {
        nsIInputStream_AddRef(NSINSTREAM(This));
        return NS_OK;
    }

    WARN("unsupported interface %s\n", debugstr_guid(riid));
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsInputStream_AddRef(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}


static nsrefcnt NSAPI nsInputStream_Release(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static nsresult NSAPI nsInputStream_Close(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Available(nsIInputStream *iface, PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Read(nsIInputStream *iface, char *aBuf, PRUint32 aCount,
                                         PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
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
        nsresult (WINAPI *aWriter)(nsIInputStream*,void*,const char*,PRUint32,PRUint32,PRUint32*),
        void *aClousure, PRUint32 aCount, PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    PRUint32 written = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p %d %p)\n", This, aWriter, aClousure, aCount, _retval);

    if(!This->buf_size)
        return S_OK;

    if(aCount > This->buf_size)
        aCount = This->buf_size;

    nsres = aWriter(NSINSTREAM(This), aClousure, This->buf, 0, aCount, &written);
    if(NS_FAILED(nsres))
        TRACE("aWritter failed: %08x\n", nsres);
    else if(written != This->buf_size)
        FIXME("written %d != buf_size %d\n", written, This->buf_size);

    This->buf_size -= written; 

    *_retval = written;
    return nsres;
}

static nsresult NSAPI nsInputStream_IsNonBlocking(nsIInputStream *iface, PRBool *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSINSTREAM_THIS

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
    nsProtocolStream *ret = heap_alloc(sizeof(nsProtocolStream));

    ret->lpInputStreamVtbl = &nsInputStreamVtbl;
    ret->ref = 1;
    ret->buf_size = 0;

    return ret;
}

#define STATUSCLB_THIS(iface) DEFINE_THIS(BSCallback, BindStatusCallback, iface)

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate2 %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINFO(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(STATUSCLB(This));
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    if(!ref) {
        if(This->post_data)
            GlobalFree(This->post_data);
        if(This->mon)
            IMoniker_Release(This->mon);
        if(This->binding)
            IBinding_Release(This->binding);
        list_remove(&This->entry);
        heap_free(This->headers);

        This->vtbl->destroy(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%d %p)\n", This, dwReserved, pbind);

    IBinding_AddRef(pbind);
    This->binding = pbind;

    if(This->doc)
        list_add_head(&This->doc->bindings, &This->entry);

    return This->vtbl->start_binding(This);
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    return This->vtbl->on_progress(This, ulStatusCode, szStatusText);
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    /* NOTE: IE7 calls GetBindResult here */

    hres = This->vtbl->stop_binding(This, hresult);

    if(This->binding) {
        IBinding_Release(This->binding);
        This->binding = NULL;
    }

    list_remove(&This->entry);
    This->doc = NULL;

    return hres;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = STATUSCLB_THIS(iface);
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

    pbindinfo->cbstgmedData = This->post_data_len;
    pbindinfo->dwCodePage = CP_UTF8;
    pbindinfo->dwOptions = 0x80000;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)STATUSCLB(This);
        IBindStatusCallback_AddRef(STATUSCLB(This));
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return This->vtbl->read_data(This, pstgmed->u.pstm);
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

#undef STATUSCLB_THIS

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

#define HTTPNEG_THIS(iface) DEFINE_THIS(BSCallback, HttpNegotiate2, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface,
                                                   REFIID riid, void **ppv)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    DWORD size;

    TRACE("(%p)->(%s %s %d %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    if(!This->headers) {
        *pszAdditionalHeaders = NULL;
        return S_OK;
    }

    size = (strlenW(This->headers)+1)*sizeof(WCHAR);
    *pszAdditionalHeaders = CoTaskMemAlloc(size);
    memcpy(*pszAdditionalHeaders, This->headers, size);

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);

    TRACE("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);

    return This->vtbl->on_response(This, dwResponseCode);
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL;
}

#undef HTTPNEG

static const IHttpNegotiate2Vtbl HttpNegotiate2Vtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

#define BINDINFO_THIS(iface) DEFINE_THIS(BSCallback, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
                                                      REFIID riid, void **ppv)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
                                                   DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%u %p %u %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);
    return E_NOTIMPL;
}

#undef BINDINFO_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(BSCallback, ServiceProvider, iface)

static HRESULT WINAPI BSCServiceProvider_QueryInterface(IServiceProvider *iface,
                                                        REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI BSCServiceProvider_AddRef(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI BSCServiceProvider_Release(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI BSCServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BSCServiceProvider_QueryInterface,
    BSCServiceProvider_AddRef,
    BSCServiceProvider_Release,
    BSCServiceProvider_QueryService
};

static void init_bscallback(BSCallback *This, const BSCallbackVtbl *vtbl, IMoniker *mon, DWORD bindf)
{
    This->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    This->lpServiceProviderVtbl    = &ServiceProviderVtbl;
    This->lpHttpNegotiate2Vtbl     = &HttpNegotiate2Vtbl;
    This->lpInternetBindInfoVtbl   = &InternetBindInfoVtbl;
    This->vtbl = vtbl;
    This->ref = 1;
    This->bindf = bindf;

    list_init(&This->entry);

    if(mon)
        IMoniker_AddRef(mon);
    This->mon = mon;
}

/* Calls undocumented 84 cmd of CGID_ShellDocView */
static void call_docview_84(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd;
    VARIANT var;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(FAILED(hres))
        return;

    VariantInit(&var);
    hres = IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 84, 0, NULL, &var);
    IOleCommandTarget_Release(olecmd);
    if(SUCCEEDED(hres) && V_VT(&var) != VT_NULL)
        FIXME("handle result\n");
}

static void parse_post_data(nsIInputStream *post_data_stream, LPWSTR *headers_ret,
                            HGLOBAL *post_data_ret, ULONG *post_data_len_ret)
{
    PRUint32 post_data_len = 0, available = 0;
    HGLOBAL post_data = NULL;
    LPWSTR headers = NULL;
    DWORD headers_len = 0, len;
    const char *ptr, *ptr2, *post_data_end;

    nsIInputStream_Available(post_data_stream, &available);
    post_data = GlobalAlloc(0, available+1);
    nsIInputStream_Read(post_data_stream, post_data, available, &post_data_len);
    
    TRACE("post_data = %s\n", debugstr_an(post_data, post_data_len));

    ptr = ptr2 = post_data;
    post_data_end = (const char*)post_data+post_data_len;

    while(ptr < post_data_end && (*ptr != '\r' || ptr[1] != '\n')) {
        while(ptr < post_data_end && (*ptr != '\r' || ptr[1] != '\n'))
            ptr++;

        if(!*ptr) {
            FIXME("*ptr = 0\n");
            return;
        }

        ptr += 2;

        if(ptr-ptr2 >= sizeof(CONTENT_LENGTH)
           && CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                             CONTENT_LENGTH, sizeof(CONTENT_LENGTH)-1,
                             ptr2, sizeof(CONTENT_LENGTH)-1) == CSTR_EQUAL) {
            ptr2 = ptr;
            continue;
        }

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, NULL, 0);

        if(headers)
            headers = heap_realloc(headers,(headers_len+len+1)*sizeof(WCHAR));
        else
            headers = heap_alloc((len+1)*sizeof(WCHAR));

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, headers+headers_len, len);
        headers_len += len;

        ptr2 = ptr;
    }

    headers[headers_len] = 0;
    *headers_ret = headers;

    if(ptr >= post_data_end-2) {
        GlobalFree(post_data);
        return;
    }

    ptr += 2;

    if(headers_len) {
        post_data_len -= ptr-(const char*)post_data;
        memmove(post_data, ptr, post_data_len);
        post_data = GlobalReAlloc(post_data, post_data_len+1, 0);
    }

    *post_data_ret = post_data;
    *post_data_len_ret = post_data_len;
}

HRESULT start_binding(HTMLWindow *window, HTMLDocumentNode *doc, BSCallback *bscallback, IBindCtx *bctx)
{
    IStream *str = NULL;
    HRESULT hres;

    bscallback->doc = doc;

    /* NOTE: IE7 calls IsSystemMoniker here*/

    if(window) {
        if(bscallback->mon != window->mon)
            set_current_mon(window, bscallback->mon);
        call_docview_84(window->doc_obj);
    }

    if(bctx) {
        RegisterBindStatusCallback(bctx, STATUSCLB(bscallback), NULL, 0);
        IBindCtx_AddRef(bctx);
    }else {
        hres = CreateAsyncBindCtx(0, STATUSCLB(bscallback), NULL, &bctx);
        if(FAILED(hres)) {
            WARN("CreateAsyncBindCtx failed: %08x\n", hres);
            bscallback->vtbl->stop_binding(bscallback, hres);
            return hres;
        }
    }

    hres = IMoniker_BindToStorage(bscallback->mon, bctx, NULL, &IID_IStream, (void**)&str);
    IBindCtx_Release(bctx);
    if(FAILED(hres)) {
        WARN("BindToStorage failed: %08x\n", hres);
        bscallback->vtbl->stop_binding(bscallback, hres);
        return hres;
    }

    if(str)
        IStream_Release(str);

    IMoniker_Release(bscallback->mon);
    bscallback->mon = NULL;

    return S_OK;
}

typedef struct {
    BSCallback bsc;

    DWORD size;
    BYTE *buf;
    HRESULT hres;
} BufferBSC;

#define BUFFERBSC_THIS(bsc) ((BufferBSC*) bsc)

static void BufferBSC_destroy(BSCallback *bsc)
{
    BufferBSC *This = BUFFERBSC_THIS(bsc);

    heap_free(This->buf);
    heap_free(This);
}

static HRESULT BufferBSC_init_bindinfo(BSCallback *bsc)
{
    return S_OK;
}

static HRESULT BufferBSC_start_binding(BSCallback *bsc)
{
    return S_OK;
}

static HRESULT BufferBSC_stop_binding(BSCallback *bsc, HRESULT result)
{
    BufferBSC *This = BUFFERBSC_THIS(bsc);

    This->hres = result;

    if(FAILED(result)) {
        heap_free(This->buf);
        This->buf = NULL;
        This->size = 0;
    }

    return S_OK;
}

static HRESULT BufferBSC_read_data(BSCallback *bsc, IStream *stream)
{
    BufferBSC *This = BUFFERBSC_THIS(bsc);
    DWORD readed;
    HRESULT hres;

    if(!This->buf) {
        This->size = 128;
        This->buf = heap_alloc(This->size);
    }

    do {
        if(This->bsc.readed == This->size) {
            This->size <<= 1;
            This->buf = heap_realloc(This->buf, This->size);
        }

        readed = 0;
        hres = IStream_Read(stream, This->buf+This->bsc.readed, This->size-This->bsc.readed, &readed);
        This->bsc.readed += readed;
    }while(hres == S_OK);

    return S_OK;
}

static HRESULT BufferBSC_on_progress(BSCallback *bsc, ULONG status_code, LPCWSTR status_text)
{
    return S_OK;
}

static HRESULT BufferBSC_on_response(BSCallback *bsc, DWORD response_code)
{
    return S_OK;
}

#undef BUFFERBSC_THIS

static const BSCallbackVtbl BufferBSCVtbl = {
    BufferBSC_destroy,
    BufferBSC_init_bindinfo,
    BufferBSC_start_binding,
    BufferBSC_stop_binding,
    BufferBSC_read_data,
    BufferBSC_on_progress,
    BufferBSC_on_response
};


static BufferBSC *create_bufferbsc(IMoniker *mon)
{
    BufferBSC *ret = heap_alloc_zero(sizeof(*ret));

    init_bscallback(&ret->bsc, &BufferBSCVtbl, mon, 0);
    ret->hres = E_FAIL;

    return ret;
}

HRESULT bind_mon_to_buffer(HTMLDocumentNode *doc, IMoniker *mon, void **buf, DWORD *size)
{
    BufferBSC *bsc = create_bufferbsc(mon);
    HRESULT hres;

    *buf = NULL;

    hres = start_binding(NULL, doc, &bsc->bsc, NULL);
    if(SUCCEEDED(hres)) {
        hres = bsc->hres;
        if(SUCCEEDED(hres)) {
            *buf = bsc->buf;
            bsc->buf = NULL;
            *size = bsc->bsc.readed;
            bsc->size = 0;
        }
    }

    IBindStatusCallback_Release(STATUSCLB(&bsc->bsc));

    return hres;
}

struct nsChannelBSC {
    BSCallback bsc;

    HTMLWindow *window;

    nsChannel *nschannel;
    nsIStreamListener *nslistener;
    nsISupports *nscontext;

    nsProtocolStream *nsstream;
};

static void on_start_nsrequest(nsChannelBSC *This)
{
    nsresult nsres;

    /* FIXME: it's needed for http connections from BindToObject. */
    if(!This->nschannel->response_status)
        This->nschannel->response_status = 200;

    nsres = nsIStreamListener_OnStartRequest(This->nslistener,
            (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext);
    if(NS_FAILED(nsres))
        FIXME("OnStartRequest failed: %08x\n", nsres);
}

static void on_stop_nsrequest(nsChannelBSC *This, HRESULT result)
{
    nsresult nsres;

    if(!This->nslistener)
        return;

    if(!This->bsc.readed && SUCCEEDED(result)) {
        TRACE("No data read! Calling OnStartRequest\n");
        on_start_nsrequest(This);
    }

    nsres = nsIStreamListener_OnStopRequest(This->nslistener, (nsIRequest*)NSCHANNEL(This->nschannel),
             This->nscontext, SUCCEEDED(result) ? NS_OK : NS_ERROR_FAILURE);
    if(NS_FAILED(nsres))
        WARN("OnStopRequest failed: %08x\n", nsres);
}

static HRESULT read_stream_data(nsChannelBSC *This, IStream *stream)
{
    DWORD read;
    nsresult nsres;
    HRESULT hres;

    if(!This->nslistener) {
        BYTE buf[1024];

        do {
            read = 0;
            hres = IStream_Read(stream, buf, sizeof(buf), &read);
        }while(hres == S_OK && read);

        return S_OK;
    }

    if(!This->nsstream)
        This->nsstream = create_nsprotocol_stream();

    do {
        read = 0;
        hres = IStream_Read(stream, This->nsstream->buf+This->nsstream->buf_size,
                sizeof(This->nsstream->buf)-This->nsstream->buf_size, &read);
        if(!read)
            break;

        This->nsstream->buf_size += read;

        if(!This->bsc.readed) {
            if(This->nsstream->buf_size >= 2
               && (BYTE)This->nsstream->buf[0] == 0xff
               && (BYTE)This->nsstream->buf[1] == 0xfe)
                This->nschannel->charset = heap_strdupA(UTF16_STR);

            if(!This->nschannel->content_type) {
                WCHAR *mime;

                hres = FindMimeFromData(NULL, NULL, This->nsstream->buf, This->nsstream->buf_size, NULL, 0, &mime, 0);
                if(FAILED(hres))
                    return hres;

                TRACE("Found MIME %s\n", debugstr_w(mime));

                This->nschannel->content_type = heap_strdupWtoA(mime);
                CoTaskMemFree(mime);
                if(!This->nschannel->content_type)
                    return E_OUTOFMEMORY;
            }

            on_start_nsrequest(This);

            if(This->window) {
                update_window_doc(This->window);
                if(This->window->readystate != READYSTATE_LOADING)
                    set_ready_state(This->window, READYSTATE_LOADING);
            }
        }

        This->bsc.readed += This->nsstream->buf_size;

        nsres = nsIStreamListener_OnDataAvailable(This->nslistener,
                (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext,
                NSINSTREAM(This->nsstream), This->bsc.readed-This->nsstream->buf_size,
                This->nsstream->buf_size);
        if(NS_FAILED(nsres))
            ERR("OnDataAvailable failed: %08x\n", nsres);

        if(This->nsstream->buf_size == sizeof(This->nsstream->buf)) {
            ERR("buffer is full\n");
            break;
        }
    }while(hres == S_OK);

    return S_OK;
}

static void add_nsrequest(nsChannelBSC *This)
{
    nsresult nsres;

    if(!This->nschannel || !This->nschannel->load_group)
        return;

    nsres = nsILoadGroup_AddRequest(This->nschannel->load_group,
            (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext);

    if(NS_FAILED(nsres))
        ERR("AddRequest failed:%08x\n", nsres);
}

#define NSCHANNELBSC_THIS(bsc) ((nsChannelBSC*) bsc)

static void nsChannelBSC_destroy(BSCallback *bsc)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    if(This->nschannel)
        nsIChannel_Release(NSCHANNEL(This->nschannel));
    if(This->nslistener)
        nsIStreamListener_Release(This->nslistener);
    if(This->nscontext)
        nsISupports_Release(This->nscontext);
    if(This->nsstream)
        nsIInputStream_Release(NSINSTREAM(This->nsstream));
    heap_free(This);
}

static HRESULT nsChannelBSC_start_binding(BSCallback *bsc)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    add_nsrequest(This);

    return S_OK;
}

static HRESULT nsChannelBSC_init_bindinfo(BSCallback *bsc)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    if(This->nschannel && This->nschannel->post_data_stream) {
        parse_post_data(This->nschannel->post_data_stream, &This->bsc.headers, &This->bsc.post_data, &This->bsc.post_data_len);
        TRACE("headers = %s post_data = %s\n", debugstr_w(This->bsc.headers),
              debugstr_an(This->bsc.post_data, This->bsc.post_data_len));
    }

    return S_OK;
}

static HRESULT nsChannelBSC_stop_binding(BSCallback *bsc, HRESULT result)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    on_stop_nsrequest(This, result);

    if(This->nslistener) {
        if(This->nschannel->load_group) {
            nsresult nsres;

            nsres = nsILoadGroup_RemoveRequest(This->nschannel->load_group,
                    (nsIRequest*)NSCHANNEL(This->nschannel), NULL, NS_OK);
            if(NS_FAILED(nsres))
                ERR("RemoveRequest failed: %08x\n", nsres);
        }
    }

    return S_OK;
}

static HRESULT nsChannelBSC_read_data(BSCallback *bsc, IStream *stream)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    return read_stream_data(This, stream);
}

static HRESULT nsChannelBSC_on_progress(BSCallback *bsc, ULONG status_code, LPCWSTR status_text)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    switch(status_code) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        if(!This->nschannel)
            return S_OK;

        heap_free(This->nschannel->content_type);
        This->nschannel->content_type = heap_strdupWtoA(status_text);
        break;
    case BINDSTATUS_REDIRECTING:
        TRACE("redirect to %s\n", debugstr_w(status_text));

        /* FIXME: We should find a better way to handle this */
        set_wine_url(This->nschannel->uri, status_text);
    }

    return S_OK;
}

static HRESULT nsChannelBSC_on_response(BSCallback *bsc, DWORD response_code)
{
    nsChannelBSC *This = NSCHANNELBSC_THIS(bsc);

    This->nschannel->response_status = response_code;
    return S_OK;
}

#undef NSCHANNELBSC_THIS

static const BSCallbackVtbl nsChannelBSCVtbl = {
    nsChannelBSC_destroy,
    nsChannelBSC_init_bindinfo,
    nsChannelBSC_start_binding,
    nsChannelBSC_stop_binding,
    nsChannelBSC_read_data,
    nsChannelBSC_on_progress,
    nsChannelBSC_on_response
};

HRESULT create_channelbsc(IMoniker *mon, WCHAR *headers, BYTE *post_data, DWORD post_data_size, nsChannelBSC **retval)
{
    nsChannelBSC *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    init_bscallback(&ret->bsc, &nsChannelBSCVtbl, mon, BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA);

    if(headers) {
        ret->bsc.headers = heap_strdupW(headers);
        if(!ret->bsc.headers) {
            IBindStatusCallback_Release(STATUSCLB(&ret->bsc));
            return E_OUTOFMEMORY;
        }
    }

    if(post_data) {
        ret->bsc.post_data = GlobalAlloc(0, post_data_size);
        if(!ret->bsc.post_data) {
            heap_free(ret->bsc.headers);
            IBindStatusCallback_Release(STATUSCLB(&ret->bsc));
            return E_OUTOFMEMORY;
        }

        memcpy(ret->bsc.post_data, post_data, post_data_size);
        ret->bsc.post_data_len = post_data_size;
    }

    *retval = ret;
    return S_OK;
}

IMoniker *get_channelbsc_mon(nsChannelBSC *This)
{
    if(This->bsc.mon)
        IMoniker_AddRef(This->bsc.mon);
    return This->bsc.mon;
}

void set_window_bscallback(HTMLWindow *window, nsChannelBSC *callback)
{
    if(window->bscallback) {
        if(window->bscallback->bsc.binding)
            IBinding_Abort(window->bscallback->bsc.binding);
        window->bscallback->bsc.doc = NULL;
        window->bscallback->window = NULL;
        IBindStatusCallback_Release(STATUSCLB(&window->bscallback->bsc));
    }

    window->bscallback = callback;

    if(callback) {
        callback->window = window;
        IBindStatusCallback_AddRef(STATUSCLB(&callback->bsc));
        callback->bsc.doc = window->doc;
    }
}

typedef struct {
    task_t header;
    HTMLWindow *window;
    nsChannelBSC *bscallback;
} start_doc_binding_task_t;

static void start_doc_binding_proc(task_t *_task)
{
    start_doc_binding_task_t *task = (start_doc_binding_task_t*)_task;

    start_binding(task->window, NULL, (BSCallback*)task->bscallback, NULL);
    IBindStatusCallback_Release(STATUSCLB(&task->bscallback->bsc));
}

HRESULT async_start_doc_binding(HTMLWindow *window, nsChannelBSC *bscallback)
{
    start_doc_binding_task_t *task;

    task = heap_alloc(sizeof(start_doc_binding_task_t));
    if(!task)
        return E_OUTOFMEMORY;

    task->window = window;
    task->bscallback = bscallback;
    IBindStatusCallback_AddRef(STATUSCLB(&bscallback->bsc));

    push_task(&task->header, start_doc_binding_proc, window->task_magic);
    return S_OK;
}

void abort_document_bindings(HTMLDocumentNode *doc)
{
    BSCallback *iter;

    LIST_FOR_EACH_ENTRY(iter, &doc->bindings, BSCallback, entry) {
        if(iter->binding)
            IBinding_Abort(iter->binding);
        iter->doc = NULL;
        list_remove(&iter->entry);
    }
}

HRESULT channelbsc_load_stream(nsChannelBSC *bscallback, IStream *stream)
{
    HRESULT hres;

    if(!bscallback->nschannel) {
        ERR("NULL nschannel\n");
        return E_FAIL;
    }

    bscallback->nschannel->content_type = heap_strdupA("text/html");
    if(!bscallback->nschannel->content_type)
        return E_OUTOFMEMORY;

    add_nsrequest(bscallback);

    hres = read_stream_data(bscallback, stream);
    IBindStatusCallback_OnStopBinding(STATUSCLB(&bscallback->bsc), hres, ERROR_SUCCESS);

    return hres;
}

void channelbsc_set_channel(nsChannelBSC *This, nsChannel *channel, nsIStreamListener *listener, nsISupports *context)
{
    nsIChannel_AddRef(NSCHANNEL(channel));
    This->nschannel = channel;

    nsIStreamListener_AddRef(listener);
    This->nslistener = listener;

    if(context) {
        nsISupports_AddRef(context);
        This->nscontext = context;
    }
}

HRESULT hlink_frame_navigate(HTMLDocument *doc, LPCWSTR url,
        nsIInputStream *post_data_stream, DWORD hlnf, BOOL *cancel)
{
    IHlinkFrame *hlink_frame;
    nsChannelBSC *callback;
    IServiceProvider *sp;
    IBindCtx *bindctx;
    IMoniker *mon;
    IHlink *hlink;
    HRESULT hres;

    *cancel = FALSE;

    hres = IOleClientSite_QueryInterface(doc->doc_obj->client, &IID_IServiceProvider,
            (void**)&sp);
    if(FAILED(hres))
        return S_OK;

    hres = IServiceProvider_QueryService(sp, &IID_IHlinkFrame, &IID_IHlinkFrame,
            (void**)&hlink_frame);
    IServiceProvider_Release(sp);
    if(FAILED(hres))
        return S_OK;

    hres = create_channelbsc(NULL, NULL, NULL, 0, &callback);
    if(FAILED(hres)) {
        IHlinkFrame_Release(hlink_frame);
        return hres;
    }

    if(post_data_stream) {
        parse_post_data(post_data_stream, &callback->bsc.headers, &callback->bsc.post_data,
                        &callback->bsc.post_data_len);
        TRACE("headers = %s post_data = %s\n", debugstr_w(callback->bsc.headers),
              debugstr_an(callback->bsc.post_data, callback->bsc.post_data_len));
    }

    hres = CreateAsyncBindCtx(0, STATUSCLB(&callback->bsc), NULL, &bindctx);
    if(SUCCEEDED(hres))
        hres = CoCreateInstance(&CLSID_StdHlink, NULL, CLSCTX_INPROC_SERVER,
                &IID_IHlink, (LPVOID*)&hlink);

    if(SUCCEEDED(hres))
        hres = CreateURLMoniker(NULL, url, &mon);

    if(SUCCEEDED(hres)) {
        IHlink_SetMonikerReference(hlink, HLINKSETF_TARGET, mon, NULL);

        if(hlnf & HLNF_OPENINNEWWINDOW) {
            static const WCHAR wszBlank[] = {'_','b','l','a','n','k',0};
            IHlink_SetTargetFrameName(hlink, wszBlank); /* FIXME */
        }

        hres = IHlinkFrame_Navigate(hlink_frame, hlnf, bindctx, STATUSCLB(&callback->bsc), hlink);
        IMoniker_Release(mon);
        *cancel = hres == S_OK;
        hres = S_OK;
    }

    IHlinkFrame_Release(hlink_frame);
    IBindCtx_Release(bindctx);
    IBindStatusCallback_Release(STATUSCLB(&callback->bsc));
    return hres;
}

HRESULT navigate_url(HTMLWindow *window, const WCHAR *new_url, const WCHAR *base_url)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    nsWineURI *uri;
    HRESULT hres;

    if(!new_url) {
        *url = 0;
    }else if(base_url) {
        DWORD len = 0;

        hres = CoInternetCombineUrl(base_url, new_url, URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                url, sizeof(url)/sizeof(WCHAR), &len, 0);
        if(FAILED(hres))
            return hres;
    }else {
        strcpyW(url, new_url);
    }

    if(window->doc_obj && window->doc_obj->hostui) {
        OLECHAR *translated_url = NULL;

        hres = IDocHostUIHandler_TranslateUrl(window->doc_obj->hostui, 0, url,
                &translated_url);
        if(hres == S_OK) {
            TRACE("%08x %s -> %s\n", hres, debugstr_w(url), debugstr_w(translated_url));
            strcpyW(url, translated_url);
            CoTaskMemFree(translated_url);
        }
    }

    if(window->doc_obj && window == window->doc_obj->basedoc.window) {
        BOOL cancel;

        hres = hlink_frame_navigate(&window->doc->basedoc, url, NULL, 0, &cancel);
        if(FAILED(hres))
            return hres;

        if(cancel) {
            TRACE("Navigation handled by hlink frame\n");
            return S_OK;
        }
    }

    hres = create_doc_uri(window, url, &uri);
    if(FAILED(hres))
        return hres;

    hres = load_nsuri(window, uri, NULL, LOAD_FLAGS_NONE);
    nsISupports_Release((nsISupports*)uri);
    return hres;
}
