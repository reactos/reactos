/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

typedef struct
{
    IHlinkBrowseContext IHlinkBrowseContext_iface;
    LONG        ref;
    HLBWINFO*   BrowseWindowInfo;
    IHlink*     CurrentPage;
} HlinkBCImpl;

static inline HlinkBCImpl *impl_from_IHlinkBrowseContext(IHlinkBrowseContext *iface)
{
    return CONTAINING_RECORD(iface, HlinkBCImpl, IHlinkBrowseContext_iface);
}

static HRESULT WINAPI IHlinkBC_fnQueryInterface( IHlinkBrowseContext *iface,
        REFIID riid, LPVOID* ppvObj)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    TRACE ("(%p)->(%s,%p)\n", This, debugstr_guid (riid), ppvObj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IHlinkBrowseContext))
        *ppvObj = This;

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI IHlinkBC_fnAddRef (IHlinkBrowseContext* iface)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IHlinkBC_fnRelease (IHlinkBrowseContext* iface)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);
    if (refCount)
        return refCount;

    TRACE("-- destroying IHlinkBrowseContext (%p)\n", This);
    heap_free(This->BrowseWindowInfo);
    if (This->CurrentPage)
        IHlink_Release(This->CurrentPage);
    heap_free(This);
    return 0;
}

static HRESULT WINAPI IHlinkBC_Register(IHlinkBrowseContext* iface,
        DWORD dwReserved, IUnknown *piunk, IMoniker *pimk, DWORD *pdwRegister)
{
    static const WCHAR szIdent[] = {'W','I','N','E','H','L','I','N','K',0};
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    IMoniker *mon;
    IMoniker *composite;
    IRunningObjectTable *ROT;

    FIXME("(%p)->(%i %p %p %p)\n", This, dwReserved, piunk, pimk, pdwRegister);

    CreateItemMoniker(NULL, szIdent, &mon);
    CreateGenericComposite(mon, pimk, &composite);

    GetRunningObjectTable(0, &ROT);
    IRunningObjectTable_Register(ROT, 0, piunk, composite, pdwRegister);

    IRunningObjectTable_Release(ROT);
    IMoniker_Release(composite);
    IMoniker_Release(mon);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_GetObject(IHlinkBrowseContext* face,
        IMoniker *pimk, BOOL fBindifRootRegistered, IUnknown **ppiunk)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_Revoke(IHlinkBrowseContext* iface,
        DWORD dwRegister)
{
    HRESULT r = S_OK;
    IRunningObjectTable *ROT;
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    FIXME("(%p)->(%i)\n", This, dwRegister);

    GetRunningObjectTable(0, &ROT);
    r = IRunningObjectTable_Revoke(ROT, dwRegister);
    IRunningObjectTable_Release(ROT);

    return r;
}

static HRESULT WINAPI IHlinkBC_SetBrowseWindowInfo(IHlinkBrowseContext* iface,
        HLBWINFO *phlbwi)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    TRACE("(%p)->(%p)\n", This, phlbwi);

    if(!phlbwi)
        return E_INVALIDARG;

    heap_free(This->BrowseWindowInfo);
    This->BrowseWindowInfo = heap_alloc(phlbwi->cbSize);
    memcpy(This->BrowseWindowInfo, phlbwi, phlbwi->cbSize);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_GetBrowseWindowInfo(IHlinkBrowseContext* iface,
        HLBWINFO *phlbwi)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    TRACE("(%p)->(%p)\n", This, phlbwi);

    if(!phlbwi)
        return E_INVALIDARG;

    if(!This->BrowseWindowInfo)
        phlbwi->cbSize = 0;
    else
        memcpy(phlbwi, This->BrowseWindowInfo, This->BrowseWindowInfo->cbSize);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_SetInitialHlink(IHlinkBrowseContext* iface,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    FIXME("(%p)->(%p %s %s)\n", This, pimkTarget,
            debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName));

    if (This->CurrentPage)
        IHlink_Release(This->CurrentPage);

    HlinkCreateFromMoniker(pimkTarget, pwzLocation, pwzFriendlyName, NULL,
            0, NULL, &IID_IHlink, (LPVOID*) &This->CurrentPage);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_OnNavigateHlink(IHlinkBrowseContext *iface,
        DWORD grfHLNF, IMoniker* pmkTarget, LPCWSTR pwzLocation, LPCWSTR
        pwzFriendlyName, ULONG *puHLID)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    FIXME("(%p)->(%i %p %s %s %p)\n", This, grfHLNF, pmkTarget,
            debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), puHLID);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_UpdateHlink(IHlinkBrowseContext* iface,
        ULONG uHLID, IMoniker* pimkTarget, LPCWSTR pwzLocation,
        LPCWSTR pwzFriendlyName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_EnumNavigationStack( IHlinkBrowseContext *iface,
        DWORD dwReserved, DWORD grfHLFNAMEF, IEnumHLITEM** ppienumhlitem)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_QueryHlink( IHlinkBrowseContext* iface,
        DWORD grfHLONG, ULONG uHLID)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_GetHlink( IHlinkBrowseContext* iface,
        ULONG uHLID, IHlink** ppihl)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    TRACE("(%p)->(%x %p)\n", This, uHLID, ppihl);

    if(uHLID != HLID_CURRENT) {
        FIXME("Only HLID_CURRENT implemented, given: %x\n", uHLID);
        return E_NOTIMPL;
    }

    *ppihl = This->CurrentPage;
    IHlink_AddRef(*ppihl);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_SetCurrentHlink( IHlinkBrowseContext* iface,
        ULONG uHLID)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_Clone( IHlinkBrowseContext* iface,
        IUnknown* piunkOuter, REFIID riid, IUnknown** ppiunkOjb)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlinkBC_Close(IHlinkBrowseContext* iface,
        DWORD reserved)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IHlinkBrowseContextVtbl hlvt =
{
    IHlinkBC_fnQueryInterface,
    IHlinkBC_fnAddRef,
    IHlinkBC_fnRelease,
    IHlinkBC_Register,
    IHlinkBC_GetObject,
    IHlinkBC_Revoke,
    IHlinkBC_SetBrowseWindowInfo,
    IHlinkBC_GetBrowseWindowInfo,
    IHlinkBC_SetInitialHlink,
    IHlinkBC_OnNavigateHlink,
    IHlinkBC_UpdateHlink,
    IHlinkBC_EnumNavigationStack,
    IHlinkBC_QueryHlink,
    IHlinkBC_GetHlink,
    IHlinkBC_SetCurrentHlink,
    IHlinkBC_Clone,
    IHlinkBC_Close
};

HRESULT HLinkBrowseContext_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HlinkBCImpl * hl;

    TRACE("unkOut=%p riid=%s\n", pUnkOuter, debugstr_guid(riid));
    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hl = heap_alloc_zero(sizeof(HlinkBCImpl));
    if (!hl)
        return E_OUTOFMEMORY;

    hl->ref = 1;
    hl->IHlinkBrowseContext_iface.lpVtbl = &hlvt;

    *ppv = hl;
    return S_OK;
}
