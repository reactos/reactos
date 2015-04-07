/*
 * Copyright 2005 Jacek Caban
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

#include "mshtml_private.h"

/********************************************************************
 * common ProtocolFactory implementation
 */

typedef struct {
    IInternetProtocolInfo IInternetProtocolInfo_iface;
    IClassFactory         IClassFactory_iface;
} ProtocolFactory;

static inline ProtocolFactory *impl_from_IInternetProtocolInfo(IInternetProtocolInfo *iface)
{
    return CONTAINING_RECORD(iface, ProtocolFactory, IInternetProtocolInfo_iface);
}

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface, REFIID riid, void **ppv)
{
    ProtocolFactory *This = impl_from_IInternetProtocolInfo(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolInfo_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolInfo %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolInfo_iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", This, ppv);
        *ppv = &This->IClassFactory_iface;
    }

    if(!*ppv) {
        WARN("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocolInfo_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI InternetProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult,
        DWORD cchResult, DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %s %08x %p %d %p %d)\n", iface, debugstr_w(pwzBaseUrl),
            debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult,
            pcchResult, dwReserved);

    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    TRACE("%p)->(%s %s %08x)\n", iface, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return E_NOTIMPL;
}

static inline ProtocolFactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ProtocolFactory, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    ProtocolFactory *This = impl_from_IClassFactory(iface);
    return IInternetProtocolInfo_QueryInterface(&This->IInternetProtocolInfo_iface, riid, ppv);
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ProtocolFactory *This = impl_from_IClassFactory(iface);
    return IInternetProtocolInfo_AddRef(&This->IInternetProtocolInfo_iface);
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ProtocolFactory *This = impl_from_IClassFactory(iface);
    return IInternetProtocolInfo_Release(&This->IInternetProtocolInfo_iface);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("(%p)->(%x)\n", iface, dolock);
    return S_OK;
}

/********************************************************************
 * AboutProtocol implementation
 */

typedef struct {
    IInternetProtocol IInternetProtocol_iface;

    LONG ref;

    BYTE *data;
    ULONG data_len;
    ULONG cur;

    IUnknown *pUnkOuter;
} AboutProtocol;

static inline AboutProtocol *AboutProtocol_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, AboutProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI AboutProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        if(This->pUnkOuter)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        FIXME("IServiceProvider is not implemented\n");
        return E_NOINTERFACE;
    }

    if(!*ppv) {
        TRACE("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocol_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI AboutProtocol_AddRef(IInternetProtocol *iface)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", iface, ref);
    return This->pUnkOuter ? IUnknown_AddRef(This->pUnkOuter) : ref;
}

static ULONG WINAPI AboutProtocol_Release(IInternetProtocol *iface)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    IUnknown *pUnkOuter = This->pUnkOuter;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%x\n", iface, ref);

    if(!ref) {
        heap_free(This->data);
        heap_free(This);
    }

    return pUnkOuter ? IUnknown_Release(pUnkOuter) : ref;
}

static HRESULT WINAPI AboutProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    BINDINFO bindinfo;
    DWORD grfBINDF = 0;
    LPCWSTR text = NULL;
    DWORD data_len;
    BYTE *data;
    HRESULT hres;

    static const WCHAR html_begin[] = {0xfeff,'<','H','T','M','L','>',0};
    static const WCHAR html_end[] = {'<','/','H','T','M','L','>',0};
    static const WCHAR wszBlank[] = {'b','l','a','n','k',0};
    static const WCHAR wszAbout[] = {'a','b','o','u','t',':'};
    static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

    /* NOTE:
     * the about protocol seems not to work as I would expect. It creates html document
     * for a given url, eg. about:some_text -> <HTML>some_text</HTML> except for the case when
     * some_text = "blank", when document is blank (<HTML></HMTL>). The same happens
     * when the url does not have "about:" in the beginning.
     */

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres))
        return hres;
    ReleaseBindInfo(&bindinfo);

    TRACE("bindf %x\n", grfBINDF);

    if(strlenW(szUrl)>=sizeof(wszAbout)/sizeof(WCHAR) && !memcmp(wszAbout, szUrl, sizeof(wszAbout))) {
        text = szUrl + sizeof(wszAbout)/sizeof(WCHAR);
        if(!strcmpW(wszBlank, text))
            text = NULL;
    }

    data_len = sizeof(html_begin)+sizeof(html_end)-sizeof(WCHAR)
        + (text ? strlenW(text)*sizeof(WCHAR) : 0);
    data = heap_alloc(data_len);
    if(!data)
        return E_OUTOFMEMORY;

    heap_free(This->data);
    This->data = data;
    This->data_len = data_len;

    memcpy(This->data, html_begin, sizeof(html_begin));
    if(text)
        strcatW((LPWSTR)This->data, text);
    strcatW((LPWSTR)This->data, html_end);
    
    This->cur = 0;

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
            This->data_len, This->data_len);

    IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);

    return S_OK;
}

static HRESULT WINAPI AboutProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA* pProtocolData)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    TRACE("(%p)->(%08x)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI AboutProtocol_Suspend(IInternetProtocol *iface)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Resume(IInternetProtocol *iface)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->data)
        return E_FAIL;

    *pcbRead = (cb > This->data_len-This->cur ? This->data_len-This->cur : cb);

    if(!*pcbRead)
        return S_FALSE;

    memcpy(pv, This->data+This->cur, *pcbRead);
    This->cur += *pcbRead;

    return S_OK;
}

static HRESULT WINAPI AboutProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)->(%d)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI AboutProtocol_UnlockRequest(IInternetProtocol *iface)
{
    AboutProtocol *This = AboutProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static const IInternetProtocolVtbl AboutProtocolVtbl = {
    AboutProtocol_QueryInterface,
    AboutProtocol_AddRef,
    AboutProtocol_Release,
    AboutProtocol_Start,
    AboutProtocol_Continue,
    AboutProtocol_Abort,
    AboutProtocol_Terminate,
    AboutProtocol_Suspend,
    AboutProtocol_Resume,
    AboutProtocol_Read,
    AboutProtocol_Seek,
    AboutProtocol_LockRequest,
    AboutProtocol_UnlockRequest
};

static HRESULT WINAPI AboutProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    AboutProtocol *ret;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p %s %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc(sizeof(AboutProtocol));
    ret->IInternetProtocol_iface.lpVtbl = &AboutProtocolVtbl;
    ret->ref = 0;

    ret->data = NULL;
    ret->data_len = 0;
    ret->cur = 0;
    ret->pUnkOuter = pUnkOuter;

    if(pUnkOuter) {
        ret->ref = 1;
        if(IsEqualGUID(&IID_IUnknown, riid))
            *ppv = &ret->IInternetProtocol_iface;
        else
            hres = E_INVALIDARG;
    }else {
        hres = IInternetProtocol_QueryInterface(&ret->IInternetProtocol_iface, riid, ppv);
    }

    if(FAILED(hres))
        heap_free(ret);

    return hres;
}

static HRESULT WINAPI AboutProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %d %08x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);

    if(ParseAction == PARSE_SECURITY_URL) {
        unsigned int len = strlenW(pwzUrl)+1;

        *pcchResult = len;
        if(len > cchResult)
            return S_FALSE;

        memcpy(pwzResult, pwzUrl, len*sizeof(WCHAR));
        return S_OK;
    }

    if(ParseAction == PARSE_DOMAIN) {
        if(!pcchResult)
            return E_POINTER;

        if(pwzUrl)
            *pcchResult = strlenW(pwzUrl)+1;
        else
            *pcchResult = 1;
        return E_FAIL;
    }

    return INET_E_DEFAULT_ACTION;
}

static HRESULT WINAPI AboutProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    TRACE("%p)->(%s %08x %08x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
          cbBuffer, pcbBuf, dwReserved);

    switch(QueryOption) {
    case QUERY_CAN_NAVIGATE:
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;

    case QUERY_USES_NETWORK:
        if(!pBuffer || cbBuffer < sizeof(DWORD))
            return E_FAIL;

        *(DWORD*)pBuffer = 0;
        if(pcbBuf)
            *pcbBuf = sizeof(DWORD);

        break;

    case QUERY_IS_CACHED:
        FIXME("Unsupported option QUERY_IS_CACHED\n");
        return E_NOTIMPL;
    case QUERY_IS_INSTALLEDENTRY:
        FIXME("Unsupported option QUERY_IS_INSTALLEDENTRY\n");
        return E_NOTIMPL;
    case QUERY_IS_CACHED_OR_MAPPED:
        FIXME("Unsupported option QUERY_IS_CACHED_OR_MAPPED\n");
        return E_NOTIMPL;
    case QUERY_IS_SECURE:
        FIXME("Unsupported option QUERY_IS_SECURE\n");
        return E_NOTIMPL;
    case QUERY_IS_SAFE:
        FIXME("Unsupported option QUERY_IS_SAFE\n");
        return E_NOTIMPL;
    case QUERY_USES_HISTORYFOLDER:
        FIXME("Unsupported option QUERY_USES_HISTORYFOLDER\n");
        return E_FAIL;
    case QUERY_IS_CACHED_AND_USABLE_OFFLINE:
        FIXME("Unsupported option QUERY_IS_CACHED_AND_USABLE_OFFLINE\n");
        return E_NOTIMPL;
    default:
        return E_FAIL;
    }

    return S_OK;
}

static const IInternetProtocolInfoVtbl AboutProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    AboutProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    AboutProtocolInfo_QueryInfo
};

static const IClassFactoryVtbl AboutProtocolFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    AboutProtocolFactory_CreateInstance,
    ClassFactory_LockServer
};

static ProtocolFactory AboutProtocolFactory = {
    { &AboutProtocolInfoVtbl },
    { &AboutProtocolFactoryVtbl }
};

/********************************************************************
 * ResProtocol implementation
 */

typedef struct {
    IInternetProtocol IInternetProtocol_iface;
    LONG ref;

    BYTE *data;
    ULONG data_len;
    ULONG cur;

    IUnknown *pUnkOuter;
} ResProtocol;

static inline ResProtocol *ResProtocol_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, ResProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI ResProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        if(This->pUnkOuter)
            return IUnknown_QueryInterface(This->pUnkOuter, &IID_IUnknown, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", iface, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        FIXME("IServiceProvider is not implemented\n");
        return E_NOINTERFACE;
    }

    if(!*ppv) {
        TRACE("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocol_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ResProtocol_AddRef(IInternetProtocol *iface)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", iface, ref);
    return This->pUnkOuter ? IUnknown_AddRef(This->pUnkOuter) : ref;
}

static ULONG WINAPI ResProtocol_Release(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    IUnknown *pUnkOuter = This->pUnkOuter;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%x\n", iface, ref);

    if(!ref) {
        heap_free(This->data);
        heap_free(This);
    }

    return pUnkOuter ? IUnknown_Release(pUnkOuter) : ref;
}

static HRESULT WINAPI ResProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    WCHAR *url_dll, *url_file, *url, *mime, *res_type = (LPWSTR)RT_HTML, *ptr;
    DWORD grfBINDF = 0, len;
    BINDINFO bindinfo;
    HMODULE hdll;
    HRSRC src;
    HRESULT hres;

    static const WCHAR wszRes[] = {'r','e','s',':','/','/'};

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres))
        return hres;
    ReleaseBindInfo(&bindinfo);

    len = strlenW(szUrl)+16;
    url = heap_alloc(len*sizeof(WCHAR));
    hres = CoInternetParseUrl(szUrl, PARSE_ENCODE, 0, url, len, &len, 0);
    if(FAILED(hres)) {
        WARN("CoInternetParseUrl failed: %08x\n", hres);
        heap_free(url);
        IInternetProtocolSink_ReportResult(pOIProtSink, hres, 0, NULL);
        return hres;
    }

    if(len < sizeof(wszRes)/sizeof(wszRes[0]) || memcmp(url, wszRes, sizeof(wszRes))) {
        WARN("Wrong protocol of url: %s\n", debugstr_w(url));
        IInternetProtocolSink_ReportResult(pOIProtSink, E_INVALIDARG, 0, NULL);
        heap_free(url);
        return E_INVALIDARG;
    }

    url_dll = url + sizeof(wszRes)/sizeof(wszRes[0]);
    if(!(res_type = strchrW(url_dll, '/'))) {
        WARN("wrong url: %s\n", debugstr_w(url));
        IInternetProtocolSink_ReportResult(pOIProtSink, MK_E_SYNTAX, 0, NULL);
        heap_free(url);
        return MK_E_SYNTAX;
    }

    *res_type++ = 0;
    if ((url_file = strchrW(res_type, '/'))) {
        *url_file++ = 0;
    }else {
        url_file = res_type;
        res_type = MAKEINTRESOURCEW(RT_HTML);
    }

    /* Ignore query and hash parts. */
    if((ptr = strchrW(url_file, '?')))
        *ptr = 0;
    if(*url_file && (ptr = strchrW(url_file+1, '#')))
        *ptr = 0;

    hdll = LoadLibraryExW(url_dll, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(!hdll) {
        WARN("Could not open dll: %s\n", debugstr_w(url_dll));
        IInternetProtocolSink_ReportResult(pOIProtSink, HRESULT_FROM_WIN32(GetLastError()), 0, NULL);
        heap_free(url);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("trying to find resource type %s, name %s\n", debugstr_w(res_type), debugstr_w(url_file));

    src = FindResourceW(hdll, url_file, res_type);
    if(!src) {
        LPWSTR endpoint = NULL;
        DWORD file_id = strtolW(url_file, &endpoint, 10);
        if(endpoint == url_file+strlenW(url_file))
            src = FindResourceW(hdll, MAKEINTRESOURCEW(file_id), res_type);

        if(!src) {
            WARN("Could not find resource\n");
            IInternetProtocolSink_ReportResult(pOIProtSink,
                    HRESULT_FROM_WIN32(GetLastError()), 0, NULL);
            heap_free(url);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if(This->data) {
        WARN("data already loaded\n");
        heap_free(This->data);
    }

    This->data_len = SizeofResource(hdll, src);
    This->data = heap_alloc(This->data_len);
    memcpy(This->data, LoadResource(hdll, src), This->data_len);
    This->cur = 0;

    FreeLibrary(hdll);

    hres = FindMimeFromData(NULL, url_file, This->data, This->data_len, NULL, 0, &mime, 0);
    heap_free(url);
    if(SUCCEEDED(hres)) {
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        CoTaskMemFree(mime);
    }

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
            This->data_len, This->data_len);

    IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    
    return S_OK;
}

static HRESULT WINAPI ResProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA* pProtocolData)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    /* test show that we don't have to do anything here */
    return S_OK;
}

static HRESULT WINAPI ResProtocol_Suspend(IInternetProtocol *iface)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Resume(IInternetProtocol *iface)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->data)
        return E_FAIL;

    *pcbRead = (cb > This->data_len-This->cur ? This->data_len-This->cur : cb);

    if(!*pcbRead)
        return S_FALSE;

    memcpy(pv, This->data+This->cur, *pcbRead);
    This->cur += *pcbRead;

    return S_OK;
}

static HRESULT WINAPI ResProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)->(%d)\n", This, dwOptions);

    /* test show that we don't have to do anything here */
    return S_OK;
}

static HRESULT WINAPI ResProtocol_UnlockRequest(IInternetProtocol *iface)
{
    ResProtocol *This = ResProtocol_from_IInternetProtocol(iface);

    TRACE("(%p)\n", This);

    /* test show that we don't have to do anything here */
    return S_OK;
}

static const IInternetProtocolVtbl ResProtocolVtbl = {
    ResProtocol_QueryInterface,
    ResProtocol_AddRef,
    ResProtocol_Release,
    ResProtocol_Start,
    ResProtocol_Continue,
    ResProtocol_Abort,
    ResProtocol_Terminate,
    ResProtocol_Suspend,
    ResProtocol_Resume,
    ResProtocol_Read,
    ResProtocol_Seek,
    ResProtocol_LockRequest,
    ResProtocol_UnlockRequest
};

static HRESULT WINAPI ResProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    ResProtocol *ret;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p %s %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc(sizeof(ResProtocol));
    ret->IInternetProtocol_iface.lpVtbl = &ResProtocolVtbl;
    ret->ref = 0;
    ret->data = NULL;
    ret->data_len = 0;
    ret->cur = 0;
    ret->pUnkOuter = pUnkOuter;

    if(pUnkOuter) {
        ret->ref = 1;
        if(IsEqualGUID(&IID_IUnknown, riid))
            *ppv = &ret->IInternetProtocol_iface;
        else
            hres = E_FAIL;
    }else {
        hres = IInternetProtocol_QueryInterface(&ret->IInternetProtocol_iface, riid, ppv);
    }

    if(FAILED(hres))
        heap_free(ret);

    return hres;
}

static HRESULT WINAPI ResProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %d %x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);

    if(ParseAction == PARSE_SECURITY_URL) {
        WCHAR file_part[MAX_PATH], full_path[MAX_PATH];
        WCHAR *ptr;
        DWORD size, len;

        static const WCHAR wszFile[] = {'f','i','l','e',':','/','/'};
        static const WCHAR wszRes[] = {'r','e','s',':','/','/'};

        if(strlenW(pwzUrl) <= sizeof(wszRes)/sizeof(WCHAR) || memcmp(pwzUrl, wszRes, sizeof(wszRes)))
            return E_INVALIDARG;

        ptr = strchrW(pwzUrl + sizeof(wszRes)/sizeof(WCHAR), '/');
        if(!ptr)
            return E_INVALIDARG;

        len = ptr - (pwzUrl + sizeof(wszRes)/sizeof(WCHAR));
        if(len >= sizeof(file_part)/sizeof(WCHAR)) {
            FIXME("Too long URL\n");
            return MK_E_SYNTAX;
        }

        memcpy(file_part, pwzUrl + sizeof(wszRes)/sizeof(WCHAR), len*sizeof(WCHAR));
        file_part[len] = 0;

        len = SearchPathW(NULL, file_part, NULL, sizeof(full_path)/sizeof(WCHAR), full_path, NULL);
        if(!len) {
            HMODULE module;

            /* SearchPath does not work well with winelib files (like our test executable),
             * so we also try to load the library here */
            module = LoadLibraryExW(file_part, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(!module) {
                WARN("Could not find file %s\n", debugstr_w(file_part));
                return MK_E_SYNTAX;
            }

            len = GetModuleFileNameW(module, full_path, sizeof(full_path)/sizeof(WCHAR));
            FreeLibrary(module);
            if(!len)
                return E_FAIL;
        }

        size = sizeof(wszFile)/sizeof(WCHAR) + len + 1;
        if(pcchResult)
            *pcchResult = size;
        if(size > cchResult)
            return S_FALSE;

        memcpy(pwzResult, wszFile, sizeof(wszFile));
        memcpy(pwzResult + sizeof(wszFile)/sizeof(WCHAR), full_path, (len+1)*sizeof(WCHAR));
        return S_OK;
    }

    if(ParseAction == PARSE_DOMAIN) {
        if(!pcchResult)
            return E_POINTER;

        if(pwzUrl)
            *pcchResult = strlenW(pwzUrl)+1;
        else
            *pcchResult = 1;
        return E_FAIL;
    }

    return INET_E_DEFAULT_ACTION;
}

static HRESULT WINAPI ResProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    TRACE("%p)->(%s %08x %08x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
          cbBuffer, pcbBuf, dwReserved);

    switch(QueryOption) {
    case QUERY_USES_NETWORK:
        if(!pBuffer || cbBuffer < sizeof(DWORD))
            return E_FAIL;

        *(DWORD*)pBuffer = 0;
        if(pcbBuf)
            *pcbBuf = sizeof(DWORD);
        break;

    case QUERY_IS_SECURE:
        FIXME("QUERY_IS_SECURE not supported\n");
        return E_NOTIMPL;
    case QUERY_IS_SAFE:
        FIXME("QUERY_IS_SAFE not supported\n");
        return E_NOTIMPL;
    default:
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    }

    return S_OK;
}

static const IInternetProtocolInfoVtbl ResProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    ResProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    ResProtocolInfo_QueryInfo
};

static const IClassFactoryVtbl ResProtocolFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ResProtocolFactory_CreateInstance,
    ClassFactory_LockServer
};

static ProtocolFactory ResProtocolFactory = {
    { &ResProtocolInfoVtbl },
    { &ResProtocolFactoryVtbl }
};

/********************************************************************
 * JSProtocol implementation
 */

static HRESULT WINAPI JSProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    FIXME("(%p)->(%p %s %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI JSProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %d %x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), ParseAction,
          dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);

    switch(ParseAction) {
    case PARSE_SECURITY_URL:
        FIXME("PARSE_SECURITY_URL\n");
        return E_NOTIMPL;
    case PARSE_DOMAIN:
        FIXME("PARSE_DOMAIN\n");
        return E_NOTIMPL;
    default:
        return INET_E_DEFAULT_ACTION;
    }

    return S_OK;
}

static HRESULT WINAPI JSProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    TRACE("%p)->(%s %08x %08x %p %d %p %d)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
          cbBuffer, pcbBuf, dwReserved);

    switch(QueryOption) {
    case QUERY_USES_NETWORK:
        if(!pBuffer || cbBuffer < sizeof(DWORD))
            return E_FAIL;

        *(DWORD*)pBuffer = 0;
        if(pcbBuf)
            *pcbBuf = sizeof(DWORD);
        break;

    case QUERY_IS_SECURE:
        FIXME("QUERY_IS_SECURE not supported\n");
        return E_NOTIMPL;

    default:
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    }

    return S_OK;
}

static const IInternetProtocolInfoVtbl JSProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    JSProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    JSProtocolInfo_QueryInfo
};

static const IClassFactoryVtbl JSProtocolFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    JSProtocolFactory_CreateInstance,
    ClassFactory_LockServer
};

static ProtocolFactory JSProtocolFactory = {
    { &JSProtocolInfoVtbl },
    { &JSProtocolFactoryVtbl }
};

HRESULT ProtocolFactory_Create(REFCLSID rclsid, REFIID riid, void **ppv)
{
    ProtocolFactory *cf = NULL;

    if(IsEqualGUID(&CLSID_AboutProtocol, rclsid))
        cf = &AboutProtocolFactory;
    else if(IsEqualGUID(&CLSID_ResProtocol, rclsid))
        cf = &ResProtocolFactory;
    else if(IsEqualGUID(&CLSID_JSProtocol, rclsid))
        cf = &JSProtocolFactory;

    if(!cf) {
        FIXME("not implemented protocol %s\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }
 
    return IInternetProtocolInfo_QueryInterface(&cf->IInternetProtocolInfo_iface, riid, ppv);
}
