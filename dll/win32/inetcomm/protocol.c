/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include <assert.h>

#include "mimeole.h"
#include "inetcomm_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct {
    IUnknown IUnknown_inner;
    IInternetProtocol IInternetProtocol_iface;
    IInternetProtocolInfo IInternetProtocolInfo_iface;

    LONG ref;
    IUnknown *outer_unk;

    WCHAR *location;
    IStream *stream;
    IInternetProtocolSink *sink;
} MimeHtmlProtocol;

typedef struct {
    const WCHAR *mhtml;
    size_t mhtml_len;
    const WCHAR *location;
} mhtml_url_t;

typedef struct {
    IBindStatusCallback IBindStatusCallback_iface;

    LONG ref;

    MimeHtmlProtocol *protocol;
    HRESULT status;
    IStream *stream;
    WCHAR url[1];
} MimeHtmlBinding;

static const WCHAR mhtml_prefixW[] = L"mhtml:";
static const WCHAR mhtml_separatorW[] = L"!x-usc:";

static HRESULT parse_mhtml_url(const WCHAR *url, mhtml_url_t *r)
{
    const WCHAR *p;

    if(wcsnicmp(url, mhtml_prefixW, lstrlenW(mhtml_prefixW)))
        return E_FAIL;

    r->mhtml = url + lstrlenW(mhtml_prefixW);
    p = wcschr(r->mhtml, '!');
    if(p) {
        r->mhtml_len = p - r->mhtml;
        /* FIXME: We handle '!' and '!x-usc:' in URLs as the same thing. Those should not be the same. */
        if(!wcsncmp(p, mhtml_separatorW, lstrlenW(mhtml_separatorW)))
            p += lstrlenW(mhtml_separatorW);
        else
            p++;
    }else {
        r->mhtml_len = lstrlenW(r->mhtml);
    }

    r->location = p;
    return S_OK;
}

static HRESULT report_result(MimeHtmlProtocol *protocol, HRESULT result)
{
    if(protocol->sink) {
        IInternetProtocolSink_ReportResult(protocol->sink, result, ERROR_SUCCESS, NULL);
        IInternetProtocolSink_Release(protocol->sink);
        protocol->sink = NULL;
    }

    return result;
}

static HRESULT on_mime_message_available(MimeHtmlProtocol *protocol, IMimeMessage *mime_message)
{
    FINDBODY find = {NULL};
    IMimeBody *mime_body;
    PROPVARIANT value;
    HBODY body;
    HRESULT hres;

    hres = IMimeMessage_FindFirst(mime_message, &find, &body);
    if(FAILED(hres))
        return report_result(protocol, hres);

    if(protocol->location) {
        BOOL found = FALSE;
        do {
            hres = IMimeMessage_FindNext(mime_message, &find, &body);
            if(FAILED(hres)) {
                WARN("location %s not found\n", debugstr_w(protocol->location));
                return report_result(protocol, hres);
            }

            value.vt = VT_LPWSTR;
            hres = IMimeMessage_GetBodyProp(mime_message, body, "content-location", 0, &value);
            if(hres == MIME_E_NOT_FOUND)
                continue;
            if(FAILED(hres))
                return report_result(protocol, hres);

            found = !lstrcmpW(protocol->location, value.pwszVal);
            PropVariantClear(&value);
        }while(!found);
    }else {
        hres = IMimeMessage_FindNext(mime_message, &find, &body);
        if(FAILED(hres)) {
            WARN("location %s not found\n", debugstr_w(protocol->location));
            return report_result(protocol, hres);
        }
    }

    hres = IMimeMessage_BindToObject(mime_message, body, &IID_IMimeBody, (void**)&mime_body);
    if(FAILED(hres))
        return report_result(protocol, hres);

    value.vt = VT_LPWSTR;
    hres = IMimeBody_GetProp(mime_body, "content-type", 0, &value);
    if(SUCCEEDED(hres)) {
        hres = IInternetProtocolSink_ReportProgress(protocol->sink, BINDSTATUS_MIMETYPEAVAILABLE, value.pwszVal);
        PropVariantClear(&value);
    }

    /* FIXME: Create and report cache file. */

    hres = IMimeBody_GetData(mime_body, IET_DECODED, &protocol->stream);
    if(FAILED(hres))
        return report_result(protocol, hres);

    IInternetProtocolSink_ReportData(protocol->sink, BSCF_FIRSTDATANOTIFICATION
                                     | BSCF_INTERMEDIATEDATANOTIFICATION
                                     | BSCF_LASTDATANOTIFICATION
                                     | BSCF_DATAFULLYAVAILABLE
                                     | BSCF_AVAILABLEDATASIZEUNKNOWN, 0, 0);

    return report_result(protocol, S_OK);
}

static HRESULT load_mime_message(IStream *stream, IMimeMessage **ret)
{
    IMimeMessage *mime_message;
    HRESULT hres;

    hres = MimeMessage_create(NULL, (void**)&mime_message);
    if(FAILED(hres))
        return hres;

    IMimeMessage_InitNew(mime_message);

    hres = IMimeMessage_Load(mime_message, stream);
    if(FAILED(hres)) {
        IMimeMessage_Release(mime_message);
        return hres;
    }

    *ret = mime_message;
    return S_OK;
}

static inline MimeHtmlBinding *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlBinding, IBindStatusCallback_iface);
}

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->protocol)
            IInternetProtocol_Release(&This->protocol->IInternetProtocol_iface);
        if(This->stream)
            IStream_Release(This->stream);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pib)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%lx %p)\n", This, dwReserved, pib);

    assert(!This->stream);
    return CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD dwReserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%lu/%lu %lu %s)\n", This, ulProgress, ulProgressMax, ulStatusCode, debugstr_w(szStatusText));
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);
    IMimeMessage *mime_message = NULL;

    TRACE("(%p)->(%lx %s)\n", This, hresult, debugstr_w(szError));

    if(SUCCEEDED(hresult)) {
        hresult = load_mime_message(This->stream, &mime_message);
        IStream_Release(This->stream);
        This->stream = NULL;
    }

    This->status = hresult;

    if(mime_message)
        on_mime_message_available(This->protocol, mime_message);
    else
        report_result(This->protocol, hresult);

    if(mime_message)
        IMimeMessage_Release(mime_message);
    IInternetProtocol_Release(&This->protocol->IInternetProtocol_iface);
    This->protocol = NULL;
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)\n", This);

    *grfBINDF = BINDF_ASYNCHRONOUS;
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    MimeHtmlBinding *This = impl_from_IBindStatusCallback(iface);
    BYTE buf[4*1024];
    DWORD read;
    HRESULT hres;

    TRACE("(%p)\n", This);

    assert(pstgmed->tymed == TYMED_ISTREAM);

    while(1) {
        hres = IStream_Read(pstgmed->pstm, buf, sizeof(buf), &read);
        if(FAILED(hres))
            return hres;
        if(!read)
            break;
        hres = IStream_Write(This->stream, buf, read, NULL);
        if(FAILED(hres))
            return hres;
    }
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown* punk)
{
    ERR("\n");
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

static inline MimeHtmlProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IUnknown_inner);
}

static HRESULT WINAPI MimeHtmlProtocol_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolInfo %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolInfo_iface;
    }else {
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MimeHtmlProtocol_AddRef(IUnknown *iface)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI MimeHtmlProtocol_Release(IUnknown *iface)
{
    MimeHtmlProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lx\n", This, ref);

    if(!ref) {
        if(This->sink)
            IInternetProtocolSink_Release(This->sink);
        if(This->stream)
            IStream_Release(This->stream);
        free(This->location);
        free(This);
    }

    return ref;
}

static const IUnknownVtbl MimeHtmlProtocolInnerVtbl = {
    MimeHtmlProtocol_QueryInterface,
    MimeHtmlProtocol_AddRef,
    MimeHtmlProtocol_Release
};

static inline MimeHtmlProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI InternetProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI InternetProtocol_AddRef(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI InternetProtocol_Release(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MimeHtmlProtocol_Start(IInternetProtocol *iface, const WCHAR *szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    BINDINFO bindinfo = { sizeof(bindinfo) };
    MimeHtmlBinding *binding;
    IBindCtx *bind_ctx;
    IStream *stream;
    mhtml_url_t url;
    DWORD bindf = 0;
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink, pOIBindInfo, grfPI, dwReserved);

    hres = parse_mhtml_url(szUrl, &url);
    if(FAILED(hres))
        return hres;

    if(url.location && !(This->location = wcsdup(url.location)))
        return E_OUTOFMEMORY;

    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        return hres;
    }
    if((bindf & (BINDF_ASYNCHRONOUS|BINDF_FROMURLMON|BINDF_NEEDFILE)) != (BINDF_ASYNCHRONOUS|BINDF_FROMURLMON|BINDF_NEEDFILE))
        FIXME("unsupported bindf %lx\n", bindf);

    IInternetProtocolSink_AddRef(This->sink = pOIProtSink);

    binding = malloc(FIELD_OFFSET(MimeHtmlBinding, url[url.mhtml_len+1]));
    if(!binding)
        return E_OUTOFMEMORY;
    memcpy(binding->url, url.mhtml, url.mhtml_len*sizeof(WCHAR));
    binding->url[url.mhtml_len] = 0;

    hres = CreateURLMoniker(NULL, binding->url, &mon);
    if(FAILED(hres)) {
        free(binding);
        return hres;
    }

    binding->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    binding->ref = 1;
    binding->status = E_PENDING;
    binding->stream = NULL;
    binding->protocol = NULL;

    hres = CreateAsyncBindCtx(0, &binding->IBindStatusCallback_iface, NULL, &bind_ctx);
    if(FAILED(hres)) {
        IMoniker_Release(mon);
        IBindStatusCallback_Release(&binding->IBindStatusCallback_iface);
        return hres;
    }

    IInternetProtocol_AddRef(&This->IInternetProtocol_iface);
    binding->protocol = This;

    hres = IMoniker_BindToStorage(mon, bind_ctx, NULL, &IID_IStream, (void**)&stream);
    IBindCtx_Release(bind_ctx);
    IMoniker_Release(mon);
    if(stream)
        IStream_Release(stream);
    hres = binding->status;
    IBindStatusCallback_Release(&binding->IBindStatusCallback_iface);
    if(FAILED(hres) && hres != E_PENDING)
        report_result(This, hres);
    return hres;
}

static HRESULT WINAPI MimeHtmlProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    TRACE("(%p)->(%08lx)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI MimeHtmlProtocol_Suspend(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Resume(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    ULONG read = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    hres = IStream_Read(This->stream, pv, cb, &read);
    if(pcbRead)
        *pcbRead = read;
    if(hres != S_OK)
        return hres;

    return read ? S_OK : S_FALSE;
}

static HRESULT WINAPI MimeHtmlProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%ld)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI MimeHtmlProtocol_UnlockRequest(IInternetProtocol *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return S_OK;
}

static const IInternetProtocolVtbl MimeHtmlProtocolVtbl = {
    InternetProtocol_QueryInterface,
    InternetProtocol_AddRef,
    InternetProtocol_Release,
    MimeHtmlProtocol_Start,
    MimeHtmlProtocol_Continue,
    MimeHtmlProtocol_Abort,
    MimeHtmlProtocol_Terminate,
    MimeHtmlProtocol_Suspend,
    MimeHtmlProtocol_Resume,
    MimeHtmlProtocol_Read,
    MimeHtmlProtocol_Seek,
    MimeHtmlProtocol_LockRequest,
    MimeHtmlProtocol_UnlockRequest
};

static inline MimeHtmlProtocol *impl_from_IInternetProtocolInfo(IInternetProtocolInfo *iface)
{
    return CONTAINING_RECORD(iface, MimeHtmlProtocol, IInternetProtocolInfo_iface);
}

static HRESULT WINAPI MimeHtmlProtocolInfo_QueryInterface(IInternetProtocolInfo *iface, REFIID riid, void **ppv)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI MimeHtmlProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MimeHtmlProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MimeHtmlProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %d %lx %p %ld %p %ld)\n", This, debugstr_w(pwzUrl), ParseAction,
          dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return INET_E_DEFAULT_ACTION;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult,
        DWORD cchResult, DWORD* pcchResult, DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    size_t len = lstrlenW(mhtml_prefixW);
    mhtml_url_t url;
    WCHAR *p;
    HRESULT hres;

    TRACE("(%p)->(%s %s %08lx %p %ld %p %ld)\n", This, debugstr_w(pwzBaseUrl),
          debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult,
          pcchResult, dwReserved);

    hres = parse_mhtml_url(pwzBaseUrl, &url);
    if(FAILED(hres))
        return hres;

    if(!wcsnicmp(pwzRelativeUrl, mhtml_prefixW, len)) {
        FIXME("Relative URL is mhtml protocol\n");
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    }

    len += url.mhtml_len;
    if(*pwzRelativeUrl)
        len += lstrlenW(pwzRelativeUrl) + lstrlenW(mhtml_separatorW);
    if(len >= cchResult) {
        *pcchResult = 0;
        return E_FAIL;
    }

    lstrcpyW(pwzResult, mhtml_prefixW);
    p = pwzResult + lstrlenW(mhtml_prefixW);
    memcpy(p, url.mhtml, url.mhtml_len*sizeof(WCHAR));
    p += url.mhtml_len;
    if(*pwzRelativeUrl) {
        lstrcpyW(p, mhtml_separatorW);
        lstrcatW(p, pwzRelativeUrl);
    }else {
        *p = 0;
    }

    *pcchResult = len;
    return S_OK;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %s %08lx)\n", This, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeHtmlProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    MimeHtmlProtocol *This = impl_from_IInternetProtocolInfo(iface);
    FIXME("(%p)->(%s %08x %08lx %p %ld %p %ld)\n", This, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
          cbBuffer, pcbBuf, dwReserved);
    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
}

static const IInternetProtocolInfoVtbl MimeHtmlProtocolInfoVtbl = {
    MimeHtmlProtocolInfo_QueryInterface,
    MimeHtmlProtocolInfo_AddRef,
    MimeHtmlProtocolInfo_Release,
    MimeHtmlProtocolInfo_ParseUrl,
    MimeHtmlProtocolInfo_CombineUrl,
    MimeHtmlProtocolInfo_CompareUrl,
    MimeHtmlProtocolInfo_QueryInfo
};

HRESULT MimeHtmlProtocol_create(IUnknown *outer, void **obj)
{
    MimeHtmlProtocol *protocol;

    protocol = malloc(sizeof(*protocol));
    if(!protocol)
        return E_OUTOFMEMORY;

    protocol->IUnknown_inner.lpVtbl = &MimeHtmlProtocolInnerVtbl;
    protocol->IInternetProtocol_iface.lpVtbl = &MimeHtmlProtocolVtbl;
    protocol->IInternetProtocolInfo_iface.lpVtbl = &MimeHtmlProtocolInfoVtbl;
    protocol->ref = 1;
    protocol->outer_unk = outer ? outer : &protocol->IUnknown_inner;
    protocol->location = NULL;
    protocol->stream = NULL;
    protocol->sink = NULL;

    *obj = &protocol->IUnknown_inner;
    return S_OK;
}
