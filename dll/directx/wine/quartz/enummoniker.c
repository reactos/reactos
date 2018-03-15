/*
 * IEnumMoniker implementation
 *
 * Copyright 2003 Robert Shearman
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

#define COBJMACROS

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct EnumMonikerImpl
{
    IEnumMoniker IEnumMoniker_iface;
    LONG ref;
    IMoniker ** ppMoniker;
    ULONG nMonikerCount;
    ULONG index;
} EnumMonikerImpl;

static const IEnumMonikerVtbl EnumMonikerImpl_Vtbl;

static inline EnumMonikerImpl *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, EnumMonikerImpl, IEnumMoniker_iface);
}

static ULONG WINAPI EnumMonikerImpl_AddRef(LPENUMMONIKER iface);

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum)
{
    /* NOTE: assumes that array of IMonikers has already been AddRef'd
     * I.e. this function does not AddRef the array of incoming
     * IMonikers */
    EnumMonikerImpl * pemi = CoTaskMemAlloc(sizeof(EnumMonikerImpl));

    TRACE("(%p, %d, %p)\n", ppMoniker, nMonikerCount, ppEnum);

    *ppEnum = NULL;

    if (!pemi)
        return E_OUTOFMEMORY;

    pemi->IEnumMoniker_iface.lpVtbl = &EnumMonikerImpl_Vtbl;
    pemi->ref = 1;
    pemi->ppMoniker = CoTaskMemAlloc(nMonikerCount * sizeof(IMoniker*));
    memcpy(pemi->ppMoniker, ppMoniker, nMonikerCount*sizeof(IMoniker*));
    pemi->nMonikerCount = nMonikerCount;
    pemi->index = 0;

    *ppEnum = &pemi->IEnumMoniker_iface;

    return S_OK;
}

/**********************************************************************
 * IEnumMoniker_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(
    LPENUMMONIKER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumMoniker))
    {
        *ppvObj = iface;
        EnumMonikerImpl_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * IEnumMoniker_AddRef (also IUnknown)
 */
static ULONG WINAPI EnumMonikerImpl_AddRef(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG ref;

    if (This == NULL) return E_POINTER;

    ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() AddRef from %d\n", iface, ref - 1);

    return ref;
}

/**********************************************************************
 * IEnumMoniker_Release (also IUnknown)
 */
static ULONG WINAPI EnumMonikerImpl_Release(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() Release from %d\n", iface, ref + 1);

    if (!ref)
    {
        ULONG i;

        for (i = 0; i < This->nMonikerCount; i++)
            IMoniker_Release(This->ppMoniker[i]);

        CoTaskMemFree(This->ppMoniker);
        This->ppMoniker = NULL;
        CoTaskMemFree(This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI EnumMonikerImpl_Next(LPENUMMONIKER iface, ULONG celt, IMoniker ** rgelt, ULONG * pceltFetched)
{
    ULONG fetched;
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)->(%d, %p, %p)\n", iface, celt, rgelt, pceltFetched);

    for (fetched = 0; (This->index + fetched < This->nMonikerCount) && (fetched < celt); fetched++)
    {
        rgelt[fetched] = This->ppMoniker[This->index + fetched];
        IMoniker_AddRef(rgelt[fetched]);
    }

    This->index += fetched;

    TRACE("-- fetched %d\n", fetched);

    if (pceltFetched)
        *pceltFetched = fetched;

    if (fetched != celt)
        return S_FALSE;
    else
        return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Skip(LPENUMMONIKER iface, ULONG celt)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)->(%d)\n", iface, celt);

    This->index += celt;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Reset(LPENUMMONIKER iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)->()\n", iface);

    This->index = 0;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Clone(LPENUMMONIKER iface, IEnumMoniker ** ppenum)
{
    FIXME("(%p)->(%p): stub\n", iface, ppenum);

    return E_NOTIMPL;
}

/**********************************************************************
 * IEnumMoniker_Vtbl
 */
static const IEnumMonikerVtbl EnumMonikerImpl_Vtbl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};
