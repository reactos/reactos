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

#include "ieframe.h"

#include <wininet.h>

static const WCHAR emptyW[] = {0};

typedef struct {
    IBindStatusCallback  IBindStatusCallback_iface;
    IHttpNegotiate       IHttpNegotiate_iface;
    IHttpSecurity        IHttpSecurity_iface;

    LONG ref;

    DocHost *doc_host;
    IBinding *binding;

    BSTR url;
    HGLOBAL post_data;
    BSTR headers;
    ULONG post_data_len;
} BindStatusCallback;

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char * const BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char * const BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %d, %s,\n"
            "    {%d, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %d, %08x, %d, %d\n"
            "    {%d %p %x},\n"
            "    %s\n"
            "    %p, %d\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbstgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}

static void set_status_text(BindStatusCallback *This, ULONG statuscode, LPCWSTR str)
{
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};
    WCHAR fmt[IDS_STATUSFMT_MAXLEN];
    WCHAR buffer[IDS_STATUSFMT_MAXLEN + INTERNET_MAX_URL_LENGTH];

    if(!This->doc_host)
        return;

    TRACE("(%p, %d, %s)\n", This, statuscode, debugstr_w(str));
    buffer[0] = 0;
    if (statuscode && str && *str) {
        fmt[0] = 0;
        /* the format string must have one "%s" for the str */
        LoadStringW(ieframe_instance, IDS_STATUSFMT_FIRST + statuscode, fmt, IDS_STATUSFMT_MAXLEN);
        snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), fmt, str);
    }

    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = str ? SysAllocString(buffer) : SysAllocString(emptyW);
    TRACE("=> %s\n", debugstr_w(V_BSTR(&arg)));

    call_sink(This->doc_host->cps.wbe2, DISPID_STATUSTEXTCHANGE, &dispparams);

    if(This->doc_host->frame)
        IOleInPlaceFrame_SetStatusText(This->doc_host->frame, buffer);

    VariantClear(&arg);

}

HRESULT set_dochost_url(DocHost *This, const WCHAR *url)
{
    WCHAR *new_url;

    if(url) {
        new_url = heap_strdupW(url);
        if(!new_url)
            return E_OUTOFMEMORY;
    }else {
        new_url = NULL;
    }

    heap_free(This->url);
    This->url = new_url;

    This->container_vtbl->set_url(This, This->url);
    return S_OK;
}

void notify_download_state(DocHost *dochost, BOOL is_downloading)
{
    DISPPARAMS dwl_dp = {NULL};
    TRACE("(%x)\n", is_downloading);
    call_sink(dochost->cps.wbe2, is_downloading ? DISPID_DOWNLOADBEGIN : DISPID_DOWNLOADCOMPLETE, &dwl_dp);
}

static inline BindStatusCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
                                                        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = &This->IHttpNegotiate_iface;
    }else if(IsEqualGUID(&IID_IWindowForBindingUI, riid)) {
        TRACE("(%p)->(IID_IWindowForBindingUI %p)\n", This, ppv);
        *ppv = &This->IHttpSecurity_iface;
    }else if(IsEqualGUID(&IID_IHttpSecurity, riid)) {
        TRACE("(%p)->(IID_IHttpSecurity %p)\n", This, ppv);
        *ppv = &This->IHttpSecurity_iface;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->doc_host)
            IOleClientSite_Release(&This->doc_host->IOleClientSite_iface);
        if(This->binding)
            IBinding_Release(This->binding);
        if(This->post_data)
            GlobalFree(This->post_data);
        SysFreeString(This->headers);
        SysFreeString(This->url);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
       DWORD dwReserved, IBinding *pbind)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%d %p)\n", This, dwReserved, pbind);

    This->binding = pbind;
    IBinding_AddRef(This->binding);

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface,
       LONG *pnPriority)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface,
       DWORD reserved)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static DWORD get_http_status_code(IBinding *binding)
{
    IWinInetHttpInfo *http_info;
    DWORD status, size = sizeof(DWORD);
    HRESULT hres;

    hres = IBinding_QueryInterface(binding, &IID_IWinInetHttpInfo, (void**)&http_info);
    if(FAILED(hres))
        return HTTP_STATUS_OK;

    hres = IWinInetHttpInfo_QueryInfo(http_info, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
            &status, &size, NULL, NULL);
    IWinInetHttpInfo_Release(http_info);

    if(FAILED(hres))
        return HTTP_STATUS_OK;
    return status;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface,
        ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    DWORD status_code;

    TRACE("(%p)->(%d %d %d %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
          debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_REDIRECTING:
        return set_dochost_url(This->doc_host, szStatusText);
    case BINDSTATUS_BEGINDOWNLOADDATA:
        set_status_text(This, ulStatusCode, szStatusText);
        status_code = get_http_status_code(This->binding);
        if(status_code != HTTP_STATUS_OK)
            handle_navigation_error(This->doc_host, status_code, This->url, NULL);
        return S_OK;

    case BINDSTATUS_FINDINGRESOURCE:
    case BINDSTATUS_ENDDOWNLOADDATA:
    case BINDSTATUS_SENDINGREQUEST:
        set_status_text(This, ulStatusCode, szStatusText);
        return S_OK;

    case BINDSTATUS_CONNECTING:
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
    case BINDSTATUS_CLASSIDAVAILABLE:
    case BINDSTATUS_MIMETYPEAVAILABLE:
    case BINDSTATUS_BEGINSYNCOPERATION:
    case BINDSTATUS_ENDSYNCOPERATION:
        return S_OK;
    default:
        FIXME("status code %u\n", ulStatusCode);
    }

    return S_OK;
}

void handle_navigation_error(DocHost* doc_host, HRESULT hres, BSTR url, IHTMLWindow2 *win2)
{
    VARIANT var_status_code, var_frame_name, var_url;
    DISPPARAMS dispparams;
    VARIANTARG params[5];
    VARIANT_BOOL cancel = VARIANT_FALSE;

    dispparams.cArgs = 5;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = VT_BOOL|VT_BYREF;
    V_BOOLREF(params) = &cancel;

    V_VT(params+1) = VT_VARIANT|VT_BYREF;
    V_VARIANTREF(params+1) = &var_status_code;
    V_VT(&var_status_code) = VT_I4;
    V_I4(&var_status_code) = hres;

    V_VT(params+2) = VT_VARIANT|VT_BYREF;
    V_VARIANTREF(params+2) = &var_frame_name;
    V_VT(&var_frame_name) = VT_BSTR;
    if(win2) {
        hres = IHTMLWindow2_get_name(win2, &V_BSTR(&var_frame_name));
        if(FAILED(hres))
            V_BSTR(&var_frame_name) = NULL;
    } else
        V_BSTR(&var_frame_name) = NULL;

    V_VT(params+3) = VT_VARIANT|VT_BYREF;
    V_VARIANTREF(params+3) = &var_url;
    V_VT(&var_url) = VT_BSTR;
    V_BSTR(&var_url) = url;

    V_VT(params+4) = VT_DISPATCH;
    V_DISPATCH(params+4) = (IDispatch*)doc_host->wb;

    call_sink(doc_host->cps.wbe2, DISPID_NAVIGATEERROR, &dispparams);
    SysFreeString(V_BSTR(&var_frame_name));

    if(!cancel)
        FIXME("Navigate to error page\n");
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    set_status_text(This, 0, emptyW);

    if(!This->doc_host)
        return S_OK;

    if(!This->doc_host->olecmd)
        notify_download_state(This->doc_host, FALSE);
    if(FAILED(hresult))
        handle_navigation_error(This->doc_host, hresult, This->url, NULL);

    IOleClientSite_Release(&This->doc_host->IOleClientSite_iface);
    This->doc_host = NULL;

    IBinding_Release(This->binding);
    This->binding = NULL;

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->cbstgmedData = This->post_data_len;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)&This->IBindStatusCallback_iface;
        IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);

    return dochost_object_available(This->doc_host, punk);
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

static inline BindStatusCallback *impl_from_IHttpNegotiate(IHttpNegotiate *iface)
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IHttpNegotiate_iface);
}

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate *iface,
                                                   REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);

    TRACE("(%p)->(%s %s %d %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    if(This->headers) {
        int size = (strlenW(This->headers)+1)*sizeof(WCHAR);
        *pszAdditionalHeaders = CoTaskMemAlloc(size);
        memcpy(*pszAdditionalHeaders, This->headers, size);
    }

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate *iface,
        DWORD dwResponseCode, LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders,
        LPWSTR *pszAdditionalRequestHeaders)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    TRACE("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return S_OK;
}

static const IHttpNegotiateVtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse
};

static inline BindStatusCallback *impl_from_IHttpSecurity(IHttpSecurity *iface)
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IHttpSecurity_iface);
}

static HRESULT WINAPI HttpSecurity_QueryInterface(IHttpSecurity *iface, REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IHttpSecurity(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI HttpSecurity_AddRef(IHttpSecurity *iface)
{
    BindStatusCallback *This = impl_from_IHttpSecurity(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI HttpSecurity_Release(IHttpSecurity *iface)
{
    BindStatusCallback *This = impl_from_IHttpSecurity(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI HttpSecurity_GetWindow(IHttpSecurity *iface, REFGUID rguidReason, HWND *phwnd)
{
    BindStatusCallback *This = impl_from_IHttpSecurity(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(rguidReason), phwnd);

    if(!This->doc_host)
        return E_FAIL;

    *phwnd = This->doc_host->frame_hwnd;
    return S_OK;
}

static HRESULT WINAPI HttpSecurity_OnSecurityProblem(IHttpSecurity *iface, DWORD dwProblem)
{
    BindStatusCallback *This = impl_from_IHttpSecurity(iface);
    FIXME("(%p)->(%u)\n", This, dwProblem);
    return S_FALSE;
}

static const IHttpSecurityVtbl HttpSecurityVtbl = {
    HttpSecurity_QueryInterface,
    HttpSecurity_AddRef,
    HttpSecurity_Release,
    HttpSecurity_GetWindow,
    HttpSecurity_OnSecurityProblem
};

static BindStatusCallback *create_callback(DocHost *doc_host, LPCWSTR url, PBYTE post_data,
        ULONG post_data_len, LPCWSTR headers)
{
    BindStatusCallback *ret = heap_alloc(sizeof(BindStatusCallback));

    ret->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    ret->IHttpNegotiate_iface.lpVtbl      = &HttpNegotiateVtbl;
    ret->IHttpSecurity_iface.lpVtbl       = &HttpSecurityVtbl;

    ret->ref = 1;
    ret->url = SysAllocString(url);
    ret->post_data = NULL;
    ret->post_data_len = post_data_len;
    ret->headers = headers ? SysAllocString(headers) : NULL;

    ret->doc_host = doc_host;
    IOleClientSite_AddRef(&doc_host->IOleClientSite_iface);

    ret->binding = NULL;

    if(post_data) {
        ret->post_data = GlobalAlloc(0, post_data_len);
        memcpy(ret->post_data, post_data, post_data_len);
    }

    return ret;
}

static void on_before_navigate2(DocHost *This, LPCWSTR url, SAFEARRAY *post_data, LPWSTR headers, VARIANT_BOOL *cancel)
{
    VARIANT var_url, var_flags, var_frame_name, var_post_data, var_post_data2, var_headers;
    DISPPARAMS dispparams;
    VARIANTARG params[7];
    WCHAR file_path[MAX_PATH];
    DWORD file_path_len = sizeof(file_path) / sizeof(*file_path);

    dispparams.cArgs = 7;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    This->busy = VARIANT_TRUE;

    V_VT(params) = VT_BOOL|VT_BYREF;
    V_BOOLREF(params) = cancel;

    V_VT(params+1) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+1) = &var_headers;
    V_VT(&var_headers) = VT_BSTR;
    V_BSTR(&var_headers) = headers;

    V_VT(params+2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+2) = &var_post_data2;
    V_VT(&var_post_data2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(&var_post_data2) = &var_post_data;

    if(post_data) {
        V_VT(&var_post_data) = VT_UI1|VT_ARRAY;
        V_ARRAY(&var_post_data) = post_data;
    }else {
        V_VT(&var_post_data) = VT_EMPTY;
    }

    V_VT(params+3) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+3) = &var_frame_name;
    V_VT(&var_frame_name) = VT_BSTR;
    V_BSTR(&var_frame_name) = NULL;

    V_VT(params+4) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+4) = &var_flags;
    V_VT(&var_flags) = VT_I4;
    V_I4(&var_flags) = 0;

    V_VT(params+5) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+5) = &var_url;
    V_VT(&var_url) = VT_BSTR;
    if(PathCreateFromUrlW(url, file_path, &file_path_len, 0) == S_OK)
        V_BSTR(&var_url) = SysAllocString(file_path);
    else
        V_BSTR(&var_url) = SysAllocString(url);

    V_VT(params+6) = (VT_DISPATCH);
    V_DISPATCH(params+6) = (IDispatch*)This->wb;

    call_sink(This->cps.wbe2, DISPID_BEFORENAVIGATE2, &dispparams);

    SysFreeString(V_BSTR(&var_url));
}

/* FIXME: urlmon should handle it */
static BOOL try_application_url(LPCWSTR url)
{
    SHELLEXECUTEINFOW exec_info;
    WCHAR app[64];
    HKEY hkey;
    DWORD res, type;
    HRESULT hres;

    static const WCHAR wszURLProtocol[] = {'U','R','L',' ','P','r','o','t','o','c','o','l',0};

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, app, sizeof(app)/sizeof(WCHAR), NULL, 0);
    if(FAILED(hres))
        return FALSE;

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, app, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, wszURLProtocol, NULL, &type, NULL, NULL);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    TRACE("opening application %s\n", debugstr_w(app));

    memset(&exec_info, 0, sizeof(exec_info));
    exec_info.cbSize = sizeof(exec_info);
    exec_info.lpFile = url;
    exec_info.nShow = SW_SHOW;

    return ShellExecuteExW(&exec_info);
}

static HRESULT create_moniker(LPCWSTR url, IMoniker **mon)
{
    WCHAR new_url[INTERNET_MAX_URL_LENGTH];
    DWORD size;
    HRESULT hres;

    if(PathIsURLW(url))
        return CreateURLMoniker(NULL, url, mon);

    size = sizeof(new_url)/sizeof(WCHAR);
    hres = UrlApplySchemeW(url, new_url, &size, URL_APPLY_GUESSSCHEME | URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT);
    TRACE("was %s got %s\n", debugstr_w(url), debugstr_w(new_url));
    if(FAILED(hres)) {
        WARN("UrlApplyScheme failed: %08x\n", hres);
        return hres;
    }

    return CreateURLMoniker(NULL, new_url, mon);
}

static HRESULT bind_to_object(DocHost *This, IMoniker *mon, LPCWSTR url, IBindCtx *bindctx,
                              IBindStatusCallback *callback)
{
    IUnknown *unk = NULL;
    WCHAR *display_name;
    HRESULT hres;

    if(mon) {
        IMoniker_AddRef(mon);
    }else {
        hres = create_moniker(url, &mon);
        if(FAILED(hres))
            return hres;
    }

    hres = IMoniker_GetDisplayName(mon, 0, NULL, &display_name);
    if(FAILED(hres)) {
        FIXME("GetDisplayName failed: %08x\n", hres);
        return hres;
    }

    hres = set_dochost_url(This, display_name);
    CoTaskMemFree(display_name);
    if(FAILED(hres))
        return hres;

    IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM,
                                 (IUnknown*)&This->IOleClientSite_iface);

    hres = IMoniker_BindToObject(mon, bindctx, NULL, &IID_IUnknown, (void**)&unk);
    if(SUCCEEDED(hres)) {
        hres = S_OK;
        if(unk)
            IUnknown_Release(unk);
    }else if(try_application_url(url)) {
        hres = S_OK;
    }else {
        FIXME("BindToObject failed: %08x\n", hres);
    }

    IMoniker_Release(mon);
    return S_OK;
}

static void html_window_navigate(DocHost *This, IHTMLPrivateWindow *window, BSTR url, BSTR headers, SAFEARRAY *post_data)
{
    VARIANT headers_var, post_data_var;
    BSTR empty_str;
    HRESULT hres;

    hres = set_dochost_url(This, url);
    if(FAILED(hres))
        return;

    empty_str = SysAllocStringLen(NULL, 0);

    if(headers) {
        V_VT(&headers_var) = VT_BSTR;
        V_BSTR(&headers_var) = headers;
    }else {
        V_VT(&headers_var) = VT_EMPTY;
    }

    if(post_data) {
        V_VT(&post_data_var) = VT_UI1|VT_ARRAY;
        V_ARRAY(&post_data_var) = post_data;
    }else {
        V_VT(&post_data_var) = VT_EMPTY;
    }

    set_doc_state(This, READYSTATE_LOADING);
    hres = IHTMLPrivateWindow_SuperNavigate(window, url, empty_str, NULL, NULL, &post_data_var, &headers_var, 0);
    SysFreeString(empty_str);
    if(FAILED(hres))
        WARN("SuprtNavigate failed: %08x\n", hres);
}

typedef struct {
    task_header_t header;
    BSTR url;
    BSTR headers;
    SAFEARRAY *post_data;
    BOOL async_notif;
} task_doc_navigate_t;

static void doc_navigate_task_destr(task_header_t *t)
{
    task_doc_navigate_t *task = (task_doc_navigate_t*)t;

    SysFreeString(task->url);
    SysFreeString(task->headers);
    if(task->post_data)
        SafeArrayDestroy(task->post_data);
    heap_free(task);
}

static void doc_navigate_proc(DocHost *This, task_header_t *t)
{
    task_doc_navigate_t *task = (task_doc_navigate_t*)t;
    IHTMLPrivateWindow *priv_window;
    HRESULT hres;

    if(!This->doc_navigate) {
        ERR("Skip nav\n");
        return;
    }

    if(task->async_notif) {
        VARIANT_BOOL cancel = VARIANT_FALSE;
        on_before_navigate2(This, task->url, task->post_data, task->headers, &cancel);
        if(cancel) {
            TRACE("Navigation canceled\n");
            return;
        }
    }

    hres = IUnknown_QueryInterface(This->doc_navigate, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    if(SUCCEEDED(hres)) {
        html_window_navigate(This, priv_window, task->url, task->headers, task->post_data);
        IHTMLPrivateWindow_Release(priv_window);
    }else {
        WARN("Could not get IHTMLPrivateWindow iface: %08x\n", hres);
    }
}

static HRESULT async_doc_navigate(DocHost *This, LPCWSTR url, LPCWSTR headers, PBYTE post_data, ULONG post_data_size,
        BOOL async_notif)
{
    task_doc_navigate_t *task;

    TRACE("%s\n", debugstr_w(url));

    task = heap_alloc_zero(sizeof(*task));
    if(!task)
        return E_OUTOFMEMORY;

    task->url = SysAllocString(url);
    if(!task->url) {
        doc_navigate_task_destr(&task->header);
        return E_OUTOFMEMORY;
    }

    if(headers) {
        task->headers = SysAllocString(headers);
        if(!task->headers) {
            doc_navigate_task_destr(&task->header);
            return E_OUTOFMEMORY;
        }
    }

    if(post_data) {
        task->post_data = SafeArrayCreateVector(VT_UI1, 0, post_data_size);
        if(!task->post_data) {
            doc_navigate_task_destr(&task->header);
            return E_OUTOFMEMORY;
        }

        memcpy(task->post_data->pvData, post_data, post_data_size);
    }

    if(!async_notif) {
        VARIANT_BOOL cancel = VARIANT_FALSE;

        on_before_navigate2(This, task->url, task->post_data, task->headers, &cancel);
        if(cancel) {
            TRACE("Navigation canceled\n");
            doc_navigate_task_destr(&task->header);
            return S_OK;
        }
    }

    task->async_notif = async_notif;
    abort_dochost_tasks(This, doc_navigate_proc);
    push_dochost_task(This, &task->header, doc_navigate_proc, doc_navigate_task_destr, FALSE);
    return S_OK;
}

static HRESULT navigate_bsc(DocHost *This, BindStatusCallback *bsc, IMoniker *mon)
{
    VARIANT_BOOL cancel = VARIANT_FALSE;
    SAFEARRAY *post_data = NULL;
    IBindCtx *bindctx;
    HRESULT hres;

    set_doc_state(This, READYSTATE_LOADING);

    if(bsc->post_data) {
        post_data = SafeArrayCreateVector(VT_UI1, 0, bsc->post_data_len);
        memcpy(post_data->pvData, post_data, bsc->post_data_len);
    }

    on_before_navigate2(This, bsc->url, post_data, bsc->headers, &cancel);
    if(post_data)
        SafeArrayDestroy(post_data);
    if(cancel) {
        FIXME("Navigation canceled\n");
        return S_OK;
    }

    notify_download_state(This, TRUE);
    on_commandstate_change(This, CSC_NAVIGATEBACK, VARIANT_FALSE);
    on_commandstate_change(This, CSC_NAVIGATEFORWARD, VARIANT_FALSE);

    if(This->document)
        deactivate_document(This);

    CreateAsyncBindCtx(0, &bsc->IBindStatusCallback_iface, 0, &bindctx);

    if(This->frame)
        IOleInPlaceFrame_EnableModeless(This->frame, FALSE);

    hres = bind_to_object(This, mon, bsc->url, bindctx, &bsc->IBindStatusCallback_iface);

    if(This->frame)
        IOleInPlaceFrame_EnableModeless(This->frame, TRUE);

    IBindCtx_Release(bindctx);

    return hres;
}

typedef struct {
    task_header_t header;
    BindStatusCallback *bsc;
} task_navigate_bsc_t;

static void navigate_bsc_task_destr(task_header_t *t)
{
    task_navigate_bsc_t *task = (task_navigate_bsc_t*)t;

    IBindStatusCallback_Release(&task->bsc->IBindStatusCallback_iface);
    heap_free(task);
}

static void navigate_bsc_proc(DocHost *This, task_header_t *t)
{
    task_navigate_bsc_t *task = (task_navigate_bsc_t*)t;

    if(!This->hwnd)
        create_doc_view_hwnd(This);

    navigate_bsc(This, task->bsc, NULL);
}


HRESULT navigate_url(DocHost *This, LPCWSTR url, const VARIANT *Flags,
                     const VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    SAFEARRAY *post_array = NULL;
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL;
    HRESULT hres = S_OK;

    TRACE("navigating to %s\n", debugstr_w(url));

    if((Flags && V_VT(Flags) != VT_EMPTY && V_VT(Flags) != VT_ERROR)
       || (TargetFrameName && V_VT(TargetFrameName) != VT_EMPTY && V_VT(TargetFrameName) != VT_ERROR))
        FIXME("Unsupported args (Flags %s; TargetFrameName %s)\n", debugstr_variant(Flags), debugstr_variant(TargetFrameName));

    if(PostData) {
        if(V_VT(PostData) & VT_ARRAY)
            post_array = V_ISBYREF(PostData) ? *V_ARRAYREF(PostData) : V_ARRAY(PostData);
        else
            WARN("Invalid post data %s\n", debugstr_variant(PostData));
    }

    if(post_array) {
        LONG elem_max;
        SafeArrayAccessData(post_array, (void**)&post_data);
        SafeArrayGetUBound(post_array, 1, &elem_max);
        post_data_len = (elem_max+1) * SafeArrayGetElemsize(post_array);
    }

    if(Headers && V_VT(Headers) == VT_BSTR) {
        headers = V_BSTR(Headers);
        TRACE("Headers: %s\n", debugstr_w(headers));
    }

    set_doc_state(This, READYSTATE_LOADING);
    This->ready_state = READYSTATE_LOADING;

    if(This->doc_navigate) {
        WCHAR new_url[INTERNET_MAX_URL_LENGTH];

        if(PathIsURLW(url)) {
            new_url[0] = 0;
        }else {
            DWORD size;

            size = sizeof(new_url)/sizeof(WCHAR);
            hres = UrlApplySchemeW(url, new_url, &size,
                    URL_APPLY_GUESSSCHEME | URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT);
            if(FAILED(hres)) {
                WARN("UrlApplyScheme failed: %08x\n", hres);
                new_url[0] = 0;
            }
        }

        hres = async_doc_navigate(This, *new_url ? new_url : url, headers, post_data,
                post_data_len, TRUE);
    }else {
        task_navigate_bsc_t *task;

        task = heap_alloc(sizeof(*task));
        task->bsc = create_callback(This, url, post_data, post_data_len, headers);
        push_dochost_task(This, &task->header, navigate_bsc_proc, navigate_bsc_task_destr, This->url == NULL);
    }

    if(post_data)
        SafeArrayUnaccessData(post_array);

    return hres;
}

static HRESULT navigate_hlink(DocHost *This, IMoniker *mon, IBindCtx *bindctx,
                              IBindStatusCallback *callback)
{
    IHttpNegotiate *http_negotiate;
    BindStatusCallback *bsc;
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL, url;
    BINDINFO bindinfo;
    DWORD bindf = 0;
    HRESULT hres;

    TRACE("\n");

    hres = IMoniker_GetDisplayName(mon, 0, NULL, &url);
    if(FAILED(hres))
        FIXME("GetDisplayName failed: %08x\n", hres);

    hres = IBindStatusCallback_QueryInterface(callback, &IID_IHttpNegotiate,
                                              (void**)&http_negotiate);
    if(SUCCEEDED(hres)) {
        static const WCHAR null_string[] = {0};

        IHttpNegotiate_BeginningTransaction(http_negotiate, null_string, null_string, 0,
                                            &headers);
        IHttpNegotiate_Release(http_negotiate);
    }

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    hres = IBindStatusCallback_GetBindInfo(callback, &bindf, &bindinfo);
    dump_BINDINFO(&bindinfo);
    if(bindinfo.dwBindVerb == BINDVERB_POST) {
        post_data_len = bindinfo.cbstgmedData;
        if(post_data_len)
            post_data = bindinfo.stgmedData.u.hGlobal;
    }

    if(This->doc_navigate) {
        hres = async_doc_navigate(This, url, headers, post_data, post_data_len, FALSE);
    }else {
        bsc = create_callback(This, url, post_data, post_data_len, headers);
        hres = navigate_bsc(This, bsc, mon);
        IBindStatusCallback_Release(&bsc->IBindStatusCallback_iface);
    }

    CoTaskMemFree(url);
    CoTaskMemFree(headers);
    ReleaseBindInfo(&bindinfo);

    return hres;
}

HRESULT go_home(DocHost *This)
{
    HKEY hkey;
    DWORD res, type, size;
    WCHAR wszPageName[MAX_PATH];
    static const WCHAR wszAboutBlank[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
    static const WCHAR wszStartPage[] = {'S','t','a','r','t',' ','P','a','g','e',0};
    static const WCHAR wszSubKey[] = {'S','o','f','t','w','a','r','e','\\',
                                      'M','i','c','r','o','s','o','f','t','\\',
                                      'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
                                      'M','a','i','n',0};

    res = RegOpenKeyW(HKEY_CURRENT_USER, wszSubKey, &hkey);
    if (res != ERROR_SUCCESS)
        return navigate_url(This, wszAboutBlank, NULL, NULL, NULL, NULL);

    size = sizeof(wszPageName);
    res = RegQueryValueExW(hkey, wszStartPage, NULL, &type, (LPBYTE)wszPageName, &size);
    RegCloseKey(hkey);
    if (res != ERROR_SUCCESS || type != REG_SZ)
        return navigate_url(This, wszAboutBlank, NULL, NULL, NULL, NULL);

    return navigate_url(This, wszPageName, NULL, NULL, NULL, NULL);
}

static HRESULT navigate_history(DocHost *This, unsigned travellog_pos)
{
    IPersistHistory *persist_history;
    travellog_entry_t *entry;
    LARGE_INTEGER li;
    HRESULT hres;

    if(!This->doc_navigate) {
        FIXME("unsupported doc_navigate FALSE\n");
        return E_NOTIMPL;
    }

    This->travellog.loading_pos = travellog_pos;
    entry = This->travellog.log + This->travellog.loading_pos;

    update_navigation_commands(This);

    if(!entry->stream)
        return async_doc_navigate(This, entry->url, NULL, NULL, 0, FALSE);

    hres = IUnknown_QueryInterface(This->document, &IID_IPersistHistory, (void**)&persist_history);
    if(FAILED(hres))
        return hres;

    li.QuadPart = 0;
    IStream_Seek(entry->stream, li, STREAM_SEEK_SET, NULL);

    hres = IPersistHistory_LoadHistory(persist_history, entry->stream, NULL);
    IPersistHistory_Release(persist_history);
    return hres;
}

HRESULT go_back(DocHost *This)
{
    if(!This->travellog.position) {
        WARN("No history available\n");
        return E_FAIL;
    }

    return navigate_history(This, This->travellog.position-1);
}

HRESULT go_forward(DocHost *This)
{
    if(This->travellog.position >= This->travellog.length) {
        WARN("No history available\n");
        return E_FAIL;
    }

    return navigate_history(This, This->travellog.position+1);
}

HRESULT get_location_url(DocHost *This, BSTR *ret)
{
    FIXME("semi-stub\n");

    *ret = This->url ? SysAllocString(This->url) : SysAllocStringLen(NULL, 0);
    if(!*ret)
        return E_OUTOFMEMORY;

    return This->url ? S_OK : S_FALSE;
}

static inline HlinkFrame *impl_from_IHlinkFrame(IHlinkFrame *iface)
{
    return CONTAINING_RECORD(iface, HlinkFrame, IHlinkFrame_iface);
}

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    IMoniker *mon;
    LPWSTR location = NULL;

    TRACE("(%p)->(%08x %p %p %p)\n", This, grfHLNF, pbc, pibsc, pihlNavigate);

    if(grfHLNF)
        FIXME("unsupported grfHLNF=%08x\n", grfHLNF);

    /* Windows calls GetTargetFrameName here. */

    IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);

    if(location) {
        FIXME("location = %s\n", debugstr_w(location));
        CoTaskMemFree(location);
    }

    /* Windows calls GetHlinkSite here */

    if(grfHLNF & HLNF_OPENINNEWWINDOW) {
        FIXME("Not supported HLNF_OPENINNEWWINDOW\n");
        return E_NOTIMPL;
    }

    return navigate_hlink(This->doc_host, mon, pbc, pibsc);
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    FIXME("(%p)->(%08x %p %s %s %d)\n", This, grfHLNF, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), dwreserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    HlinkFrame *This = impl_from_IHlinkFrame(iface);
    FIXME("(%p)->(%u %p %s %s)\n", This, uHLID, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName));
    return E_NOTIMPL;
}

static const IHlinkFrameVtbl HlinkFrameVtbl = {
    HlinkFrame_QueryInterface,
    HlinkFrame_AddRef,
    HlinkFrame_Release,
    HlinkFrame_SetBrowseContext,
    HlinkFrame_GetBrowseContext,
    HlinkFrame_Navigate,
    HlinkFrame_OnNavigate,
    HlinkFrame_UpdateHlink
};

static inline HlinkFrame *impl_from_ITargetFrame2(ITargetFrame2 *iface)
{
    return CONTAINING_RECORD(iface, HlinkFrame, IHlinkFrame_iface);
}

static HRESULT WINAPI TargetFrame2_QueryInterface(ITargetFrame2 *iface, REFIID riid, void **ppv)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI TargetFrame2_AddRef(ITargetFrame2 *iface)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI TargetFrame2_Release(ITargetFrame2 *iface)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI TargetFrame2_SetFrameName(ITargetFrame2 *iface, LPCWSTR pszFrameName)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFrameName));
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetFrameName(ITargetFrame2 *iface, LPWSTR *ppszFrameName)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%p)\n", This, ppszFrameName);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetParentFrame(ITargetFrame2 *iface, IUnknown **ppunkParent)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%p)\n", This, ppunkParent);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_SetFrameSrc(ITargetFrame2 *iface, LPCWSTR pszFrameSrc)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFrameSrc));
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetFrameSrc(ITargetFrame2 *iface, LPWSTR *ppszFrameSrc)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetFramesContainer(ITargetFrame2 *iface, IOleContainer **ppContainer)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%p)\n", This, ppContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_SetFrameOptions(ITargetFrame2 *iface, DWORD dwFlags)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%x)\n", This, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetFrameOptions(ITargetFrame2 *iface, DWORD *pdwFlags)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%p)\n", This, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_SetFrameMargins(ITargetFrame2 *iface, DWORD dwWidth, DWORD dwHeight)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%d %d)\n", This, dwWidth, dwHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetFrameMargins(ITargetFrame2 *iface, DWORD *pdwWidth, DWORD *pdwHeight)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%p %p)\n", This, pdwWidth, pdwHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_FindFrame(ITargetFrame2 *iface, LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%s %x %p)\n", This, debugstr_w(pszTargetName), dwFlags, ppunkTargetFrame);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFrame2_GetTargetAlias(ITargetFrame2 *iface, LPCWSTR pszTargetName, LPWSTR *ppszTargetAlias)
{
    HlinkFrame *This = impl_from_ITargetFrame2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pszTargetName), ppszTargetAlias);
    return E_NOTIMPL;
}

static const ITargetFrame2Vtbl TargetFrame2Vtbl = {
    TargetFrame2_QueryInterface,
    TargetFrame2_AddRef,
    TargetFrame2_Release,
    TargetFrame2_SetFrameName,
    TargetFrame2_GetFrameName,
    TargetFrame2_GetParentFrame,
    TargetFrame2_SetFrameSrc,
    TargetFrame2_GetFrameSrc,
    TargetFrame2_GetFramesContainer,
    TargetFrame2_SetFrameOptions,
    TargetFrame2_GetFrameOptions,
    TargetFrame2_SetFrameMargins,
    TargetFrame2_GetFrameMargins,
    TargetFrame2_FindFrame,
    TargetFrame2_GetTargetAlias
};

static inline HlinkFrame *impl_from_ITargetFramePriv2(ITargetFramePriv2 *iface)
{
    return CONTAINING_RECORD(iface, HlinkFrame, ITargetFramePriv2_iface);
}

static HRESULT WINAPI TargetFramePriv2_QueryInterface(ITargetFramePriv2 *iface, REFIID riid, void **ppv)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI TargetFramePriv2_AddRef(ITargetFramePriv2 *iface)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI TargetFramePriv2_Release(ITargetFramePriv2 *iface)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI TargetFramePriv2_FindFrameDownwards(ITargetFramePriv2 *iface,
        LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%s %x %p)\n", This, debugstr_w(pszTargetName), dwFlags, ppunkTargetFrame);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_FindFrameInContext(ITargetFramePriv2 *iface,
        LPCWSTR pszTargetName, IUnknown *punkContextFrame, DWORD dwFlags, IUnknown **ppunkTargetFrame)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%s %p %x %p)\n", This, debugstr_w(pszTargetName), punkContextFrame, dwFlags, ppunkTargetFrame);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_OnChildFrameActivate(ITargetFramePriv2 *iface, IUnknown *pUnkChildFrame)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%p)\n", This, pUnkChildFrame);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_OnChildFrameDeactivate(ITargetFramePriv2 *iface, IUnknown *pUnkChildFrame)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%p)\n", This, pUnkChildFrame);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_NavigateHack(ITargetFramePriv2 *iface, DWORD grfHLNF, LPBC pbc,
        IBindStatusCallback *pibsc, LPCWSTR pszTargetName, LPCWSTR pszUrl, LPCWSTR pszLocation)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%x %p %p %s %s %s)\n", This, grfHLNF, pbc, pibsc, debugstr_w(pszTargetName),
          debugstr_w(pszUrl), debugstr_w(pszLocation));
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_FindBrowserByIndex(ITargetFramePriv2 *iface, DWORD dwID, IUnknown **ppunkBrowser)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    FIXME("(%p)->(%d %p)\n", This, dwID, ppunkBrowser);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetFramePriv2_AggregatedNavigation2(ITargetFramePriv2 *iface, DWORD grfHLNF, LPBC pbc,
        IBindStatusCallback *pibsc, LPCWSTR pszTargetName, IUri *pUri, LPCWSTR pszLocation)
{
    HlinkFrame *This = impl_from_ITargetFramePriv2(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%x %p %p %s %p %s)\n", This, grfHLNF, pbc, pibsc, debugstr_w(pszTargetName),
          pUri, debugstr_w(pszLocation));

    /*
     * NOTE: This is an undocumented function. It seems to be working the way it's implemented,
     * but I couldn't get its tests working. It's used by mshtml to load content in a new
     * instance of browser.
     */

    hres = CreateURLMonikerEx2(NULL, pUri, &mon, 0);
    if(FAILED(hres))
        return hres;

    hres = navigate_hlink(This->doc_host, mon, pbc, pibsc);
    IMoniker_Release(mon);
    return hres;
}

static const ITargetFramePriv2Vtbl TargetFramePriv2Vtbl = {
    TargetFramePriv2_QueryInterface,
    TargetFramePriv2_AddRef,
    TargetFramePriv2_Release,
    TargetFramePriv2_FindFrameDownwards,
    TargetFramePriv2_FindFrameInContext,
    TargetFramePriv2_OnChildFrameActivate,
    TargetFramePriv2_OnChildFrameDeactivate,
    TargetFramePriv2_NavigateHack,
    TargetFramePriv2_FindBrowserByIndex,
    TargetFramePriv2_AggregatedNavigation2
};

static inline HlinkFrame *impl_from_IWebBrowserPriv2IE9(IWebBrowserPriv2IE9 *iface)
{
    return CONTAINING_RECORD(iface, HlinkFrame, IWebBrowserPriv2IE9_iface);
}

static HRESULT WINAPI WebBrowserPriv2IE9_QueryInterface(IWebBrowserPriv2IE9 *iface, REFIID riid, void **ppv)
{
    HlinkFrame *This = impl_from_IWebBrowserPriv2IE9(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI WebBrowserPriv2IE9_AddRef(IWebBrowserPriv2IE9 *iface)
{
    HlinkFrame *This = impl_from_IWebBrowserPriv2IE9(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI WebBrowserPriv2IE9_Release(IWebBrowserPriv2IE9 *iface)
{
    HlinkFrame *This = impl_from_IWebBrowserPriv2IE9(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI WebBrowserPriv2IE9_NavigateWithBindCtx2(IWebBrowserPriv2IE9 *iface, IUri *uri, VARIANT *flags,
        VARIANT *target_frame, VARIANT *post_data, VARIANT *headers, IBindCtx *bind_ctx, LPOLESTR url_fragment, DWORD unused)
{
    HlinkFrame *This = impl_from_IWebBrowserPriv2IE9(iface);
    FIXME("(%p)->(%p %s %s %s %s %p %s)\n", This, uri, debugstr_variant(flags), debugstr_variant(target_frame),
          debugstr_variant(post_data), debugstr_variant(headers), bind_ctx, debugstr_w(url_fragment));
    return E_NOTIMPL;
}

static const IWebBrowserPriv2IE9Vtbl WebBrowserPriv2IE9Vtbl = {
    WebBrowserPriv2IE9_QueryInterface,
    WebBrowserPriv2IE9_AddRef,
    WebBrowserPriv2IE9_Release,
    WebBrowserPriv2IE9_NavigateWithBindCtx2
};

BOOL HlinkFrame_QI(HlinkFrame *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHlinkFrame, riid)) {
        TRACE("(%p)->(IID_IHlinkFrame %p)\n", This, ppv);
        *ppv = &This->IHlinkFrame_iface;
    }else if(IsEqualGUID(&IID_ITargetFrame2, riid)) {
        TRACE("(%p)->(IID_ITargetFrame2 %p)\n", This, ppv);
        *ppv = &This->ITargetFrame2_iface;
    }else if(IsEqualGUID(&IID_ITargetFramePriv, riid)) {
        TRACE("(%p)->(IID_ITargetFramePriv %p)\n", This, ppv);
        *ppv = &This->ITargetFramePriv2_iface;
    }else if(IsEqualGUID(&IID_ITargetFramePriv2, riid)) {
        TRACE("(%p)->(IID_ITargetFramePriv2 %p)\n", This, ppv);
        *ppv = &This->ITargetFramePriv2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowserPriv2IE9, riid)) {
        TRACE("(%p)->(IID_IWebBrowserPriv2IE9 %p)\n", This, ppv);
        *ppv = &This->IWebBrowserPriv2IE9_iface;
    }else {
        return FALSE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return TRUE;
}

void HlinkFrame_Init(HlinkFrame *This, IUnknown *outer, DocHost *doc_host)
{
    This->IHlinkFrame_iface.lpVtbl   = &HlinkFrameVtbl;
    This->ITargetFrame2_iface.lpVtbl = &TargetFrame2Vtbl;
    This->ITargetFramePriv2_iface.lpVtbl = &TargetFramePriv2Vtbl;
    This->IWebBrowserPriv2IE9_iface.lpVtbl = &WebBrowserPriv2IE9Vtbl;

    This->outer = outer;
    This->doc_host = doc_host;
}
