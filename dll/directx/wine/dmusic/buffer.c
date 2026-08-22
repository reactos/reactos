/*
 * IDirectMusicBuffer Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2012 Christian Costa
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmusic_private.h"
#include "initguid.h"
#include "dmksctrl.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static inline IDirectMusicBufferImpl *impl_from_IDirectMusicBuffer(IDirectMusicBuffer *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicBufferImpl, IDirectMusicBuffer_iface);
}

/* IDirectMusicBufferImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicBufferImpl_QueryInterface(LPDIRECTMUSICBUFFER iface, REFIID riid, LPVOID *ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicBuffer))
    {
        IDirectMusicBuffer_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicBufferImpl_AddRef(LPDIRECTMUSICBUFFER iface)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicBufferImpl_Release(LPDIRECTMUSICBUFFER iface)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref) {
        free(This->data);
        free(This);
    }

    return ref;
}

/* IDirectMusicBufferImpl IDirectMusicBuffer part: */
static HRESULT WINAPI IDirectMusicBufferImpl_Flush(LPDIRECTMUSICBUFFER iface)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->()\n", iface);

    This->write_pos = 0;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_TotalTime(LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prtTime)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    FIXME("(%p, %p): stub\n", This, prtTime);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_PackStructured(LPDIRECTMUSICBUFFER iface, REFERENCE_TIME ref_time, DWORD channel_group, DWORD channel_message)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);
    DWORD new_write_pos = This->write_pos + DMUS_EVENT_SIZE(sizeof(channel_message));
    DMUS_EVENTHEADER *header;

    TRACE("(%p, 0x%s, %lu, %#lx)\n", iface, wine_dbgstr_longlong(ref_time), channel_group, channel_message);

    if (new_write_pos > This->size)
        return DMUS_E_BUFFER_FULL;

    /* Channel_message 0xZZYYXX (3 bytes) is a midi message where XX = status byte, YY = byte 1 and ZZ = byte 2 */

    if (!(channel_message & 0x80))
    {
        /* Status byte MSB is always set */
        return DMUS_E_INVALID_EVENT;
    }

    if (!This->write_pos)
        This->start_time = ref_time;

    header = (DMUS_EVENTHEADER*)&This->data[This->write_pos];
    header->cbEvent = 3; /* Midi message takes 4 bytes space but only 3 are relevant */
    header->dwChannelGroup = channel_group;
    header->rtDelta = ref_time - This->start_time;
    header->dwFlags = DMUS_EVENT_STRUCTURED;

    *(DWORD*)&header[1] = channel_message;
    This->write_pos = new_write_pos;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_PackUnstructured(IDirectMusicBuffer *iface,
        REFERENCE_TIME ref_time, DWORD channel_group, DWORD len, BYTE *data)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);
    DWORD new_write_pos = This->write_pos + DMUS_EVENT_SIZE(len);
    DMUS_EVENTHEADER *header;

    TRACE("(%p, 0x%s, %ld, %ld, %p)\n", This, wine_dbgstr_longlong(ref_time), channel_group, len, data);

    if (new_write_pos > This->size)
        return DMUS_E_BUFFER_FULL;

    if (!This->write_pos)
        This->start_time = ref_time;

    header = (DMUS_EVENTHEADER*)&This->data[This->write_pos];
    header->cbEvent = len;
    header->dwChannelGroup = channel_group;
    header->rtDelta = ref_time - This->start_time;
    header->dwFlags = 0;

    memcpy(&header[1], data, len);
    This->write_pos = new_write_pos;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_ResetReadPtr(LPDIRECTMUSICBUFFER iface)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    FIXME("(%p): stub\n", This);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetNextEvent(LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE* ppData)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    FIXME("(%p, %p, %p, %p, %p): stub\n", This, prt, pdwChannelGroup, pdwLength, ppData);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetRawBufferPtr(LPDIRECTMUSICBUFFER iface, LPBYTE* data)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(%p)\n", iface, data);

    if (!data)
        return E_POINTER;

    *data = This->data;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetStartTime(LPDIRECTMUSICBUFFER iface, LPREFERENCE_TIME ref_time)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(%p)\n", iface, ref_time);

    if (!ref_time)
        return E_POINTER;
    if (!This->write_pos)
        return DMUS_E_BUFFER_EMPTY;

    *ref_time = This->start_time;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetUsedBytes(LPDIRECTMUSICBUFFER iface, LPDWORD used_bytes)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(%p)\n", iface, used_bytes);

    if (!used_bytes)
        return E_POINTER;

    *used_bytes = This->write_pos;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetMaxBytes(LPDIRECTMUSICBUFFER iface, LPDWORD max_bytes)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(%p)\n", iface, max_bytes);

    if (!max_bytes)
        return E_POINTER;

    *max_bytes = This->size;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_GetBufferFormat(LPDIRECTMUSICBUFFER iface, LPGUID format)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(%p)\n", iface, format);

    if (!format)
        return E_POINTER;

    *format = This->format;
    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_SetStartTime(LPDIRECTMUSICBUFFER iface, REFERENCE_TIME ref_time)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p)->(0x%s)\n", This, wine_dbgstr_longlong(ref_time));

    This->start_time = ref_time;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicBufferImpl_SetUsedBytes(LPDIRECTMUSICBUFFER iface, DWORD used_bytes)
{
    IDirectMusicBufferImpl *This = impl_from_IDirectMusicBuffer(iface);

    TRACE("(%p, %lu)\n", iface, used_bytes);

    if (used_bytes > This->size)
        return DMUS_E_BUFFER_FULL;

    This->write_pos = used_bytes;

    return S_OK;
}

static const IDirectMusicBufferVtbl DirectMusicBuffer_Vtbl = {
	IDirectMusicBufferImpl_QueryInterface,
	IDirectMusicBufferImpl_AddRef,
	IDirectMusicBufferImpl_Release,
	IDirectMusicBufferImpl_Flush,
	IDirectMusicBufferImpl_TotalTime,
	IDirectMusicBufferImpl_PackStructured,
	IDirectMusicBufferImpl_PackUnstructured,
	IDirectMusicBufferImpl_ResetReadPtr,
	IDirectMusicBufferImpl_GetNextEvent,
	IDirectMusicBufferImpl_GetRawBufferPtr,
	IDirectMusicBufferImpl_GetStartTime,
	IDirectMusicBufferImpl_GetUsedBytes,
	IDirectMusicBufferImpl_GetMaxBytes,
	IDirectMusicBufferImpl_GetBufferFormat,
	IDirectMusicBufferImpl_SetStartTime,
	IDirectMusicBufferImpl_SetUsedBytes
};

HRESULT DMUSIC_CreateDirectMusicBufferImpl(LPDMUS_BUFFERDESC desc, LPVOID* ret_iface)
{
    IDirectMusicBufferImpl* dmbuffer;

    TRACE("(%p, %p)\n", desc, ret_iface);

    *ret_iface = NULL;

    dmbuffer = calloc(1, sizeof(IDirectMusicBufferImpl));
    if (!dmbuffer)
        return E_OUTOFMEMORY;

    dmbuffer->IDirectMusicBuffer_iface.lpVtbl = &DirectMusicBuffer_Vtbl;
    dmbuffer->ref = 1;

    if (IsEqualGUID(&desc->guidBufferFormat, &GUID_NULL))
        dmbuffer->format = KSDATAFORMAT_SUBTYPE_MIDI;
    else
        dmbuffer->format = desc->guidBufferFormat;
    dmbuffer->size = (desc->cbBuffer + 3) & ~3; /* Buffer size must be multiple of 4 bytes */

    dmbuffer->data = malloc(dmbuffer->size);
    if (!dmbuffer->data) {
        free(dmbuffer);
        return E_OUTOFMEMORY;
    }

    *ret_iface = &dmbuffer->IDirectMusicBuffer_iface;

    return S_OK;
}
