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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "amstream_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    IAudioData IAudioData_iface;
    LONG ref;
    DWORD size;
    BYTE *data;
    BOOL data_owned;
    DWORD actual_data;
    WAVEFORMATEX wave_format;
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

    TRACE("(%p)->(): new ref = %lu\n", iface, This->ref);

    return ref;
}

static ULONG WINAPI IAudioDataImpl_Release(IAudioData* iface)
{
    AMAudioDataImpl *audiodata = impl_from_IAudioData(iface);
    ULONG ref = InterlockedDecrement(&audiodata->ref);

    TRACE("%p decreasing refcount to %lu.\n", audiodata, ref);

    if (!ref)
    {
        if (audiodata->data_owned)
            free(audiodata->data);
        free(audiodata);
    }

    return ref;
}

/*** IMemoryData methods ***/
static HRESULT WINAPI IAudioDataImpl_SetBuffer(IAudioData* iface, DWORD size, BYTE *data, DWORD flags)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);

    TRACE("(%p)->(%lu,%p,%lx)\n", iface, size, data, flags);

    if (!size)
    {
        return E_INVALIDARG;
    }

    if (This->data_owned)
    {
        free(This->data);
        This->data_owned = FALSE;
    }

    This->size = size;
    This->data = data;

    if (!This->data)
    {
        This->data = malloc(This->size);
        This->data_owned = TRUE;
        if (!This->data)
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

static HRESULT WINAPI IAudioDataImpl_GetInfo(IAudioData* iface, DWORD *length, BYTE **data, DWORD *actual_data)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);

    TRACE("(%p)->(%p,%p,%p)\n", iface, length, data, actual_data);

    if (!This->data)
    {
        return MS_E_NOTINIT;
    }

    if (length)
    {
        *length = This->size;
    }
    if (data)
    {
        *data = This->data;
    }
    if (actual_data)
    {
        *actual_data = This->actual_data;
    }

    return S_OK;
}

static HRESULT WINAPI IAudioDataImpl_SetActual(IAudioData* iface, DWORD data_valid)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);

    TRACE("(%p)->(%lu)\n", iface, data_valid);

    if (data_valid > This->size)
    {
        return E_INVALIDARG;
    }

    This->actual_data = data_valid;

    return S_OK;
}

/*** IAudioData methods ***/
static HRESULT WINAPI IAudioDataImpl_GetFormat(IAudioData* iface, WAVEFORMATEX *wave_format_current)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);

    TRACE("(%p)->(%p)\n", iface, wave_format_current);

    if (!wave_format_current)
    {
        return E_POINTER;
    }

    *wave_format_current = This->wave_format;

    return S_OK;
}

static HRESULT WINAPI IAudioDataImpl_SetFormat(IAudioData* iface, const WAVEFORMATEX *wave_format)
{
    AMAudioDataImpl *This = impl_from_IAudioData(iface);

    TRACE("(%p)->(%p)\n", iface, wave_format);

    if (!wave_format)
    {
        return E_POINTER;
    }

    if (WAVE_FORMAT_PCM != wave_format->wFormatTag)
    {
        return E_INVALIDARG;
    }

    This->wave_format = *wave_format;

    return S_OK;
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

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAudioData_iface.lpVtbl = &AudioData_Vtbl;
    object->ref = 1;

    object->wave_format.wFormatTag = WAVE_FORMAT_PCM;
    object->wave_format.nChannels = 1;
    object->wave_format.nSamplesPerSec = 11025;
    object->wave_format.wBitsPerSample = 16;
    object->wave_format.nBlockAlign = object->wave_format.wBitsPerSample * object->wave_format.nChannels / 8;
    object->wave_format.nAvgBytesPerSec = object->wave_format.nBlockAlign * object->wave_format.nSamplesPerSec;

    *ppObj = &object->IAudioData_iface;

    return S_OK;
}
