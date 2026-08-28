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
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlink);

struct link_entry
{
    struct list entry;
    IHlink *link;
};

typedef struct
{
    IHlinkBrowseContext IHlinkBrowseContext_iface;
    LONG        ref;
    HLBWINFO*   BrowseWindowInfo;
    struct link_entry *current;
    struct list links;
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
    {
        *ppvObj = &This->IHlinkBrowseContext_iface;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppvObj);
    return S_OK;
}

static ULONG WINAPI IHlinkBC_fnAddRef (IHlinkBrowseContext* iface)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IHlinkBC_fnRelease (IHlinkBrowseContext* iface)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, ref + 1);

    if (!ref)
    {
        struct link_entry *link, *link2;

        LIST_FOR_EACH_ENTRY_SAFE(link, link2, &This->links, struct link_entry, entry)
        {
            list_remove(&link->entry);
            IHlink_Release(link->link);
            free(link);
        }

        free(This->BrowseWindowInfo);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IHlinkBC_Register(IHlinkBrowseContext* iface,
        DWORD dwReserved, IUnknown *piunk, IMoniker *pimk, DWORD *pdwRegister)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);
    IMoniker *mon;
    IMoniker *composite;
    IRunningObjectTable *ROT;
    HRESULT hr;

    FIXME("(%p)->(%li %p %p %p)\n", This, dwReserved, piunk, pimk, pdwRegister);

    hr = CreateItemMoniker(NULL, L"WINEHLINK", &mon);
    if (FAILED(hr))
        return hr;
    CreateGenericComposite(mon, pimk, &composite);

    GetRunningObjectTable(0, &ROT);
    IRunningObjectTable_Register(ROT, 0, piunk, composite, pdwRegister);

    IRunningObjectTable_Release(ROT);
    IMoniker_Release(composite);
    IMoniker_Release(mon);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_GetObject(IHlinkBrowseContext* iface,
        IMoniker *pimk, BOOL fBindifRootRegistered, IUnknown **ppiunk)
{
    HlinkBCImpl *This = impl_from_IHlinkBrowseContext(iface);
    IMoniker *mon;
    IMoniker *composite;
    IRunningObjectTable *ROT;
    HRESULT hr;

    TRACE("(%p)->(%p, %d, %p)\n", This, pimk, fBindifRootRegistered, ppiunk);

    hr = CreateItemMoniker(NULL, L"WINEHLINK", &mon);
    if (FAILED(hr)) return hr;
    CreateGenericComposite(mon, pimk, &composite);

    GetRunningObjectTable(0, &ROT);
    hr = IRunningObjectTable_GetObject(ROT, composite, ppiunk);

    IRunningObjectTable_Release(ROT);
    IMoniker_Release(composite);
    IMoniker_Release(mon);

    return hr;
}

static HRESULT WINAPI IHlinkBC_Revoke(IHlinkBrowseContext* iface,
        DWORD dwRegister)
{
    HRESULT r;
    IRunningObjectTable *ROT;
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    FIXME("(%p)->(%li)\n", This, dwRegister);

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

    free(This->BrowseWindowInfo);
    This->BrowseWindowInfo = malloc(phlbwi->cbSize);
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
    HlinkBCImpl *This = impl_from_IHlinkBrowseContext(iface);
    struct link_entry *link;

    TRACE("(%p)->(%p %s %s)\n", This, pimkTarget, debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName));

    if (!list_empty(&This->links))
        return CO_E_ALREADYINITIALIZED;

    link = malloc(sizeof(struct link_entry));
    if (!link) return E_OUTOFMEMORY;

    HlinkCreateFromMoniker(pimkTarget, pwzLocation, pwzFriendlyName, NULL,
            0, NULL, &IID_IHlink, (void**)&link->link);

    list_add_head(&This->links, &link->entry);
    This->current = LIST_ENTRY(list_head(&This->links), struct link_entry, entry);
    return S_OK;
}

static HRESULT WINAPI IHlinkBC_OnNavigateHlink(IHlinkBrowseContext *iface,
        DWORD grfHLNF, IMoniker* pmkTarget, LPCWSTR pwzLocation, LPCWSTR
        pwzFriendlyName, ULONG *puHLID)
{
    HlinkBCImpl  *This = impl_from_IHlinkBrowseContext(iface);

    FIXME("(%p)->(%li %p %s %s %p)\n", This, grfHLNF, pmkTarget,
            debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), puHLID);

    return S_OK;
}

static struct link_entry *context_get_entry(HlinkBCImpl *ctxt, ULONG hlid)
{
    struct list *entry;

    switch (hlid)
    {
    case HLID_PREVIOUS:
        entry = list_prev(&ctxt->links, &ctxt->current->entry);
        break;
    case HLID_NEXT:
        entry = list_next(&ctxt->links, &ctxt->current->entry);
        break;
    case HLID_CURRENT:
        entry = &ctxt->current->entry;
        break;
    case HLID_STACKBOTTOM:
        entry = list_tail(&ctxt->links);
        break;
    case HLID_STACKTOP:
        entry = list_head(&ctxt->links);
        break;
    default:
        WARN("unknown id 0x%lx\n", hlid);
        entry = NULL;
    }

    return entry ? LIST_ENTRY(entry, struct link_entry, entry) : NULL;
}

static HRESULT WINAPI IHlinkBC_UpdateHlink(IHlinkBrowseContext* iface,
        ULONG hlid, IMoniker *target, LPCWSTR location, LPCWSTR friendly_name)
{
    HlinkBCImpl *This = impl_from_IHlinkBrowseContext(iface);
    struct link_entry *entry = context_get_entry(This, hlid);
    IHlink *link;
    HRESULT hr;

    TRACE("(%p)->(0x%lx %p %s %s)\n", This, hlid, target, debugstr_w(location), debugstr_w(friendly_name));

    if (!entry)
        return E_INVALIDARG;

    hr = HlinkCreateFromMoniker(target, location, friendly_name, NULL, 0, NULL, &IID_IHlink, (void**)&link);
    if (FAILED(hr))
        return hr;

    IHlink_Release(entry->link);
    entry->link = link;

    return S_OK;
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

static HRESULT WINAPI IHlinkBC_GetHlink(IHlinkBrowseContext* iface, ULONG hlid, IHlink **ret)
{
    HlinkBCImpl *This = impl_from_IHlinkBrowseContext(iface);
    struct link_entry *link;

    TRACE("(%p)->(0x%lx %p)\n", This, hlid, ret);

    link = context_get_entry(This, hlid);
    if (!link)
        return E_FAIL;

    *ret = link->link;
    IHlink_AddRef(*ret);

    return S_OK;
}

static HRESULT WINAPI IHlinkBC_SetCurrentHlink(IHlinkBrowseContext* iface, ULONG hlid)
{
    HlinkBCImpl *This = impl_from_IHlinkBrowseContext(iface);
    struct link_entry *link;

    TRACE("(%p)->(0x%08lx)\n", This, hlid);

    link = context_get_entry(This, hlid);
    if (!link)
        return E_FAIL;

    This->current = link;
    return S_OK;
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
    HRESULT hr;

    TRACE("unkOut=%p riid=%s\n", pUnkOuter, debugstr_guid(riid));
    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hl = calloc(1, sizeof(*hl));
    if (!hl)
        return E_OUTOFMEMORY;

    hl->ref = 1;
    hl->IHlinkBrowseContext_iface.lpVtbl = &hlvt;
    list_init(&hl->links);

    hr = IHlinkBrowseContext_QueryInterface(&hl->IHlinkBrowseContext_iface, riid, ppv);
    IHlinkBrowseContext_Release(&hl->IHlinkBrowseContext_iface);

    return hr;
}
