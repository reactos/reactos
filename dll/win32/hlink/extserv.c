/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "hlink_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlink);

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

typedef struct {
    const IUnknownVtbl        *lpIUnknownVtbl;
    const IAuthenticateVtbl   *lpIAuthenticateVtbl;
    const IHttpNegotiateVtbl  *lpIHttpNegotiateVtbl;

    LONG ref;
    IUnknown *outer;

    HWND hwnd;
    LPWSTR username;
    LPWSTR password;
    LPWSTR headers;
} ExtensionService;

#define EXTSERVUNK(x)    ((IUnknown*)       &(x)->lpIUnknownVtbl)
#define AUTHENTICATE(x)  ((IAuthenticate*)  &(x)->lpIAuthenticateVtbl)
#define HTTPNEGOTIATE(x) ((IHttpNegotiate*) &(x)->lpIHttpNegotiateVtbl)

#define EXTSERVUNK_THIS(iface)  DEFINE_THIS(ExtensionService, IUnknown, iface)

static HRESULT WINAPI ExtServUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = EXTSERVUNK(This);
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        TRACE("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
        *ppv = AUTHENTICATE(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEGOTIATE(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ExtServUnk_AddRef(IUnknown *iface)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ExtServUnk_Release(IUnknown *iface)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This->username);
        heap_free(This->password);
        heap_free(This->headers);
        heap_free(This);
    }

    return ref;
}

#undef EXTSERVUNK_THIS

static const IUnknownVtbl ExtServUnkVtbl = {
    ExtServUnk_QueryInterface,
    ExtServUnk_AddRef,
    ExtServUnk_Release
};

#define AUTHENTICATE_THIS(iface) DEFINE_THIS(ExtensionService, IAuthenticate, iface)

static HRESULT WINAPI Authenticate_QueryInterface(IAuthenticate *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI Authenticate_AddRef(IAuthenticate *iface)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI Authenticate_Release(IAuthenticate *iface)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI Authenticate_Authenticate(IAuthenticate *iface,
        HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);

    TRACE("(%p)->(%p %p %p)\n", This, phwnd, pszUsername, pszPassword);

    if(!phwnd || !pszUsername || !pszPassword)
        return E_INVALIDARG;

    *phwnd = This->hwnd;
    *pszUsername = hlink_co_strdupW(This->username);
    *pszPassword = hlink_co_strdupW(This->password);

    return S_OK;
}

#undef AUTHENTICATE_THIS

static const IAuthenticateVtbl AuthenticateVtbl = {
    Authenticate_QueryInterface,
    Authenticate_AddRef,
    Authenticate_Release,
    Authenticate_Authenticate
};

#define HTTPNEGOTIATE_THIS(iface) DEFINE_THIS(ExtensionService, IHttpNegotiate, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = HTTPNEGOTIATE_THIS(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    ExtensionService *This = HTTPNEGOTIATE_THIS(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate *iface)
{
    ExtensionService *This = HTTPNEGOTIATE_THIS(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    ExtensionService *This = HTTPNEGOTIATE_THIS(iface);

    TRACE("(%p)->(%s %s %x %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders), dwReserved,
          pszAdditionalHeaders);

    if(!pszAdditionalHeaders)
        return E_INVALIDARG;

    *pszAdditionalHeaders = hlink_co_strdupW(This->headers);
    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    ExtensionService *This = HTTPNEGOTIATE_THIS(iface);

    TRACE("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);

    *pszAdditionalRequestHeaders = NULL;
    return S_OK;
}

#undef HTTPNEGOTIATE_THIS

static const IHttpNegotiateVtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse
};

/***********************************************************************
 *             HlinkCreateExtensionServices (HLINK.@)
 */
HRESULT WINAPI HlinkCreateExtensionServices(LPCWSTR pwzAdditionalHeaders,
        HWND phwnd, LPCWSTR pszUsername, LPCWSTR pszPassword,
        IUnknown *punkOuter, REFIID riid, void** ppv)
{
    ExtensionService *ret;
    int len = 0;
    HRESULT hres = S_OK;

    TRACE("%s %p %s %s %p %s %p\n",debugstr_w(pwzAdditionalHeaders),
            phwnd, debugstr_w(pszUsername), debugstr_w(pszPassword),
            punkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc(sizeof(*ret));

    ret->lpIUnknownVtbl = &ExtServUnkVtbl;
    ret->lpIAuthenticateVtbl = &AuthenticateVtbl;
    ret->lpIHttpNegotiateVtbl = &HttpNegotiateVtbl;
    ret->ref = 1;
    ret->hwnd = phwnd;
    ret->username = hlink_strdupW(pszUsername);
    ret->password = hlink_strdupW(pszPassword);

    if(pwzAdditionalHeaders)
        len = strlenW(pwzAdditionalHeaders);

    if(len && pwzAdditionalHeaders[len-1] != '\n' && pwzAdditionalHeaders[len-1] != '\r') {
        static const WCHAR endlW[] = {'\r','\n',0};
        ret->headers = heap_alloc(len*sizeof(WCHAR) + sizeof(endlW));
        memcpy(ret->headers, pwzAdditionalHeaders, len*sizeof(WCHAR));
        memcpy(ret->headers+len, endlW, sizeof(endlW));
    }else {
        ret->headers = hlink_strdupW(pwzAdditionalHeaders);
    }

    if(!punkOuter) {
        ret->outer = EXTSERVUNK(ret);
        hres = IUnknown_QueryInterface(EXTSERVUNK(ret), riid, ppv);
        IUnknown_Release(EXTSERVUNK(ret));
    }else if(IsEqualGUID(&IID_IUnknown, riid)) {
        ret->outer = punkOuter;
        *ppv = EXTSERVUNK(ret);
    }else {
        IUnknown_Release(EXTSERVUNK(ret));
        hres = E_INVALIDARG;
    }

    return hres;
}
