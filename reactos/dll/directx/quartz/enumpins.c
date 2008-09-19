/*
 * Implementation of IEnumPins Interface
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

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct IEnumPinsImpl
{
    const IEnumPinsVtbl * lpVtbl;
    LONG refCount;
    ULONG uIndex;
    IBaseFilter *base;
    FNOBTAINPIN receive_pin;
    DWORD synctime;
} IEnumPinsImpl;

static const struct IEnumPinsVtbl IEnumPinsImpl_Vtbl;

HRESULT IEnumPinsImpl_Construct(IEnumPins ** ppEnum, FNOBTAINPIN receive_pin, IBaseFilter *base)
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
    pEnumPins->lpVtbl = &IEnumPinsImpl_Vtbl;
    pEnumPins->refCount = 1;
    pEnumPins->uIndex = 0;
    pEnumPins->receive_pin = receive_pin;
    pEnumPins->base = base;
    IBaseFilter_AddRef(base);
    *ppEnum = (IEnumPins *)(&pEnumPins->lpVtbl);

    receive_pin(base, ~0, NULL, &pEnumPins->synctime);

    TRACE("Created new enumerator (%p)\n", *ppEnum);
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_QueryInterface(IEnumPins * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IEnumPins))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumPinsImpl_AddRef(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IEnumPinsImpl_Release(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        IBaseFilter_Release(This->base);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static HRESULT WINAPI IEnumPinsImpl_Next(IEnumPins * iface, ULONG cPins, IPin ** ppPins, ULONG * pcFetched)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    DWORD synctime = This->synctime;
    HRESULT hr = S_OK;
    ULONG i = 0;

    TRACE("(%u, %p, %p)\n", cPins, ppPins, pcFetched);

    if (!ppPins)
        return E_POINTER;

    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;

    if (pcFetched)
        *pcFetched = 0;

    while (i < cPins && hr == S_OK)
    {
        hr = This->receive_pin(This->base, This->uIndex + i, &ppPins[i], &synctime);

        if (hr == S_OK)
            ++i;

        if (synctime != This->synctime)
            break;
    }

    if (!i && synctime != This->synctime)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (pcFetched)
        *pcFetched = i;
    This->uIndex += i;

    if (i < cPins)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Skip(IEnumPins * iface, ULONG cPins)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    DWORD synctime = This->synctime;
    HRESULT hr;
    IPin *pin = NULL;

    TRACE("(%u)\n", cPins);

    hr = This->receive_pin(This->base, This->uIndex + cPins, &pin, &synctime);
    if (pin)
        IPin_Release(pin);

    if (synctime != This->synctime)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (hr == S_OK)
        This->uIndex += cPins;

    return hr;
}

static HRESULT WINAPI IEnumPinsImpl_Reset(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    TRACE("IEnumPinsImpl::Reset()\n");
    This->receive_pin(This->base, ~0, NULL, &This->synctime);

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Clone(IEnumPins * iface, IEnumPins ** ppEnum)
{
    HRESULT hr;
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    TRACE("(%p)\n", ppEnum);

    hr = IEnumPinsImpl_Construct(ppEnum, This->receive_pin, This->base);
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
