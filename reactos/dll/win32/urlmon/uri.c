/*
 * Copyright 2010 Jacek Caban for CodeWeavers
 * Copyright 2010 Thomas Mullaly
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

typedef struct {
    const IUriBuilderVtbl  *lpIUriBuilderVtbl;
    LONG ref;
} UriBuilder;

#define URI(x)         ((IUri*)  &(x)->lpIUriVtbl)
#define URIBUILDER(x)  ((IUriBuilder*)  &(x)->lpIUriBuilderVtbl)

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

    if(!pcchProperty)
        return E_INVALIDARG;

    /* Microsoft's implementation for the ZONE property of a URI seems to be lacking...
     * From what I can tell, instead of checking which URLZONE the URI belongs to it
     * simply assigns URLZONE_INVALID and returns E_NOTIMPL. This also applies to the GetZone
     * function.
     */
    if(uriProp == Uri_PROPERTY_ZONE) {
        *pcchProperty = URLZONE_INVALID;
        return E_NOTIMPL;
    }

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

    if(!pstrAbsoluteUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAuthority(IUri *iface, BSTR *pstrAuthority)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAuthority);

    if(!pstrAuthority)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDisplayUri(IUri *iface, BSTR *pstrDisplayUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDisplayUri);

    if(!pstrDisplayUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDomain(IUri *iface, BSTR *pstrDomain)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDomain);

    if(!pstrDomain)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetExtension(IUri *iface, BSTR *pstrExtension)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrExtension);

    if(!pstrExtension)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetFragment(IUri *iface, BSTR *pstrFragment)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrFragment);

    if(!pstrFragment)
        return E_POINTER;

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

    if(!pstrPassword)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPath(IUri *iface, BSTR *pstrPath)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPath);

    if(!pstrPath)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPathAndQuery(IUri *iface, BSTR *pstrPathAndQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPathAndQuery);

    if(!pstrPathAndQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetQuery(IUri *iface, BSTR *pstrQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrQuery);

    if(!pstrQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetRawUri(IUri *iface, BSTR *pstrRawUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrRawUri);

    if(!pstrRawUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetSchemeName(IUri *iface, BSTR *pstrSchemeName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrSchemeName);

    if(!pstrSchemeName)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserInfo(IUri *iface, BSTR *pstrUserInfo)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserInfo);

    if(!pstrUserInfo)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetUserName(IUri *iface, BSTR *pstrUserName)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrUserName);

    if(!pstrUserName)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHostType(IUri *iface, DWORD *pdwHostType)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwHostType);

    if(!pdwHostType)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPort(IUri *iface, DWORD *pdwPort)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwPort);

    if(!pdwPort)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetScheme(IUri *iface, DWORD *pdwScheme)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwScheme);

    if(!pdwScheme)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetZone(IUri *iface, DWORD *pdwZone)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwZone);

    if(!pdwZone)
        return E_INVALIDARG;

    /* Microsoft doesn't seem to have this implemented yet... See
     * the comment in Uri_GetPropertyDWORD for more about this.
     */
    *pdwZone = URLZONE_INVALID;
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

    if(!ppURI)
        return E_INVALIDARG;

    if(!pwzURI) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    ret = heap_alloc(sizeof(Uri));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriVtbl = &UriVtbl;
    ret->ref = 1;

    *ppURI = URI(ret);
    return S_OK;
}

#define URIBUILDER_THIS(iface) DEFINE_THIS(UriBuilder, IUriBuilder, iface)

static HRESULT WINAPI UriBuilder_QueryInterface(IUriBuilder *iface, REFIID riid, void **ppv)
{
    UriBuilder *This = URIBUILDER_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
    }else if(IsEqualGUID(&IID_IUriBuilder, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UriBuilder_AddRef(IUriBuilder *iface)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI UriBuilder_Release(IUriBuilder *iface)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI UriBuilder_CreateUriSimple(IUriBuilder *iface,
                                                 DWORD        dwAllowEncodingPropertyMask,
                                                 DWORD_PTR    dwReserved,
                                                 IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUri(IUriBuilder *iface,
                                           DWORD        dwCreateFlags,
                                           DWORD        dwAllowEncodingPropertyMask,
                                           DWORD_PTR    dwReserved,
                                           IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x %d %d %p)\n", This, dwCreateFlags, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUriWithFlags(IUriBuilder *iface,
                                         DWORD        dwCreateFlags,
                                         DWORD        dwUriBuilderFlags,
                                         DWORD        dwAllowEncodingPropertyMask,
                                         DWORD_PTR    dwReserved,
                                         IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x 0x%08x %d %d %p)\n", This, dwCreateFlags, dwUriBuilderFlags,
        dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI  UriBuilder_GetIUri(IUriBuilder *iface, IUri **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetIUri(IUriBuilder *iface, IUri *pIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetFragment(IUriBuilder *iface, DWORD *pcchFragment, LPCWSTR *ppwzFragment)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchFragment, ppwzFragment);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetHost(IUriBuilder *iface, DWORD *pcchHost, LPCWSTR *ppwzHost)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchHost, ppwzHost);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPassword(IUriBuilder *iface, DWORD *pcchPassword, LPCWSTR *ppwzPassword)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPassword, ppwzPassword);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPath(IUriBuilder *iface, DWORD *pcchPath, LPCWSTR *ppwzPath)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPath, ppwzPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPort(IUriBuilder *iface, BOOL *pfHasPort, DWORD *pdwPort)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pfHasPort, pdwPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetQuery(IUriBuilder *iface, DWORD *pcchQuery, LPCWSTR *ppwzQuery)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchQuery, ppwzQuery);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetSchemeName(IUriBuilder *iface, DWORD *pcchSchemeName, LPCWSTR *ppwzSchemeName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchSchemeName, ppwzSchemeName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetUserName(IUriBuilder *iface, DWORD *pcchUserName, LPCWSTR *ppwzUserName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchUserName, ppwzUserName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetFragment(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetHost(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPassword(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPath(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPort(IUriBuilder *iface, BOOL fHasPort, DWORD dwNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, fHasPort, dwNewValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetQuery(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetSchemeName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetUserName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_RemoveProperties(IUriBuilder *iface, DWORD dwPropertyMask)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x)\n", This, dwPropertyMask);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_HasBeenModified(IUriBuilder *iface, BOOL *pfModified)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pfModified);
    return E_NOTIMPL;
}

#undef URIBUILDER_THIS

static const IUriBuilderVtbl UriBuilderVtbl = {
    UriBuilder_QueryInterface,
    UriBuilder_AddRef,
    UriBuilder_Release,
    UriBuilder_CreateUriSimple,
    UriBuilder_CreateUri,
    UriBuilder_CreateUriWithFlags,
    UriBuilder_GetIUri,
    UriBuilder_SetIUri,
    UriBuilder_GetFragment,
    UriBuilder_GetHost,
    UriBuilder_GetPassword,
    UriBuilder_GetPath,
    UriBuilder_GetPort,
    UriBuilder_GetQuery,
    UriBuilder_GetSchemeName,
    UriBuilder_GetUserName,
    UriBuilder_SetFragment,
    UriBuilder_SetHost,
    UriBuilder_SetPassword,
    UriBuilder_SetPath,
    UriBuilder_SetPort,
    UriBuilder_SetQuery,
    UriBuilder_SetSchemeName,
    UriBuilder_SetUserName,
    UriBuilder_RemoveProperties,
    UriBuilder_HasBeenModified,
};

/***********************************************************************
 *           CreateIUriBuilder (urlmon.@)
 */
HRESULT WINAPI CreateIUriBuilder(IUri *pIUri, DWORD dwFlags, DWORD_PTR dwReserved, IUriBuilder **ppIUriBuilder)
{
    UriBuilder *ret;

    TRACE("(%p %x %x %p)\n", pIUri, dwFlags, (DWORD)dwReserved, ppIUriBuilder);

    ret = heap_alloc(sizeof(UriBuilder));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriBuilderVtbl = &UriBuilderVtbl;
    ret->ref = 1;

    *ppIUriBuilder = URIBUILDER(ret);
    return S_OK;
}
