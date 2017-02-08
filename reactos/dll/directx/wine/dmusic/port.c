/*
 * IDirectMusicPort Implementation
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

#include <assert.h>

static inline IDirectMusicDownloadedInstrumentImpl* impl_from_IDirectMusicDownloadedInstrument(IDirectMusicDownloadedInstrument *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicDownloadedInstrumentImpl, IDirectMusicDownloadedInstrument_iface);
}

static inline SynthPortImpl *impl_from_SynthPortImpl_IDirectMusicPort(IDirectMusicPort *iface)
{
    return CONTAINING_RECORD(iface, SynthPortImpl, IDirectMusicPort_iface);
}

static inline SynthPortImpl *impl_from_SynthPortImpl_IDirectMusicPortDownload(IDirectMusicPortDownload *iface)
{
    return CONTAINING_RECORD(iface, SynthPortImpl, IDirectMusicPortDownload_iface);
}

static inline SynthPortImpl *impl_from_SynthPortImpl_IDirectMusicThru(IDirectMusicThru *iface)
{
    return CONTAINING_RECORD(iface, SynthPortImpl, IDirectMusicThru_iface);
}

/* IDirectMusicDownloadedInstrument IUnknown part follows: */
static HRESULT WINAPI IDirectMusicDownloadedInstrumentImpl_QueryInterface(IDirectMusicDownloadedInstrument *iface, REFIID riid, VOID **ret_iface)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicDownloadedInstrument))
    {
        IDirectMusicDownloadedInstrument_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_AddRef(LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface)
{
    IDirectMusicDownloadedInstrumentImpl *This = impl_from_IDirectMusicDownloadedInstrument(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_Release(LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface)
{
    IDirectMusicDownloadedInstrumentImpl *This = impl_from_IDirectMusicDownloadedInstrument(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->data);
        HeapFree(GetProcessHeap(), 0, This);
        DMUSIC_UnlockModule();
    }

    return ref;
}

static const IDirectMusicDownloadedInstrumentVtbl DirectMusicDownloadedInstrument_Vtbl = {
    IDirectMusicDownloadedInstrumentImpl_QueryInterface,
    IDirectMusicDownloadedInstrumentImpl_AddRef,
    IDirectMusicDownloadedInstrumentImpl_Release
};

static inline IDirectMusicDownloadedInstrumentImpl* unsafe_impl_from_IDirectMusicDownloadedInstrument(IDirectMusicDownloadedInstrument *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &DirectMusicDownloadedInstrument_Vtbl);

    return impl_from_IDirectMusicDownloadedInstrument(iface);
}

static HRESULT DMUSIC_CreateDirectMusicDownloadedInstrumentImpl(IDirectMusicDownloadedInstrument **instrument)
{
    IDirectMusicDownloadedInstrumentImpl *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        *instrument = NULL;
        return E_OUTOFMEMORY;
    }

    object->IDirectMusicDownloadedInstrument_iface.lpVtbl = &DirectMusicDownloadedInstrument_Vtbl;
    object->ref = 1;

    *instrument = &object->IDirectMusicDownloadedInstrument_iface;
    DMUSIC_LockModule();

    return S_OK;
}

/* SynthPortImpl IDirectMusicPort IUnknown part follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_QueryInterface(LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ret_iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPort)) {
        *ret_iface = &This->IDirectMusicPort_iface;
        IDirectMusicPort_AddRef((LPDIRECTMUSICPORT)*ret_iface);
        return S_OK;
    } else if (IsEqualGUID(riid, &IID_IDirectMusicPortDownload)) {
        *ret_iface = &This->IDirectMusicPortDownload_iface;
        IDirectMusicPortDownload_AddRef((LPDIRECTMUSICPORTDOWNLOAD)*ret_iface);
        return S_OK;
    } else if (IsEqualGUID(riid, &IID_IDirectMusicThru)) {
        *ret_iface = &This->IDirectMusicThru_iface;
        IDirectMusicThru_AddRef((LPDIRECTMUSICTHRU)*ret_iface);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI SynthPortImpl_IDirectMusicPort_AddRef(LPDIRECTMUSICPORT iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    DMUSIC_LockModule();

    return ref;
}

static ULONG WINAPI SynthPortImpl_IDirectMusicPort_Release(LPDIRECTMUSICPORT iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", This, ref);

    if (!ref)
    {
        IDirectMusicSynth_Activate(This->synth, FALSE);
        IDirectMusicSynth_Close(This->synth);
        IDirectMusicSynth_Release(This->synth);
        IDirectMusicSynthSink_Release(This->synth_sink);
        IReferenceClock_Release(This->pLatencyClock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    DMUSIC_UnlockModule();

    return ref;
}

/* SynthPortImpl IDirectMusicPort interface follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_PlayBuffer(LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER buffer)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
    HRESULT hr;
    REFERENCE_TIME time;
    LPBYTE data;
    DWORD size;

    TRACE("(%p/%p)->(%p)\n", iface, This, buffer);

    if (!buffer)
        return E_POINTER;

    hr = IDirectMusicBuffer_GetStartTime(buffer, &time);

    if (SUCCEEDED(hr))
        hr = IDirectMusicBuffer_GetRawBufferPtr(buffer, &data);

    if (SUCCEEDED(hr))
        hr = IDirectMusicBuffer_GetUsedBytes(buffer, &size);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_PlayBuffer(This->synth, time, data, size);

    return hr;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_SetReadNotificationHandle(LPDIRECTMUSICPORT iface, HANDLE event)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, event);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_Read(LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER buffer)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, buffer);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_DownloadInstrument(LPDIRECTMUSICPORT iface, IDirectMusicInstrument* instrument, IDirectMusicDownloadedInstrument** downloaded_instrument, DMUS_NOTERANGE* note_ranges, DWORD num_note_ranges)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
    IDirectMusicInstrumentImpl *instrument_object;
    HRESULT ret;
    BOOL free;
    HANDLE download;
    DMUS_DOWNLOADINFO *info;
    DMUS_OFFSETTABLE *offset_table;
    DMUS_INSTRUMENT *instrument_info;
    BYTE *data;
    ULONG offset;
    ULONG nb_regions;
    ULONG size;
    ULONG i;

    TRACE("(%p/%p)->(%p, %p, %p, %d)\n", iface, This, instrument, downloaded_instrument, note_ranges, num_note_ranges);

    if (!instrument || !downloaded_instrument || (num_note_ranges && !note_ranges))
        return E_POINTER;

    instrument_object = impl_from_IDirectMusicInstrument(instrument);

    nb_regions = instrument_object->header.cRegions;
    size = sizeof(DMUS_DOWNLOADINFO) + sizeof(ULONG) * (1 + nb_regions) + sizeof(DMUS_INSTRUMENT) + sizeof(DMUS_REGION) * nb_regions;

    data = HeapAlloc(GetProcessHeap(), 0, size);
    if (!data)
        return E_OUTOFMEMORY;

    info = (DMUS_DOWNLOADINFO*)data;
    offset_table = (DMUS_OFFSETTABLE*)(data + sizeof(DMUS_DOWNLOADINFO));
    offset = sizeof(DMUS_DOWNLOADINFO) + sizeof(ULONG) * (1 + nb_regions);

    info->dwDLType = DMUS_DOWNLOADINFO_INSTRUMENT2;
    info->dwDLId = 0;
    info->dwNumOffsetTableEntries = 1 + instrument_object->header.cRegions;
    info->cbSize = size;

    offset_table->ulOffsetTable[0] = offset;
    instrument_info = (DMUS_INSTRUMENT*)(data + offset);
    offset += sizeof(DMUS_INSTRUMENT);
    instrument_info->ulPatch = MIDILOCALE2Patch(&instrument_object->header.Locale);
    instrument_info->ulFirstRegionIdx = 1;
    instrument_info->ulGlobalArtIdx = 0; /* FIXME */
    instrument_info->ulFirstExtCkIdx = 0; /* FIXME */
    instrument_info->ulCopyrightIdx = 0; /* FIXME */
    instrument_info->ulFlags = 0; /* FIXME */

    for (i = 0;  i < nb_regions; i++)
    {
        DMUS_REGION *region = (DMUS_REGION*)(data + offset);

        offset_table->ulOffsetTable[1 + i] = offset;
        offset += sizeof(DMUS_REGION);
        region->RangeKey = instrument_object->regions[i].header.RangeKey;
        region->RangeVelocity = instrument_object->regions[i].header.RangeVelocity;
        region->fusOptions = instrument_object->regions[i].header.fusOptions;
        region->usKeyGroup = instrument_object->regions[i].header.usKeyGroup;
        region->ulRegionArtIdx = 0; /* FIXME */
        region->ulNextRegionIdx = i != (nb_regions - 1) ? (i + 2) : 0;
        region->ulFirstExtCkIdx = 0; /* FIXME */
        region->WaveLink = instrument_object->regions[i].wave_link;
        region->WSMP = instrument_object->regions[i].wave_sample;
        region->WLOOP[0] = instrument_object->regions[i].wave_loop;
    }

    ret = IDirectMusicSynth8_Download(This->synth, &download, (VOID*)data, &free);

    if (SUCCEEDED(ret))
        ret = DMUSIC_CreateDirectMusicDownloadedInstrumentImpl(downloaded_instrument);

    if (SUCCEEDED(ret))
    {
        IDirectMusicDownloadedInstrumentImpl *downloaded_object = impl_from_IDirectMusicDownloadedInstrument(*downloaded_instrument);

        downloaded_object->data = data;
        downloaded_object->downloaded = TRUE;
    }

    *downloaded_instrument = NULL;
    HeapFree(GetProcessHeap(), 0, data);

    return E_FAIL;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_UnloadInstrument(LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *downloaded_instrument)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
    IDirectMusicDownloadedInstrumentImpl *downloaded_object = unsafe_impl_from_IDirectMusicDownloadedInstrument(downloaded_instrument);

    TRACE("(%p/%p)->(%p)\n", iface, This, downloaded_instrument);

    if (!downloaded_instrument)
        return E_POINTER;

    if (!downloaded_object->downloaded)
        return DMUS_E_NOT_DOWNLOADED_TO_PORT;

    HeapFree(GetProcessHeap(), 0, downloaded_object->data);
    downloaded_object->data = NULL;
    downloaded_object->downloaded = FALSE;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetLatencyClock(LPDIRECTMUSICPORT iface, IReferenceClock** clock)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, clock);

    *clock = This->pLatencyClock;
    IReferenceClock_AddRef(*clock);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetRunningStats(LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS stats)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, stats);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_Compact(LPDIRECTMUSICPORT iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetCaps(LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS port_caps)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, port_caps);

    *port_caps = This->caps;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_DeviceIoControl(LPDIRECTMUSICPORT iface, DWORD io_control_code, LPVOID in_buffer, DWORD in_buffer_size,
                                                           LPVOID out_buffer, DWORD out_buffer_size, LPDWORD bytes_returned, LPOVERLAPPED overlapped)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d, %p, %d, %p, %d, %p, %p): stub\n", iface, This, io_control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, bytes_returned, overlapped);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_SetNumChannelGroups(LPDIRECTMUSICPORT iface, DWORD channel_groups)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d): semi-stub\n", iface, This, channel_groups);

    This->nrofgroups = channel_groups;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetNumChannelGroups(LPDIRECTMUSICPORT iface, LPDWORD channel_groups)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, channel_groups);

    *channel_groups = This->nrofgroups;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_Activate(LPDIRECTMUSICPORT iface, BOOL active)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%d)\n", iface, This, active);

    This->fActive = active;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_SetChannelPriority(LPDIRECTMUSICPORT iface, DWORD channel_group, DWORD channel, DWORD priority)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%d, %d, %d): semi-stub\n", iface, This, channel_group, channel, priority);

    if (channel > 16)
    {
        WARN("isn't there supposed to be 16 channels (no. %d requested)?! (faking as it is ok)\n", channel);
        /*return E_INVALIDARG;*/
    }

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetChannelPriority(LPDIRECTMUSICPORT iface, DWORD channel_group, DWORD channel, LPDWORD priority)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    TRACE("(%p/%p)->(%u, %u, %p)\n", iface, This, channel_group, channel, priority);

    *priority = This->group[channel_group - 1].channel[channel].priority;

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_SetDirectSound(LPDIRECTMUSICPORT iface, LPDIRECTSOUND direct_sound, LPDIRECTSOUNDBUFFER direct_sound_buffer)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, direct_sound, direct_sound_buffer);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPort_GetFormat(LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize)
{
	SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPort(iface);
	WAVEFORMATEX format;
	FIXME("(%p, %p, %p, %p): stub\n", This, pWaveFormatEx, pdwWaveFormatExSize, pdwBufferSize);

	if (pWaveFormatEx == NULL)
	{
		if (pdwWaveFormatExSize)
			*pdwWaveFormatExSize = sizeof(format);
		else
			return E_POINTER;
	}
	else
	{
		if (pdwWaveFormatExSize == NULL)
			return E_POINTER;

		/* Just fill this in with something that will not crash Direct Sound for now. */
		/* It won't be used anyway until Performances are completed */
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2; /* This->params.dwAudioChannels; */
		format.nSamplesPerSec = 44100; /* This->params.dwSampleRate; */
		format.wBitsPerSample = 16;	/* FIXME: check this */
		format.nBlockAlign = (format.wBitsPerSample * format.nChannels) / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		if (*pdwWaveFormatExSize >= sizeof(format))
		{
			CopyMemory(pWaveFormatEx, &format, min(sizeof(format), *pdwWaveFormatExSize));
			*pdwWaveFormatExSize = sizeof(format);	/* FIXME check if this is set */
		}
		else
			return E_POINTER;	/* FIXME find right error */
	}

	if (pdwBufferSize)
		*pdwBufferSize = 44100 * 2 * 2;
	else
		return E_POINTER;

	return S_OK;
}

static const IDirectMusicPortVtbl SynthPortImpl_DirectMusicPort_Vtbl = {
    /**** IDirectMusicPort IUnknown part methods ***/
    SynthPortImpl_IDirectMusicPort_QueryInterface,
    SynthPortImpl_IDirectMusicPort_AddRef,
    SynthPortImpl_IDirectMusicPort_Release,
    /**** IDirectMusicPort methods ***/
    SynthPortImpl_IDirectMusicPort_PlayBuffer,
    SynthPortImpl_IDirectMusicPort_SetReadNotificationHandle,
    SynthPortImpl_IDirectMusicPort_Read,
    SynthPortImpl_IDirectMusicPort_DownloadInstrument,
    SynthPortImpl_IDirectMusicPort_UnloadInstrument,
    SynthPortImpl_IDirectMusicPort_GetLatencyClock,
    SynthPortImpl_IDirectMusicPort_GetRunningStats,
    SynthPortImpl_IDirectMusicPort_Compact,
    SynthPortImpl_IDirectMusicPort_GetCaps,
    SynthPortImpl_IDirectMusicPort_DeviceIoControl,
    SynthPortImpl_IDirectMusicPort_SetNumChannelGroups,
    SynthPortImpl_IDirectMusicPort_GetNumChannelGroups,
    SynthPortImpl_IDirectMusicPort_Activate,
    SynthPortImpl_IDirectMusicPort_SetChannelPriority,
    SynthPortImpl_IDirectMusicPort_GetChannelPriority,
    SynthPortImpl_IDirectMusicPort_SetDirectSound,
    SynthPortImpl_IDirectMusicPort_GetFormat
};

/* SynthPortImpl IDirectMusicPortDownload IUnknown part follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_QueryInterface(LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ret_iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI SynthPortImpl_IDirectMusicPortDownload_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI SynthPortImpl_IDirectMusicPortDownload_Release(LPDIRECTMUSICPORTDOWNLOAD iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

/* SynthPortImpl IDirectMusicPortDownload Interface follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_GetBuffer(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD DLId, IDirectMusicDownload** IDMDownload)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%u, %p): stub\n", iface, This, DLId, IDMDownload);

    if (!IDMDownload)
        return E_POINTER;

    return DMUSIC_CreateDirectMusicDownloadImpl(&IID_IDirectMusicDownload, (LPVOID*)IDMDownload, NULL);
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_AllocateBuffer(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD size, IDirectMusicDownload** IDMDownload)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%u, %p): stub\n", iface, This, size, IDMDownload);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_GetDLId(LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* start_DLId, DWORD count)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p, %u): stub\n", iface, This, start_DLId, count);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* append)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, append);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_Download(LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* IDMDownload)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, IDMDownload);

    return S_OK;
}

static HRESULT WINAPI SynthPortImpl_IDirectMusicPortDownload_Unload(LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* IDMDownload)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, IDMDownload);

    return S_OK;
}

static const IDirectMusicPortDownloadVtbl SynthPortImpl_DirectMusicPortDownload_Vtbl = {
    /*** IDirectMusicPortDownload IUnknown part methods ***/
    SynthPortImpl_IDirectMusicPortDownload_QueryInterface,
    SynthPortImpl_IDirectMusicPortDownload_AddRef,
    SynthPortImpl_IDirectMusicPortDownload_Release,
    /*** IDirectMusicPortDownload methods ***/
    SynthPortImpl_IDirectMusicPortDownload_GetBuffer,
    SynthPortImpl_IDirectMusicPortDownload_AllocateBuffer,
    SynthPortImpl_IDirectMusicPortDownload_GetDLId,
    SynthPortImpl_IDirectMusicPortDownload_GetAppend,
    SynthPortImpl_IDirectMusicPortDownload_Download,
    SynthPortImpl_IDirectMusicPortDownload_Unload
};

/* SynthPortImpl IDirectMusicThru IUnknown part follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicThru_QueryInterface(LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ret_iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicThru(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI SynthPortImpl_IDirectMusicThru_AddRef(LPDIRECTMUSICTHRU iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicThru(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI SynthPortImpl_IDirectMusicThru_Release(LPDIRECTMUSICTHRU iface)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicThru(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

/*  SynthPortImpl IDirectMusicThru Interface follows: */
static HRESULT WINAPI SynthPortImpl_IDirectMusicThru_ThruChannel(LPDIRECTMUSICTHRU iface, DWORD source_channel_group, DWORD source_channel, DWORD destination_channel_group,
                                                       DWORD destination_channel, LPDIRECTMUSICPORT destination_port)
{
    SynthPortImpl *This = impl_from_SynthPortImpl_IDirectMusicThru(iface);

    FIXME("(%p/%p)->(%d, %d, %d, %d, %p): stub\n", iface, This, source_channel_group, source_channel, destination_channel_group, destination_channel, destination_port);

    return S_OK;
}

static const IDirectMusicThruVtbl SynthPortImpl_DirectMusicThru_Vtbl = {
    /*** IDirectMusicThru IUnknown part methods */
    SynthPortImpl_IDirectMusicThru_QueryInterface,
    SynthPortImpl_IDirectMusicThru_AddRef,
    SynthPortImpl_IDirectMusicThru_Release,
    /*** IDirectMusicThru methods ***/
    SynthPortImpl_IDirectMusicThru_ThruChannel
};

HRESULT DMUSIC_CreateSynthPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device)
{
    SynthPortImpl *obj;
    HRESULT hr = E_FAIL;
    int i;

    TRACE("(%p,%p,%p,%p,%p,%d)\n", guid, object, unkouter, port_params, port_caps, device);

    *object = NULL;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SynthPortImpl));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IDirectMusicPort_iface.lpVtbl = &SynthPortImpl_DirectMusicPort_Vtbl;
    obj->IDirectMusicPortDownload_iface.lpVtbl = &SynthPortImpl_DirectMusicPortDownload_Vtbl;
    obj->IDirectMusicThru_iface.lpVtbl = &SynthPortImpl_DirectMusicThru_Vtbl;
    obj->ref = 0;  /* Will be inited by QueryInterface */
    obj->fActive = FALSE;
    obj->params = *port_params;
    obj->caps = *port_caps;

    hr = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (LPVOID*)&obj->pLatencyClock, NULL);
    if (hr != S_OK)
    {
        HeapFree(GetProcessHeap(), 0, obj);
        return hr;
    }

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth, (void**)&obj->synth);

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink, (void**)&obj->synth_sink);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_SetMasterClock(obj->synth, obj->pLatencyClock);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynthSink_SetMasterClock(obj->synth_sink, obj->pLatencyClock);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_SetSynthSink(obj->synth, obj->synth_sink);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_Open(obj->synth, port_params);

    if (0)
    {
        if (port_params->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) {
            obj->nrofgroups = port_params->dwChannelGroups;
            /* Setting default priorities */
            for (i = 0; i < obj->nrofgroups; i++) {
                TRACE ("Setting default channel priorities on channel group %i\n", i + 1);
                obj->group[i].channel[0].priority = DAUD_CHAN1_DEF_VOICE_PRIORITY;
                obj->group[i].channel[1].priority = DAUD_CHAN2_DEF_VOICE_PRIORITY;
                obj->group[i].channel[2].priority = DAUD_CHAN3_DEF_VOICE_PRIORITY;
                obj->group[i].channel[3].priority = DAUD_CHAN4_DEF_VOICE_PRIORITY;
                obj->group[i].channel[4].priority = DAUD_CHAN5_DEF_VOICE_PRIORITY;
                obj->group[i].channel[5].priority = DAUD_CHAN6_DEF_VOICE_PRIORITY;
                obj->group[i].channel[6].priority = DAUD_CHAN7_DEF_VOICE_PRIORITY;
                obj->group[i].channel[7].priority = DAUD_CHAN8_DEF_VOICE_PRIORITY;
                obj->group[i].channel[8].priority = DAUD_CHAN9_DEF_VOICE_PRIORITY;
                obj->group[i].channel[9].priority = DAUD_CHAN10_DEF_VOICE_PRIORITY;
                obj->group[i].channel[10].priority = DAUD_CHAN11_DEF_VOICE_PRIORITY;
                obj->group[i].channel[11].priority = DAUD_CHAN12_DEF_VOICE_PRIORITY;
                obj->group[i].channel[12].priority = DAUD_CHAN13_DEF_VOICE_PRIORITY;
                obj->group[i].channel[13].priority = DAUD_CHAN14_DEF_VOICE_PRIORITY;
                obj->group[i].channel[14].priority = DAUD_CHAN15_DEF_VOICE_PRIORITY;
                obj->group[i].channel[15].priority = DAUD_CHAN16_DEF_VOICE_PRIORITY;
            }
        }
    }

    if (SUCCEEDED(hr))
        return IDirectMusicPort_QueryInterface((LPDIRECTMUSICPORT)obj, guid, object);

    if (obj->synth)
        IDirectMusicSynth_Release(obj->synth);
    if (obj->synth_sink)
        IDirectMusicSynthSink_Release(obj->synth_sink);
    if (obj->pLatencyClock)
        IReferenceClock_Release(obj->pLatencyClock);
    HeapFree(GetProcessHeap(), 0, obj);

    return hr;
}

HRESULT DMUSIC_CreateMidiOutPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device)
{
    TRACE("(%p,%p,%p,%p,%p,%d): stub\n", guid, object, unkouter, port_params, port_caps, device);

    return E_NOTIMPL;
}

HRESULT DMUSIC_CreateMidiInPortImpl(LPCGUID guid, LPVOID *object, LPUNKNOWN unkouter, LPDMUS_PORTPARAMS port_params, LPDMUS_PORTCAPS port_caps, DWORD device)
{
    TRACE("(%p,%p,%p,%p,%p,%d): stub\n", guid, object, unkouter, port_params, port_caps, device);

    return E_NOTIMPL;
}
