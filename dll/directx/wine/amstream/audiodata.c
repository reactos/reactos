/*
 * Implementation of IAudioData Interface
 *
 * Copyright 2012 Christian Costa
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

#include "amstream_private.h"

typedef struct {
    IAudioData IAudioData_iface;
    LONG ref;
} AMAudioDataImpl;

static inline AMAudioDataImpl *impl_from_IAudioData(IAudioData *iface)
{
    return CONTAINING_RECORD(iface, AMAudioDataImpl, IAudioData_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAudioDataImpl_QueryInterface(IAudioData *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMemoryData) ||
        IsEqualGUID(riid, &IID_IAudioData))
    {
        IAudioData_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAudioDataImpl_AddRef(IAudioData* iface)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, This->ref);

    return ref;
}

static ULONG WINAPI IAudioDataImpl_Release(IAudioData* iface)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, This->ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IMemoryData methods ***/
static HRESULT WINAPI IAudioDataImpl_SetBuffer(IAudioData* iface, DWORD size, BYTE *data, DWORD flags)
{
    FIXME("(%p)->(%u,%p,%x): stub\n", iface, size, data, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioDataImpl_GetInfo(IAudioData* iface, DWORD *length, BYTE **data, DWORD *actual_data)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, length, data, actual_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioDataImpl_SetActual(IAudioData* iface, DWORD data_valid)
{
    FIXME("(%p)->(%u): stub\n", iface, data_valid);

    return E_NOTIMPL;
}

/*** IAudioData methods ***/
static HRESULT WINAPI IAudioDataImpl_GetFormat(IAudioData* iface, WAVEFORMATEX *wave_format_current)
{
    FIXME("(%p)->(%p): stub\n", iface, wave_format_current);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioDataImpl_SetFormat(IAudioData* iface, const WAVEFORMATEX *wave_format)
{
    FIXME("(%p)->(%p): stub\n", iface, wave_format);

    return E_NOTIMPL;
}

static const struct IAudioDataVtbl AudioData_Vtbl =
{
    /*** IUnknown methods ***/
    IAudioDataImpl_QueryInterface,
    IAudioDataImpl_AddRef,
    IAudioDataImpl_Release,
    /*** IMemoryData methods ***/
    IAudioDataImpl_SetBuffer,
    IAudioDataImpl_GetInfo,
    IAudioDataImpl_SetActual,
    /*** IAudioData methods ***/
    IAudioDataImpl_GetFormat,
    IAudioDataImpl_SetFormat
};

HRESULT AMAudioData_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    AMAudioDataImpl *object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AMAudioDataImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAudioData_iface.lpVtbl = &AudioData_Vtbl;
    object->ref = 1;

    *ppObj = &object->IAudioData_iface;

    return S_OK;
}
