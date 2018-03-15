/*
 * Implementation of IEnumFilters Interface
 *
 * Copyright 2004 Christian Costa
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

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct IEnumFiltersImpl
{
    IEnumFilters IEnumFilters_iface;
    LONG refCount;
    IGraphVersion * pVersionSource;
    LONG Version;
    IBaseFilter *** pppFilters;
    ULONG * pNumFilters;
    ULONG uIndex;
} IEnumFiltersImpl;

static const struct IEnumFiltersVtbl IEnumFiltersImpl_Vtbl;

static inline IEnumFiltersImpl *impl_from_IEnumFilters(IEnumFilters *iface)
{
    return CONTAINING_RECORD(iface, IEnumFiltersImpl, IEnumFilters_iface);
}

HRESULT IEnumFiltersImpl_Construct(IGraphVersion * pVersionSource, IBaseFilter *** pppFilters, ULONG * pNumFilters, IEnumFilters ** ppEnum)
{
    /* Note: The incoming IBaseFilter interfaces are not AddRef'd here as in Windows,
     * they should have been previously AddRef'd. */
    IEnumFiltersImpl * pEnumFilters = CoTaskMemAlloc(sizeof(IEnumFiltersImpl));
    HRESULT hr;
    LONG currentVersion;

    TRACE("(%p, %p, %p)\n", pppFilters, pNumFilters, ppEnum);

    *ppEnum = NULL;

    if (!pEnumFilters)
    {
        return E_OUTOFMEMORY;
    }

    pEnumFilters->IEnumFilters_iface.lpVtbl = &IEnumFiltersImpl_Vtbl;
    pEnumFilters->refCount = 1;
    pEnumFilters->uIndex = 0;
    pEnumFilters->pNumFilters = pNumFilters;
    pEnumFilters->pppFilters = pppFilters;
    IGraphVersion_AddRef(pVersionSource);
    pEnumFilters->pVersionSource = pVersionSource;

    /* Store the current version of the graph */
    hr = IGraphVersion_QueryVersion(pVersionSource, &currentVersion);
    pEnumFilters->Version = (hr==S_OK) ? currentVersion : 0;

    *ppEnum = &pEnumFilters->IEnumFilters_iface;
    return S_OK;
}

static HRESULT WINAPI IEnumFiltersImpl_QueryInterface(IEnumFilters * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumFilters))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumFiltersImpl_AddRef(IEnumFilters * iface)
{
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->()\n", iface);

    return refCount;
}

static ULONG WINAPI IEnumFiltersImpl_Release(IEnumFilters * iface)
{
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->()\n", iface);

    if (!refCount)
    {
        IGraphVersion_Release(This->pVersionSource);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static HRESULT WINAPI IEnumFiltersImpl_Next(IEnumFilters * iface, ULONG cFilters, IBaseFilter ** ppFilters, ULONG * pcFetched)
{
    ULONG cFetched; 
    ULONG i;
    LONG currentVersion;
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);
    HRESULT hr;

    cFetched = min(*This->pNumFilters, This->uIndex + cFilters) - This->uIndex;

    TRACE("(%p)->(%u, %p, %p)\n", iface, cFilters, ppFilters, pcFetched);

    /* First of all check if the graph has changed */
    hr = IGraphVersion_QueryVersion(This->pVersionSource, &currentVersion);
    if (hr==S_OK && This->Version != currentVersion)
        return VFW_E_ENUM_OUT_OF_SYNC;


    if (!ppFilters)
        return E_POINTER;

    for (i = 0; i < cFetched; i++)
    {
        ppFilters[i] = (*This->pppFilters)[This->uIndex + i];
	IBaseFilter_AddRef(ppFilters[i]);
    }

    This->uIndex += cFetched;

    if (pcFetched)
        *pcFetched = cFetched;

    if (cFetched != cFilters)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumFiltersImpl_Skip(IEnumFilters * iface, ULONG cFilters)
{
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);

    TRACE("(%p)->(%u)\n", iface, cFilters);

    if (This->uIndex + cFilters < *This->pNumFilters)
    {
        This->uIndex += cFilters;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumFiltersImpl_Reset(IEnumFilters * iface)
{
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);
    HRESULT hr;
    LONG currentVersion;

    TRACE("(%p)->()\n", iface);

    This->uIndex = 0;
    hr = IGraphVersion_QueryVersion(This->pVersionSource, &currentVersion);
    if (hr == S_OK)
        This->Version = currentVersion;
    return S_OK;
}

static HRESULT WINAPI IEnumFiltersImpl_Clone(IEnumFilters * iface, IEnumFilters ** ppEnum)
{
    HRESULT hr;
    IEnumFiltersImpl *This = impl_from_IEnumFilters(iface);

    TRACE("(%p)->(%p)\n", iface, ppEnum);

    hr = IEnumFiltersImpl_Construct(This->pVersionSource, This->pppFilters, This->pNumFilters, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumFilters_Skip(*ppEnum, This->uIndex);
}

static const IEnumFiltersVtbl IEnumFiltersImpl_Vtbl =
{
    IEnumFiltersImpl_QueryInterface,
    IEnumFiltersImpl_AddRef,
    IEnumFiltersImpl_Release,
    IEnumFiltersImpl_Next,
    IEnumFiltersImpl_Skip,
    IEnumFiltersImpl_Reset,
    IEnumFiltersImpl_Clone
};
