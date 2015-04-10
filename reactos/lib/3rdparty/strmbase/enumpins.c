/*
 * Implementation of IEnumPins Interface
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

typedef struct IEnumPinsImpl
{
    IEnumPins IEnumPins_iface;
    LONG refCount;
    ULONG uIndex;
    BaseFilter *base;
    BaseFilter_GetPin receive_pin;
    BaseFilter_GetPinCount receive_pincount;
    BaseFilter_GetPinVersion receive_version;
    DWORD Version;
} IEnumPinsImpl;

static inline IEnumPinsImpl *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, IEnumPinsImpl, IEnumPins_iface);
}

static const struct IEnumPinsVtbl IEnumPinsImpl_Vtbl;

HRESULT WINAPI EnumPins_Construct(BaseFilter *base,  BaseFilter_GetPin receive_pin, BaseFilter_GetPinCount receive_pincount, BaseFilter_GetPinVersion receive_version, IEnumPins ** ppEnum)
{
    IEnumPinsImpl * pEnumPins;

    if (!ppEnum)
        return E_POINTER;

    pEnumPins = CoTaskMemAlloc(sizeof(IEnumPinsImpl));
    if (!pEnumPins)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumPins->IEnumPins_iface.lpVtbl = &IEnumPinsImpl_Vtbl;
    pEnumPins->refCount = 1;
    pEnumPins->uIndex = 0;
    pEnumPins->receive_pin = receive_pin;
    pEnumPins->receive_pincount = receive_pincount;
    pEnumPins->receive_version = receive_version;
    pEnumPins->base = base;
    IBaseFilter_AddRef(&base->IBaseFilter_iface);
    *ppEnum = &pEnumPins->IEnumPins_iface;
    pEnumPins->Version = receive_version(base);

    TRACE("Created new enumerator (%p)\n", *ppEnum);
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_QueryInterface(IEnumPins * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumPins))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumPinsImpl_AddRef(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG ref = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->(): new ref =  %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IEnumPinsImpl_Release(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG ref = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        IBaseFilter_Release(&This->base->IBaseFilter_iface);
        CoTaskMemFree(This);
    }

    return ref;
}

static HRESULT WINAPI IEnumPinsImpl_Next(IEnumPins * iface, ULONG cPins, IPin ** ppPins, ULONG * pcFetched)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG i = 0;

    TRACE("(%p)->(%u, %p, %p)\n", iface, cPins, ppPins, pcFetched);

    if (!ppPins)
        return E_POINTER;

    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;

    if (pcFetched)
        *pcFetched = 0;

    if (This->Version != This->receive_version(This->base))
        return VFW_E_ENUM_OUT_OF_SYNC;

    while (i < cPins)
    {
       IPin *pin;
       pin = This->receive_pin(This->base, This->uIndex + i);

       if (!pin)
         break;
       else
         ppPins[i] = pin;
       ++i;
    }

    if (pcFetched)
        *pcFetched = i;
    This->uIndex += i;

    if (i < cPins)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Skip(IEnumPins * iface, ULONG cPins)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);

    TRACE("(%p)->(%u)\n", iface, cPins);

    if (This->Version != This->receive_version(This->base))
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (This->receive_pincount(This->base) >= This->uIndex + cPins)
        return S_FALSE;

    This->uIndex += cPins;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Reset(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);

    TRACE("(%p)->()\n", iface);

    This->Version = This->receive_version(This->base);

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Clone(IEnumPins * iface, IEnumPins ** ppEnum)
{
    HRESULT hr;
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);

    TRACE("(%p)->(%p)\n", iface, ppEnum);

    hr = EnumPins_Construct(This->base, This->receive_pin, This->receive_pincount, This->receive_version, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumPins_Skip(*ppEnum, This->uIndex);
}

static const IEnumPinsVtbl IEnumPinsImpl_Vtbl =
{
    IEnumPinsImpl_QueryInterface,
    IEnumPinsImpl_AddRef,
    IEnumPinsImpl_Release,
    IEnumPinsImpl_Next,
    IEnumPinsImpl_Skip,
    IEnumPinsImpl_Reset,
    IEnumPinsImpl_Clone
};
