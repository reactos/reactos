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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"

#include "shdocvw.h"
#include "mshtml.h"
#include "exdispid.h"
#include "shellapi.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static const WCHAR emptyW[] = {0};

typedef struct {
    const IBindStatusCallbackVtbl  *lpBindStatusCallbackVtbl;
    const IHttpNegotiateVtbl       *lpHttpNegotiateVtbl;

    LONG ref;

    DocHost *doc_host;

    LPWSTR url;
    HGLOBAL post_data;
    BSTR headers;
    ULONG post_data_len;
} BindStatusCallback;

#define BINDSC(x)  ((IBindStatusCallback*) &(x)->lpBindStatusCallbackVtbl)
#define HTTPNEG(x) ((IHttpNegotiate*)      &(x)->lpHttpNegotiateVtbl)

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

static void set_status_text(BindStatusCallback *This, LPCWSTR str)
{
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    if(!This->doc_host)
        return;

    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = str ? SysAllocString(str) : NULL;
    call_sink(This->doc_host->cps.wbe2, DISPID_STATUSTEXTCHANGE, &dispparams);
    VariantClear(&arg);

    if(This->doc_host->frame)
        IOleInPlaceFrame_SetStatusText(This->doc_host->frame, str);
}

#define BINDSC_THIS(iface) DEFINE_THIS(BindStatusCallback, BindStatusCallback, iface)

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
                                                        REFIID riid, void **ppv)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDSC(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = BINDSC(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(BINDSC(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->doc_host)
            IOleClientSite_Release(CLIENTSITE(This->doc_host));
        if(This->post_data)
            GlobalFree(This->post_data);
        SysFreeString(This->headers);
        heap_free(This->url);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
       DWORD dwReserved, IBinding *pbind)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    TRACE("(%p)->(%d %p)\n", This, dwReserved, pbind);

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface,
       LONG *pnPriority)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface,
       DWORD reserved)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface,
        ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    TRACE("(%p)->(%d %d %d %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
          debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        set_status_text(This, szStatusText); /* FIXME: "Start downloading from site: %s" */
        return S_OK;
    case BINDSTATUS_ENDDOWNLOADDATA:
        set_status_text(This, szStatusText); /* FIXME: "Downloading from site: %s" */
        return S_OK;
    case BINDSTATUS_CLASSIDAVAILABLE:
    case BINDSTATUS_MIMETYPEAVAILABLE:
    case BINDSTATUS_BEGINSYNCOPERATION:
    case BINDSTATUS_ENDSYNCOPERATION:
        return S_OK;
    default:
        FIXME("status code %u\n", ulStatusCode);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    set_status_text(This, emptyW);

    if(This->doc_host) {
        IOleClientSite_Release(CLIENTSITE(This->doc_host));
        This->doc_host = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->cbstgmedData = This->post_data_len;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)BINDSC(This);
        IBindStatusCallback_AddRef(BINDSC(This));
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);
    return E_NOTIMPL;
}

static void object_available_proc(DocHost *This, task_header_t *task)
{
    object_available(This);
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    task_header_t *task;
    IOleObject *oleobj;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);

    IUnknown_AddRef(punk);
    This->doc_host->document = punk;

    hres = IUnknown_QueryInterface(punk, &IID_IOleObject, (void**)&oleobj);
    if(SUCCEEDED(hres)) {
        CLSID clsid;

        hres = IOleObject_GetUserClassID(oleobj, &clsid);
        if(SUCCEEDED(hres))
            TRACE("Got clsid %s\n",
                  IsEqualGUID(&clsid, &CLSID_HTMLDocument) ? "CLSID_HTMLDocument" : debugstr_guid(&clsid));

        hres = IOleObject_SetClientSite(oleobj, CLIENTSITE(This->doc_host));
        if(FAILED(hres))
            FIXME("SetClientSite failed: %08x\n", hres);

        IOleObject_Release(oleobj);
    }else {
        FIXME("Could not get IOleObject iface: %08x\n", hres);
    }

    /* FIXME: Call SetAdvise */
    /* FIXME: Call Invoke(DISPID_READYSTATE) */

    task = heap_alloc(sizeof(*task));
    push_dochost_task(This->doc_host, task, object_available_proc, FALSE);

    return S_OK;
}

#undef BSC_THIS

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

#define HTTPNEG_THIS(iface) DEFINE_THIS(BindStatusCallback, HttpNegotiate, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate *iface,
                                                   REFIID riid, void **ppv)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_QueryInterface(BINDSC(This), riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_AddRef(BINDSC(This));
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate *iface)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_Release(BINDSC(This));
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);

    FIXME("(%p)->(%s %s %d %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
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
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return E_NOTIMPL;
}

#undef HTTPNEG_THIS

static const IHttpNegotiateVtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse
};

static BindStatusCallback *create_callback(DocHost *doc_host, LPCWSTR url, PBYTE post_data,
        ULONG post_data_len, LPCWSTR headers)
{
    BindStatusCallback *ret = heap_alloc(sizeof(BindStatusCallback));

    ret->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    ret->lpHttpNegotiateVtbl      = &HttpNegotiateVtbl;

    ret->ref = 1;
    ret->url = heap_strdupW(url);
    ret->post_data = NULL;
    ret->post_data_len = post_data_len;
    ret->headers = headers ? SysAllocString(headers) : NULL;

    ret->doc_host = doc_host;
    IOleClientSite_AddRef(CLIENTSITE(doc_host));

    if(post_data) {
        ret->post_data = GlobalAlloc(0, post_data_len);
        memcpy(ret->post_data, post_data, post_data_len);
    }

    return ret;
}

static void on_before_navigate2(DocHost *This, LPCWSTR url, const BYTE *post_data,
                                ULONG post_data_len, LPWSTR headers, VARIANT_BOOL *cancel)
{
    VARIANT var_url, var_flags, var_frame_name, var_post_data, var_post_data2, var_headers;
    DISPPARAMS dispparams;
    VARIANTARG params[7];

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
    VariantInit(&var_post_data);

    if(post_data_len) {
        SAFEARRAYBOUND bound = {post_data_len, 0};
        void *data;

        V_VT(&var_post_data) = VT_UI1|VT_ARRAY;
        V_ARRAY(&var_post_data) = SafeArrayCreate(VT_UI1, 1, &bound);

        SafeArrayAccessData(V_ARRAY(&var_post_data), &data);
        memcpy(data, post_data, post_data_len);
        SafeArrayUnaccessData(V_ARRAY(&var_post_data));
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
    V_BSTR(&var_url) = SysAllocString(url);

    V_VT(params+6) = (VT_DISPATCH);
    V_DISPATCH(params+6) = This->disp;

    call_sink(This->cps.wbe2, DISPID_BEFORENAVIGATE2, &dispparams);

    SysFreeString(V_BSTR(&var_url));
    if(post_data_len)
        SafeArrayDestroy(V_ARRAY(&var_post_data));
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

    TRACE("openning application %s\n", debugstr_w(app));
 
    memset(&exec_info, 0, sizeof(exec_info));
    exec_info.cbSize = sizeof(exec_info);
    exec_info.lpFile = url;
    exec_info.nShow = SW_SHOW;

    return ShellExecuteExW(&exec_info);
}

static HRESULT http_load_hack(DocHost *This, IMoniker *mon, IBindStatusCallback *callback, IBindCtx *bindctx)
{
    IPersistMoniker *persist;
    IUnknown *doc;
    HRESULT hres;

    /*
     * FIXME:
     * We should use URLMoniker's BindToObject instead creating HTMLDocument here.
     * This should be fixed when mshtml.dll and urlmon.dll will be good enough.
     */

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
                            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &IID_IUnknown, (void**)&doc);

    if(FAILED(hres)) {
        ERR("Could not create HTMLDocument: %08x\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(doc, &IID_IPersistMoniker, (void**)&persist);
    if(FAILED(hres)) {
        IUnknown_Release(doc);
        return hres;
    }

    hres = IPersistMoniker_Load(persist, FALSE, mon, bindctx, 0);
    IPersistMoniker_Release(persist);

    if(SUCCEEDED(hres))
        hres = IBindStatusCallback_OnObjectAvailable(callback, &IID_IUnknown, doc);
    else
        WARN("Load failed: %08x\n", hres);

    IUnknown_Release(doc);

    return IBindStatusCallback_OnStopBinding(callback, hres, NULL);
}

static HRESULT create_moniker(LPCWSTR url, IMoniker **mon)
{
    WCHAR new_url[INTERNET_MAX_URL_LENGTH];
    DWORD size;
    HRESULT hres;

    if(PathIsURLW(url))
        return CreateURLMoniker(NULL, url, mon);

    if(url[1] == ':') {
        size = sizeof(new_url);
        hres = UrlCreateFromPathW(url, new_url, &size, 0);
        if(FAILED(hres)) {
            WARN("UrlCreateFromPathW failed: %08x\n", hres);
            return hres;
        }
    }else {
        size = sizeof(new_url);
        hres = UrlApplySchemeW(url, new_url, &size, URL_APPLY_GUESSSCHEME);
        TRACE("got %s\n", debugstr_w(new_url));
        if(FAILED(hres)) {
            WARN("UrlApplyScheme failed: %08x\n", hres);
            return hres;
        }
    }

    return CreateURLMoniker(NULL, new_url, mon);
}

static HRESULT bind_to_object(DocHost *This, IMoniker *mon, LPCWSTR url, IBindCtx *bindctx,
                              IBindStatusCallback *callback)
{
    WCHAR schema[30];
    DWORD schema_len;
    HRESULT hres;

    static const WCHAR httpW[] = {'h','t','t','p',0};
    static const WCHAR httpsW[] = {'h','t','t','p','s',0};
    static const WCHAR ftpW[]= {'f','t','p',0};

    if(mon) {
        IMoniker_AddRef(mon);
    }else {
        hres = create_moniker(url, &mon);
        if(FAILED(hres))
            return hres;
    }

    CoTaskMemFree(This->url);
    hres = IMoniker_GetDisplayName(mon, 0, NULL, &This->url);
    if(FAILED(hres))
        FIXME("GetDisplayName failed: %08x\n", hres);

    IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM,
                                 (IUnknown*)CLIENTSITE(This));

    hres = CoInternetParseUrl(This->url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(SUCCEEDED(hres) &&
       (!strcmpW(schema, httpW) || !strcmpW(schema, httpsW) || !strcmpW(schema, ftpW))) {
        hres = http_load_hack(This, mon, callback, bindctx);
    }else {
        IUnknown *unk = NULL;

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
    }

    IMoniker_Release(mon);
    return S_OK;
}

static HRESULT navigate_bsc(DocHost *This, BindStatusCallback *bsc, IMoniker *mon)
{
    IBindCtx *bindctx;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    HRESULT hres;

    on_before_navigate2(This, bsc->url, bsc->post_data, bsc->post_data_len, bsc->headers, &cancel);
    if(cancel) {
        FIXME("Navigation canceled\n");
        return S_OK;
    }

    if(This->document)
        deactivate_document(This);

    CreateAsyncBindCtx(0, BINDSC(bsc), 0, &bindctx);

    if(This->frame)
        IOleInPlaceFrame_EnableModeless(This->frame, FALSE);

    hres = bind_to_object(This, mon, bsc->url, bindctx, BINDSC(bsc));

    if(This->frame)
        IOleInPlaceFrame_EnableModeless(This->frame, TRUE);

    IBindCtx_Release(bindctx);

    return hres;
}

typedef struct {
    task_header_t header;
    BindStatusCallback *bsc;
} task_navigate_bsc_t;

static void navigate_bsc_proc(DocHost *This, task_header_t *t)
{
    task_navigate_bsc_t *task = (task_navigate_bsc_t*)t;

    if(!This->hwnd)
        create_doc_view_hwnd(This);

    navigate_bsc(This, task->bsc, NULL);

    IBindStatusCallback_Release(BINDSC(task->bsc));
}


HRESULT navigate_url(DocHost *This, LPCWSTR url, const VARIANT *Flags,
                     const VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    task_navigate_bsc_t *task;
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL;

    TRACE("navigating to %s\n", debugstr_w(url));

    if((Flags && V_VT(Flags) != VT_EMPTY) 
       || (TargetFrameName && V_VT(TargetFrameName) != VT_EMPTY))
        FIXME("Unsupported args (Flags %p:%d; TargetFrameName %p:%d)\n",
                Flags, Flags ? V_VT(Flags) : -1, TargetFrameName,
                TargetFrameName ? V_VT(TargetFrameName) : -1);

    if(PostData) {
        TRACE("PostData vt=%d\n", V_VT(PostData));

        if(V_VT(PostData) == (VT_ARRAY | VT_UI1)) {
            SafeArrayAccessData(V_ARRAY(PostData), (void**)&post_data);
            post_data_len = V_ARRAY(PostData)->rgsabound[0].cElements;
        }
    }

    if(Headers && V_VT(Headers) != VT_EMPTY && V_VT(Headers) != VT_ERROR) {
        if(V_VT(Headers) != VT_BSTR)
            return E_INVALIDARG;

        headers = V_BSTR(Headers);
        TRACE("Headers: %s\n", debugstr_w(headers));
    }

    task = heap_alloc(sizeof(*task));
    task->bsc = create_callback(This, url, post_data, post_data_len, headers);

    if(post_data)
        SafeArrayUnaccessData(V_ARRAY(PostData));

    push_dochost_task(This, &task->header, navigate_bsc_proc, This->url == NULL);

    return S_OK;
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

    hres = IMoniker_GetDisplayName(mon, 0, NULL, &url);
    if(FAILED(hres))
        FIXME("GetDisplayName failed: %08x\n", hres);

    bsc = create_callback(This, url, post_data, post_data_len, headers);
    CoTaskMemFree(url);

    hres = navigate_bsc(This, bsc, mon);

    IBindStatusCallback_Release(BINDSC(bsc));
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

#define HLINKFRAME_THIS(iface) DEFINE_THIS(WebBrowser, HlinkFrame, iface)

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_QueryInterface(WEBBROWSER2(This), riid, ppv);
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER2(This));
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER2(This));
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
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

    return navigate_hlink(&This->doc_host, mon, pbc, pibsc);
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%08x %p %s %s %d)\n", This, grfHLNF, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), dwreserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%u %p %s %s)\n", This, uHLID, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName));
    return E_NOTIMPL;
}

#undef HLINKFRAME_THIS

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

void WebBrowser_HlinkFrame_Init(WebBrowser *This)
{
    This->lpHlinkFrameVtbl = &HlinkFrameVtbl;
}
