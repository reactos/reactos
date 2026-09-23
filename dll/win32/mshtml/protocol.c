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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    IUnknown          IUnknown_inner;
    IInternetProtocol IInternetProtocol_iface;

    LONG ref;

    BYTE *data;
    ULONG data_len;
    ULONG cur;

    IUnknown *outer;
} InternetProtocol;

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
    TRACE("%p)->(%s %s %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzBaseUrl),
            debugstr_w(pwzRelativeUrl), dwCombineFlags, pwzResult, cchResult,
            pcchResult, dwReserved);

    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    TRACE("%p)->(%s %s %08lx)\n", iface, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
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

static inline InternetProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, InternetProtocol, IUnknown_inner);
}

static inline InternetProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, InternetProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI Protocol_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    InternetProtocol *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = &This->IInternetProtocol_iface;
    }else {
        if(IsEqualGUID(&IID_IServiceProvider, riid))
            FIXME("IServiceProvider is not implemented\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Protocol_AddRef(IUnknown *iface)
{
    InternetProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", iface, ref);
    return ref;
}

static ULONG WINAPI Protocol_Release(IUnknown *iface)
{
    InternetProtocol *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lx\n", iface, ref);

    if(!ref) {
        free(This->data);
        free(This);
    }

    return ref;
}

static const IUnknownVtbl ProtocolUnkVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release
};

static HRESULT WINAPI InternetProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI InternetProtocol_AddRef(IInternetProtocol *iface)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI InternetProtocol_Release(IInternetProtocol *iface)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI InternetProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA* pProtocolData)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    TRACE("(%p)->(%08lx)\n", This, dwOptions);
    return S_OK;
}

static HRESULT WINAPI InternetProtocol_Suspend(IInternetProtocol *iface)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocol_Resume(IInternetProtocol *iface)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if(!This->data)
        return E_FAIL;

    *pcbRead = (cb > This->data_len-This->cur ? This->data_len-This->cur : cb);

    if(!*pcbRead)
        return S_FALSE;

    memcpy(pv, This->data+This->cur, *pcbRead);
    This->cur += *pcbRead;

    return S_OK;
}

static HRESULT WINAPI InternetProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%ld)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI InternetProtocol_UnlockRequest(IInternetProtocol *iface)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT create_protocol_instance(const IInternetProtocolVtbl *protocol_vtbl,
                                        IUnknown *outer, REFIID riid, void **ppv)
{
    InternetProtocol *protocol;
    HRESULT hres;

    if(outer && !IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = NULL;
        return E_INVALIDARG;
    }

    protocol = calloc(1, sizeof(InternetProtocol));
    protocol->IUnknown_inner.lpVtbl = &ProtocolUnkVtbl;
    protocol->IInternetProtocol_iface.lpVtbl = protocol_vtbl;
    protocol->outer = outer ? outer : &protocol->IUnknown_inner;
    protocol->ref = 1;

    hres = IUnknown_QueryInterface(&protocol->IUnknown_inner, riid, ppv);
    IUnknown_Release(&protocol->IUnknown_inner);
    return hres;
}

/********************************************************************
 * about protocol implementation
 */

static HRESULT WINAPI AboutProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    BINDINFO bindinfo;
    DWORD grfBINDF = 0;
    LPCWSTR text = NULL;
    DWORD data_len;
    BYTE *data;
    HRESULT hres;
    static const WCHAR wszAbout[] = {'a','b','o','u','t',':'};

    /* NOTE:
     * the about protocol seems not to work as I would expect. It creates html document
     * for a given url, eg. about:some_text -> <HTML>some_text</HTML> except for the case when
     * some_text = "blank", when document is blank (<HTML></HMTL>). The same happens
     * when the url does not have "about:" in the beginning.
     */

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres))
        return hres;
    ReleaseBindInfo(&bindinfo);

    TRACE("bindf %lx\n", grfBINDF);

    if(lstrlenW(szUrl) >= ARRAY_SIZE(wszAbout) && !memcmp(wszAbout, szUrl, sizeof(wszAbout))) {
        text = szUrl + ARRAY_SIZE(wszAbout);
        if(!wcscmp(L"blank", text))
            text = NULL;
    }

    data_len = sizeof(L"\xfeff<HTML>")+sizeof(L"</HTML>")-sizeof(WCHAR)
        + (text ? lstrlenW(text)*sizeof(WCHAR) : 0);
    data = malloc(data_len);
    if(!data)
        return E_OUTOFMEMORY;

    free(This->data);
    This->data = data;
    This->data_len = data_len;

    lstrcpyW((LPWSTR)This->data, L"\xfeff<HTML>");
    if(text)
        lstrcatW((LPWSTR)This->data, text);
    lstrcatW((LPWSTR)This->data, L"</HTML>");

    This->cur = 0;

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
            This->data_len, This->data_len);

    IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);

    return S_OK;
}

static const IInternetProtocolVtbl AboutProtocolVtbl = {
    InternetProtocol_QueryInterface,
    InternetProtocol_AddRef,
    InternetProtocol_Release,
    AboutProtocol_Start,
    InternetProtocol_Continue,
    InternetProtocol_Abort,
    InternetProtocol_Terminate,
    InternetProtocol_Suspend,
    InternetProtocol_Resume,
    InternetProtocol_Read,
    InternetProtocol_Seek,
    InternetProtocol_LockRequest,
    InternetProtocol_UnlockRequest
};

static HRESULT WINAPI AboutProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%p %s %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppv);

    return create_protocol_instance(&AboutProtocolVtbl, pUnkOuter, riid, ppv);
}

static HRESULT WINAPI AboutProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %d %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);

    if(ParseAction == PARSE_SECURITY_URL) {
        unsigned int len = lstrlenW(pwzUrl)+1;

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
            *pcchResult = lstrlenW(pwzUrl)+1;
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
    TRACE("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
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
 * res protocol implementation
 */

static HRESULT WINAPI ResProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    InternetProtocol *This = impl_from_IInternetProtocol(iface);
    WCHAR *url_dll, *url_file, *url, *mime, *res_type, *alt_res_type = NULL, *ptr;
    DWORD grfBINDF = 0, len;
    BINDINFO bindinfo;
    HMODULE hdll;
    HRSRC src;
    HRESULT hres;

    static const WCHAR wszRes[] = {'r','e','s',':','/','/'};

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres))
        return hres;
    ReleaseBindInfo(&bindinfo);

    len = lstrlenW(szUrl)+16;
    url = malloc(len * sizeof(WCHAR));
    hres = CoInternetParseUrl(szUrl, PARSE_ENCODE, 0, url, len, &len, 0);
    if(FAILED(hres)) {
        WARN("CoInternetParseUrl failed: %08lx\n", hres);
        free(url);
        IInternetProtocolSink_ReportResult(pOIProtSink, hres, 0, NULL);
        return hres;
    }

    if(len < ARRAY_SIZE(wszRes) || memcmp(url, wszRes, sizeof(wszRes))) {
        WARN("Wrong protocol of url: %s\n", debugstr_w(url));
        IInternetProtocolSink_ReportResult(pOIProtSink, E_INVALIDARG, 0, NULL);
        free(url);
        return E_INVALIDARG;
    }

    url_dll = url + ARRAY_SIZE(wszRes);
    if(!(res_type = wcschr(url_dll, '/'))) {
        WARN("wrong url: %s\n", debugstr_w(url));
        IInternetProtocolSink_ReportResult(pOIProtSink, MK_E_SYNTAX, 0, NULL);
        free(url);
        return MK_E_SYNTAX;
    }

    *res_type++ = 0;
    if ((url_file = wcschr(res_type, '/'))) {
        DWORD res_type_id;
        WCHAR *endpoint;
        *url_file++ = 0;
        res_type_id = wcstol(res_type, &endpoint, 10);
        if(!*endpoint)
            res_type = MAKEINTRESOURCEW(res_type_id);
    }else {
        url_file = res_type;
        res_type = MAKEINTRESOURCEW(RT_HTML);
        alt_res_type = MAKEINTRESOURCEW(2110 /* RT_FILE */);
    }

    /* Ignore query and hash parts. */
    if((ptr = wcschr(url_file, '?')))
        *ptr = 0;
    if(*url_file && (ptr = wcschr(url_file+1, '#')))
        *ptr = 0;

    hdll = LoadLibraryExW(url_dll, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(!hdll) {
        WARN("Could not open dll: %s\n", debugstr_w(url_dll));
        IInternetProtocolSink_ReportResult(pOIProtSink, HRESULT_FROM_WIN32(GetLastError()), 0, NULL);
        free(url);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("trying to find resource type %s, name %s\n", debugstr_w(res_type), debugstr_w(url_file));

    src = FindResourceW(hdll, url_file, res_type);
    if(!src && alt_res_type)
        src = FindResourceW(hdll, url_file, alt_res_type);
    if(!src) {
        LPWSTR endpoint = NULL;
        DWORD file_id = wcstol(url_file, &endpoint, 10);
        if(!*endpoint) {
            src = FindResourceW(hdll, MAKEINTRESOURCEW(file_id), res_type);
            if(!src && alt_res_type)
                src = FindResourceW(hdll, MAKEINTRESOURCEW(file_id), alt_res_type);
        }
        if(!src) {
            WARN("Could not find resource\n");
            IInternetProtocolSink_ReportResult(pOIProtSink,
                    HRESULT_FROM_WIN32(GetLastError()), 0, NULL);
            free(url);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if(This->data) {
        WARN("data already loaded\n");
        free(This->data);
    }

    This->data_len = SizeofResource(hdll, src);
    This->data = malloc(This->data_len);
    memcpy(This->data, LoadResource(hdll, src), This->data_len);
    This->cur = 0;

    FreeLibrary(hdll);

    hres = FindMimeFromData(NULL, url_file, This->data, This->data_len, NULL, 0, &mime, 0);
    free(url);
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

static const IInternetProtocolVtbl ResProtocolVtbl = {
    InternetProtocol_QueryInterface,
    InternetProtocol_AddRef,
    InternetProtocol_Release,
    ResProtocol_Start,
    InternetProtocol_Continue,
    InternetProtocol_Abort,
    InternetProtocol_Terminate,
    InternetProtocol_Suspend,
    InternetProtocol_Resume,
    InternetProtocol_Read,
    InternetProtocol_Seek,
    InternetProtocol_LockRequest,
    InternetProtocol_UnlockRequest
};

static HRESULT WINAPI ResProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%p %s %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppv);

    return create_protocol_instance(&ResProtocolVtbl, pUnkOuter, riid, ppv);
}

static HRESULT WINAPI ResProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    TRACE("%p)->(%s %d %lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);

    if(ParseAction == PARSE_SECURITY_URL) {
        WCHAR file_part[MAX_PATH], full_path[MAX_PATH];
        WCHAR *ptr;
        DWORD size, len;

        static const WCHAR wszFile[] = {'f','i','l','e',':','/','/'};
        static const WCHAR wszRes[] = {'r','e','s',':','/','/'};

        if(lstrlenW(pwzUrl) <= ARRAY_SIZE(wszRes) || memcmp(pwzUrl, wszRes, sizeof(wszRes)))
            return E_INVALIDARG;

        ptr = wcschr(pwzUrl + ARRAY_SIZE(wszRes), '/');
        if(!ptr)
            return E_INVALIDARG;

        len = ptr - (pwzUrl + ARRAY_SIZE(wszRes));
        if(len >= ARRAY_SIZE(file_part)) {
            FIXME("Too long URL\n");
            return MK_E_SYNTAX;
        }

        memcpy(file_part, pwzUrl + ARRAY_SIZE(wszRes), len*sizeof(WCHAR));
        file_part[len] = 0;

        len = SearchPathW(NULL, file_part, NULL, ARRAY_SIZE(full_path), full_path, NULL);
        if(!len) {
            HMODULE module;

            /* SearchPath does not work well with winelib files (like our test executable),
             * so we also try to load the library here */
            module = LoadLibraryExW(file_part, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(!module) {
                WARN("Could not find file %s\n", debugstr_w(file_part));
                return MK_E_SYNTAX;
            }

            len = GetModuleFileNameW(module, full_path, ARRAY_SIZE(full_path));
            FreeLibrary(module);
            if(!len)
                return E_FAIL;
        }

        size = ARRAY_SIZE(wszFile) + len + 1;
        if(pcchResult)
            *pcchResult = size;
        if(size > cchResult)
            return S_FALSE;

        memcpy(pwzResult, wszFile, sizeof(wszFile));
        memcpy(pwzResult + ARRAY_SIZE(wszFile), full_path, (len+1)*sizeof(WCHAR));
        return S_OK;
    }

    if(ParseAction == PARSE_DOMAIN) {
        if(!pcchResult)
            return E_POINTER;

        if(pwzUrl)
            *pcchResult = lstrlenW(pwzUrl)+1;
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
    TRACE("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
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
    TRACE("%p)->(%s %d %lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), ParseAction,
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
    TRACE("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
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
