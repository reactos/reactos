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
    if(!This->window || !This->window->doc_obj || !This->window->doc_obj->url) {
        FIXME("No current URL\n");
        return E_NOTIMPL;
    }

    *ret = This->window->doc_obj->url;
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
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLocation_get_href(IHTMLLocation *iface, BSTR *p)
{
    HTMLLocation *This = HTMLLOCATION_THIS(iface);
    const WCHAR *url;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    hres = get_url(This, &url);
    if(FAILED(hres))
        return hres;

    *p = SysAllocString(url);
    return *p ? S_OK : E_OUTOFMEMORY;
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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
    WCHAR buf[INTERNET_MAX_PATH_LENGTH];
    URL_COMPONENTSW url = {sizeof(url)};
    const WCHAR *doc_url;
    DWORD size = 0;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    hres = get_url(This, &doc_url);
    if(FAILED(hres))
        return hres;

    hres = CoInternetParseUrl(doc_url, PARSE_PATH_FROM_URL, 0, buf, sizeof(buf), &size, 0);
    if(SUCCEEDED(hres)) {
        *p = SysAllocString(buf);
        if(!*p)
            return E_OUTOFMEMORY;
        return S_OK;
    }

    url.dwUrlPathLength = 1;
    if(!InternetCrackUrlW(doc_url, 0, 0, &url)) {
        FIXME("InternetCrackUrl failed\n");
        return E_FAIL;
    }

    if(!url.dwUrlPathLength) {
        *p = NULL;
        return S_OK;
    }

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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return E_NOTIMPL;
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
