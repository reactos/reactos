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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "wininet.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT get_url(HTMLLocation *This, const WCHAR **ret)
{
    if(!This->window || !This->window->url) {
        FIXME("No current URL\n");
        return E_NOTIMPL;
    }

    *ret = This->window->url;
    return S_OK;
}

static HRESULT get_url_components(HTMLLocation *This, URL_COMPONENTSW *url)
{
    const WCHAR *doc_url;
    HRESULT hres;

    hres = get_url(This, &doc_url);
    if(FAILED(hres))
        return hres;

    if(!InternetCrackUrlW(doc_url, 0, 0, url)) {
        FIXME("InternetCrackUrlW failed: 0x%08x\n", GetLastError());
        SetLastError(0);
        return E_FAIL;
    }

    return S_OK;
}

#define HTMLLOCATION_THIS(iface) DEFINE_THIS(HTMLLocation, HTMLLocation, iface)

static HRESULT WINAPI HTMLLocation_QueryInterface(IHTMLLocation *iface, REFIID riid, void **ppv)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLLOCATION(This);
    }else if(IsEqualGUID(&IID_IHTMLLocation, riid)) {
        TRACE("(%p)->(IID_IHTMLLocation %p)\n", This, ppv);
        *ppv = HTMLLOCATION(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLLocation_AddRef(IHTMLLocation *iface)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLLocation_Release(IHTMLLocation *iface)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->window)
            This->window->location = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLLocation_GetTypeInfoCount(IHTMLLocation *iface, UINT *pctinfo)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLLocation_GetTypeInfo(IHTMLLocation *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLLocation_GetIDsOfNames(IHTMLLocation *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLLocation_Invoke(IHTMLLocation *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLLocation_put_href(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->window) {
        FIXME("No window available\n");
        return E_FAIL;
    }

    return navigate_url(This->window, v, This->window->url);
}

static HRESULT WINAPI HTMLLocation_get_href(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    WCHAR *buf = NULL, *url_path = NULL;
    HRESULT hres, ret;
    DWORD len = 0;
    int i;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwSchemeLength = 1;
    url.dwHostNameLength = 1;
    url.dwUrlPathLength = 1;
    url.dwExtraInfoLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    switch(url.nScheme) {
    case INTERNET_SCHEME_FILE:
        {
            /* prepend a slash */
            url_path = HeapAlloc(GetProcessHeap(), 0, (url.dwUrlPathLength + 1) * sizeof(WCHAR));
            if(!url_path)
                return E_OUTOFMEMORY;
            url_path[0] = '/';
            memcpy(url_path + 1, url.lpszUrlPath, url.dwUrlPathLength * sizeof(WCHAR));
            url.lpszUrlPath = url_path;
            url.dwUrlPathLength = url.dwUrlPathLength + 1;
        }
        break;

    case INTERNET_SCHEME_HTTP:
    case INTERNET_SCHEME_HTTPS:
    case INTERNET_SCHEME_FTP:
        if(!url.dwUrlPathLength) {
            /* add a slash if it's blank */
            url_path = url.lpszUrlPath = HeapAlloc(GetProcessHeap(), 0, 1 * sizeof(WCHAR));
            if(!url.lpszUrlPath)
                return E_OUTOFMEMORY;
            url.lpszUrlPath[0] = '/';
            url.dwUrlPathLength = 1;
        }
        break;

    default:
        break;
    }

    /* replace \ with / */
    for(i = 0; i < url.dwUrlPathLength; ++i)
        if(url.lpszUrlPath[i] == '\\')
            url.lpszUrlPath[i] = '/';

    if(InternetCreateUrlW(&url, ICU_ESCAPE, NULL, &len)) {
        FIXME("InternetCreateUrl succeeded with NULL buffer?\n");
        ret = E_FAIL;
        goto cleanup;
    }

    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        FIXME("InternetCreateUrl failed with error: %08x\n", GetLastError());
        SetLastError(0);
        ret = E_FAIL;
        goto cleanup;
    }
    SetLastError(0);

    buf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if(!buf) {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }

    if(!InternetCreateUrlW(&url, ICU_ESCAPE, buf, &len)) {
        FIXME("InternetCreateUrl failed with error: %08x\n", GetLastError());
        SetLastError(0);
        ret = E_FAIL;
        goto cleanup;
    }

    *p = SysAllocStringLen(buf, len);
    if(!*p) {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }

    ret = S_OK;

cleanup:
    HeapFree(GetProcessHeap(), 0, buf);
    HeapFree(GetProcessHeap(), 0, url_path);

    return ret;
}

static HRESULT WINAPI HTMLLocation_put_protocol(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_protocol(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwSchemeLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(!url.dwSchemeLength) {
        FIXME("Unexpected blank protocol\n");
        return E_NOTIMPL;
    }else {
        WCHAR buf[url.dwSchemeLength + 1];
        memcpy(buf, url.lpszScheme, url.dwSchemeLength * sizeof(WCHAR));
        buf[url.dwSchemeLength] = ':';
        *p = SysAllocStringLen(buf, url.dwSchemeLength + 1);
    }
    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_host(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_host(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwHostNameLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(!url.dwHostNameLength){
        *p = NULL;
        return S_OK;
    }

    if(url.nPort) {
        /* <hostname>:<port> */
        const WCHAR format[] = {'%','d',0};
        DWORD len = url.dwHostNameLength + 1 + 5 + 1;
        WCHAR buf[len];

        memcpy(buf, url.lpszHostName, url.dwHostNameLength * sizeof(WCHAR));
        buf[url.dwHostNameLength] = ':';
        snprintfW(buf + url.dwHostNameLength + 1, 6, format, url.nPort);
        *p = SysAllocString(buf);
    }else
        *p = SysAllocStringLen(url.lpszHostName, url.dwHostNameLength);

    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_hostname(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_hostname(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwHostNameLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(!url.dwHostNameLength){
        *p = NULL;
        return S_OK;
    }

    *p = SysAllocStringLen(url.lpszHostName, url.dwHostNameLength);
    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_port(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_port(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(url.nPort) {
        const WCHAR format[] = {'%','d',0};
        WCHAR buf[6];
        snprintfW(buf, 6, format, url.nPort);
        *p = SysAllocString(buf);
    }else {
        const WCHAR empty[] = {0};
        *p = SysAllocString(empty);
    }

    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_pathname(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_pathname(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwUrlPathLength = 1;
    url.dwExtraInfoLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(url.dwUrlPathLength && url.lpszUrlPath[0] == '/')
        *p = SysAllocStringLen(url.lpszUrlPath + 1, url.dwUrlPathLength - 1);
    else
        *p = SysAllocStringLen(url.lpszUrlPath, url.dwUrlPathLength);

    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_search(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_search(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    HRESULT hres;
    const WCHAR hash[] = {'#',0};

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwExtraInfoLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(!url.dwExtraInfoLength){
        *p = NULL;
        return S_OK;
    }

    url.dwExtraInfoLength = strcspnW(url.lpszExtraInfo, hash);

    *p = SysAllocStringLen(url.lpszExtraInfo, url.dwExtraInfoLength);

    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_put_hash(IHTMLLocation *iface, BSTR v)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_hash(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    URL_COMPONENTSW url = {sizeof(URL_COMPONENTSW)};
    const WCHAR hash[] = {'#',0};
    DWORD hash_pos = 0;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    url.dwExtraInfoLength = 1;
    hres = get_url_components(This, &url);
    if(FAILED(hres))
        return hres;

    if(!url.dwExtraInfoLength){
        *p = NULL;
        return S_OK;
    }

    hash_pos = strcspnW(url.lpszExtraInfo, hash);
    url.dwExtraInfoLength -= hash_pos;

    *p = SysAllocStringLen(url.lpszExtraInfo + hash_pos, url.dwExtraInfoLength);

    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLLocation_reload(IHTMLLocation *iface, VARIANT_BOOL flag)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%x)\n", This, flag);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_replace(IHTMLLocation *iface, BSTR bstr)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(bstr));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_assign(IHTMLLocation *iface, BSTR bstr)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(bstr));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_toString(IHTMLLocation *iface, BSTR *String)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

#undef HTMLLOCATION_THIS

static const IHTMLLocationVtbl HTMLLocationVtbl = {
    HTMLLocation_QueryInterface,
    HTMLLocation_AddRef,
    HTMLLocation_Release,
    HTMLLocation_GetTypeInfoCount,
    HTMLLocation_GetTypeInfo,
    HTMLLocation_GetIDsOfNames,
    HTMLLocation_Invoke,
    HTMLLocation_put_href,
    HTMLLocation_get_href,
    HTMLLocation_put_protocol,
    HTMLLocation_get_protocol,
    HTMLLocation_put_host,
    HTMLLocation_get_host,
    HTMLLocation_put_hostname,
    HTMLLocation_get_hostname,
    HTMLLocation_put_port,
    HTMLLocation_get_port,
    HTMLLocation_put_pathname,
    HTMLLocation_get_pathname,
    HTMLLocation_put_search,
    HTMLLocation_get_search,
    HTMLLocation_put_hash,
    HTMLLocation_get_hash,
    HTMLLocation_reload,
    HTMLLocation_replace,
    HTMLLocation_assign,
    HTMLLocation_toString
};

static const tid_t HTMLLocation_iface_tids[] = {
    IHTMLLocation_tid,
    0
};
static dispex_static_data_t HTMLLocation_dispex = {
    NULL,
    DispHTMLLocation_tid,
    NULL,
    HTMLLocation_iface_tids
};


HRESULT HTMLLocation_Create(HTMLWindow *window, HTMLLocation **ret)
{
    HTMLLocation *location;

    location = heap_alloc(sizeof(*location));
    if(!location)
        return E_OUTOFMEMORY;

    location->lpHTMLLocationVtbl = &HTMLLocationVtbl;
    location->ref = 1;
    location->window = window;

    init_dispex(&location->dispex, (IUnknown*)HTMLLOCATION(location),  &HTMLLocation_dispex);

    *ret = location;
    return S_OK;
}
