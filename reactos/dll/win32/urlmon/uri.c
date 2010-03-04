/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IUriVtbl  *lpIUriVtbl;
    LONG ref;
} Uri;

#define URI(x)  ((IUri*)  &(x)->lpIUriVtbl)

#define URI_THIS(iface) DEFINE_THIS(Uri, IUri, iface)

static HRESULT WINAPI Uri_QueryInterface(IUri *iface, REFIID riid, void **ppv)
{
    Uri *This = URI_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URI(This);
    }else if(IsEqualGUID(&IID_IUri, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URI(This);
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Uri_AddRef(IUri *iface)
{
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Uri_Release(IUri *iface)
{
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI Uri_GetPropertyBSTR(IUri *iface, Uri_PROPERTY uriProp, BSTR *pbstrProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPropertyLength(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPropertyDWORD(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_HasProperty(IUri *iface, Uri_PROPERTY uriProp, BOOL *pfHasProperty)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAbsoluteUri(IUri *iface, BSTR *pstrAbsoluteUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAbsoluteUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAuthority(IUri *iface, BSTR *pstrAuthority)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAuthority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDisplayUri(IUri *iface, BSTR *pstrDisplayUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDisplayUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDomain(IUri *iface, BSTR *pstrDomain)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDomain);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetExtension(IUri *iface, BSTR *pstrExtension)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrExtension);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetFragment(IUri *iface, BSTR *pstrFragment)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrFragment);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHost(IUri *iface, BSTR *pstrHost)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrHost);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPassword(IUri *iface, BSTR *pstrPassword)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPassword);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPath(IUri *iface, BSTR *pstrPath)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPathAndQuery(IUri *iface, BSTR *pstrPathAndQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPathAndQuery);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetQuery(IUri *iface, BSTR *pstrQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrQuery);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetRawUri(IUri *iface, BSTR *pstrRawUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrRawUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetSchemeName(IUri *iface, BSTR *pstrSchemeName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrSchemeName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserInfo(IUri *iface, BSTR *pstrUserInfo)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserName(IUri *iface, BSTR *pstrUserName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHostType(IUri *iface, DWORD *pdwHostType)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwHostType);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPort(IUri *iface, DWORD *pdwPort)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetScheme(IUri *iface, DWORD *pdwScheme)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwScheme);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetZone(IUri *iface, DWORD *pdwZone)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwZone);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetProperties(IUri *iface, DWORD *pdwProperties)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_IsEqual(IUri *iface, IUri *pUri, BOOL *pfEqual)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pUri, pfEqual);
    return E_NOTIMPL;
}

#undef URI_THIS

static const IUriVtbl UriVtbl = {
    Uri_QueryInterface,
    Uri_AddRef,
    Uri_Release,
    Uri_GetPropertyBSTR,
    Uri_GetPropertyLength,
    Uri_GetPropertyDWORD,
    Uri_HasProperty,
    Uri_GetAbsoluteUri,
    Uri_GetAuthority,
    Uri_GetDisplayUri,
    Uri_GetDomain,
    Uri_GetExtension,
    Uri_GetFragment,
    Uri_GetHost,
    Uri_GetPassword,
    Uri_GetPath,
    Uri_GetPathAndQuery,
    Uri_GetQuery,
    Uri_GetRawUri,
    Uri_GetSchemeName,
    Uri_GetUserInfo,
    Uri_GetUserName,
    Uri_GetHostType,
    Uri_GetPort,
    Uri_GetScheme,
    Uri_GetZone,
    Uri_GetProperties,
    Uri_IsEqual
};

/***********************************************************************
 *           CreateUri (urlmon.@)
 */
HRESULT WINAPI CreateUri(LPCWSTR pwzURI, DWORD dwFlags, DWORD_PTR dwReserved, IUri **ppURI)
{
    Uri *ret;

    TRACE("(%s %x %x %p)\n", debugstr_w(pwzURI), dwFlags, (DWORD)dwReserved, ppURI);

    ret = heap_alloc(sizeof(Uri));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriVtbl = &UriVtbl;
    ret->ref = 1;

    *ppURI = URI(ret);
    return S_OK;
}
