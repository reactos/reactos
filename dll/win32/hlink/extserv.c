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

WINE_DEFAULT_DEBUG_CHANNEL(hlink);

typedef struct {
    IUnknown           IUnknown_inner;
    IAuthenticate      IAuthenticate_iface;
    IHttpNegotiate     IHttpNegotiate_iface;
    IExtensionServices IExtensionServices_iface;

    IUnknown *outer_unk;
    LONG ref;

    HWND hwnd;
    LPWSTR username;
    LPWSTR password;
    LPWSTR headers;
} ExtensionService;

static inline ExtensionService *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, ExtensionService, IUnknown_inner);
}

static HRESULT WINAPI ExtServUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = impl_from_IUnknown(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        TRACE("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
        *ppv = &This->IAuthenticate_iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = &This->IHttpNegotiate_iface;
    }else if(IsEqualGUID(&IID_IExtensionServices, riid)) {
        TRACE("(%p)->(IID_IExtensionServices %p)\n", This, ppv);
        *ppv = &This->IExtensionServices_iface;
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
    ExtensionService *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ExtServUnk_Release(IUnknown *iface)
{
    ExtensionService *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This->username);
        free(This->password);
        free(This->headers);
        free(This);
    }

    return ref;
}

static const IUnknownVtbl ExtServUnkVtbl = {
    ExtServUnk_QueryInterface,
    ExtServUnk_AddRef,
    ExtServUnk_Release
};

static inline ExtensionService *impl_from_IAuthenticate(IAuthenticate *iface)
{
    return CONTAINING_RECORD(iface, ExtensionService, IAuthenticate_iface);
}

static HRESULT WINAPI Authenticate_QueryInterface(IAuthenticate *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = impl_from_IAuthenticate(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI Authenticate_AddRef(IAuthenticate *iface)
{
    ExtensionService *This = impl_from_IAuthenticate(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI Authenticate_Release(IAuthenticate *iface)
{
    ExtensionService *This = impl_from_IAuthenticate(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI Authenticate_Authenticate(IAuthenticate *iface,
        HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
    ExtensionService *This = impl_from_IAuthenticate(iface);

    TRACE("(%p)->(%p %p %p)\n", This, phwnd, pszUsername, pszPassword);

    if(!phwnd || !pszUsername || !pszPassword)
        return E_INVALIDARG;

    *phwnd = This->hwnd;
    *pszUsername = hlink_co_strdupW(This->username);
    *pszPassword = hlink_co_strdupW(This->password);

    return S_OK;
}

static const IAuthenticateVtbl AuthenticateVtbl = {
    Authenticate_QueryInterface,
    Authenticate_AddRef,
    Authenticate_Release,
    Authenticate_Authenticate
};

static inline ExtensionService *impl_from_IHttpNegotiate(IHttpNegotiate *iface)
{
    return CONTAINING_RECORD(iface, ExtensionService, IHttpNegotiate_iface);
}

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = impl_from_IHttpNegotiate(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    ExtensionService *This = impl_from_IHttpNegotiate(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate *iface)
{
    ExtensionService *This = impl_from_IHttpNegotiate(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    ExtensionService *This = impl_from_IHttpNegotiate(iface);

    TRACE("(%p)->(%s %s %lx %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders), dwReserved,
          pszAdditionalHeaders);

    if(!pszAdditionalHeaders)
        return E_INVALIDARG;

    *pszAdditionalHeaders = hlink_co_strdupW(This->headers);
    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    ExtensionService *This = impl_from_IHttpNegotiate(iface);

    TRACE("(%p)->(%ld %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);

    *pszAdditionalRequestHeaders = NULL;
    return S_OK;
}

static const IHttpNegotiateVtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse
};

static inline ExtensionService *impl_from_IExtensionServices(IExtensionServices *iface)
{
    return CONTAINING_RECORD(iface, ExtensionService, IExtensionServices_iface);
}

static HRESULT WINAPI ExtServ_QueryInterface(IExtensionServices *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = impl_from_IExtensionServices(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI ExtServ_AddRef(IExtensionServices *iface)
{
    ExtensionService *This = impl_from_IExtensionServices(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI ExtServ_Release(IExtensionServices *iface)
{
    ExtensionService *This = impl_from_IExtensionServices(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT ExtServ_ImplSetAdditionalHeaders(ExtensionService* This, LPCWSTR pwzAdditionalHeaders)
{
    int len;

    free(This->headers);
    This->headers = NULL;

    if (!pwzAdditionalHeaders)
        return S_OK;

    len = lstrlenW(pwzAdditionalHeaders);

    if(len && pwzAdditionalHeaders[len-1] != '\n' && pwzAdditionalHeaders[len-1] != '\r') {
        This->headers = malloc(len*sizeof(WCHAR) + sizeof(L"\r\n"));
        memcpy(This->headers, pwzAdditionalHeaders, len*sizeof(WCHAR));
        memcpy(This->headers+len, L"\r\n", sizeof(L"\r\n"));
    }else {
        This->headers = wcsdup(pwzAdditionalHeaders);
    }

    return S_OK;
}

static HRESULT WINAPI ExtServ_SetAdditionalHeaders(IExtensionServices* iface, LPCWSTR pwzAdditionalHeaders)
{
    ExtensionService *This = impl_from_IExtensionServices(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzAdditionalHeaders));

    return ExtServ_ImplSetAdditionalHeaders(This,pwzAdditionalHeaders);
}

static HRESULT ExtServ_ImplSetAuthenticateData(ExtensionService* This, HWND phwnd, LPCWSTR pwzUsername, LPCWSTR pwzPassword)
{
    free(This->username);
    free(This->password);

    This->hwnd = phwnd;
    This->username = wcsdup(pwzUsername);
    This->password = wcsdup(pwzPassword);

    return S_OK;
}

static HRESULT WINAPI ExtServ_SetAuthenticateData(IExtensionServices* iface, HWND phwnd, LPCWSTR pwzUsername, LPCWSTR pwzPassword)
{
    ExtensionService *This = impl_from_IExtensionServices(iface);

    TRACE("(%p)->(%p %s %s)\n", This, phwnd, debugstr_w(pwzUsername), debugstr_w(pwzPassword));

    return ExtServ_ImplSetAuthenticateData(This, phwnd, pwzUsername, pwzPassword);
}

static const IExtensionServicesVtbl ExtServVtbl = {
    ExtServ_QueryInterface,
    ExtServ_AddRef,
    ExtServ_Release,
    ExtServ_SetAdditionalHeaders,
    ExtServ_SetAuthenticateData
};

/***********************************************************************
 *             HlinkCreateExtensionServices (HLINK.@)
 */
HRESULT WINAPI HlinkCreateExtensionServices(LPCWSTR pwzAdditionalHeaders,
        HWND phwnd, LPCWSTR pszUsername, LPCWSTR pszPassword,
        IUnknown *punkOuter, REFIID riid, void** ppv)
{
    ExtensionService *ret;
    HRESULT hres = S_OK;

    TRACE("%s %p %s %s %p %s %p\n",debugstr_w(pwzAdditionalHeaders),
            phwnd, debugstr_w(pszUsername), debugstr_w(pszPassword),
            punkOuter, debugstr_guid(riid), ppv);

    if (!(ret = calloc(1, sizeof(*ret))))
        return E_OUTOFMEMORY;

    ret->IUnknown_inner.lpVtbl = &ExtServUnkVtbl;
    ret->IAuthenticate_iface.lpVtbl = &AuthenticateVtbl;
    ret->IHttpNegotiate_iface.lpVtbl = &HttpNegotiateVtbl;
    ret->IExtensionServices_iface.lpVtbl = &ExtServVtbl;
    ret->ref = 1;

    ExtServ_ImplSetAuthenticateData(ret, phwnd, pszUsername, pszPassword);
    ExtServ_ImplSetAdditionalHeaders(ret, pwzAdditionalHeaders);

    if(!punkOuter) {
        ret->outer_unk = &ret->IUnknown_inner;
        hres = IUnknown_QueryInterface(&ret->IUnknown_inner, riid, ppv);
        IUnknown_Release(&ret->IUnknown_inner);
    }else if(IsEqualGUID(&IID_IUnknown, riid)) {
        ret->outer_unk = punkOuter;
        *ppv = &ret->IUnknown_inner;
    }else {
        IUnknown_Release(&ret->IUnknown_inner);
        hres = E_INVALIDARG;
    }

    return hres;
}
