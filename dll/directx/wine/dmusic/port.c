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

#include <assert.h>
#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

struct download_entry
{
    struct list entry;
    IDirectMusicDownload *download;
    HANDLE handle;
    DWORD id;
};

struct synth_port {
    IDirectMusicPort IDirectMusicPort_iface;
    IDirectMusicPortDownload IDirectMusicPortDownload_iface;
    IDirectMusicThru IDirectMusicThru_iface;
    IKsControl IKsControl_iface;
    LONG ref;
    IDirectMusic8Impl *parent;
    IDirectSound *dsound;
    IDirectSoundBuffer *dsbuffer;
    IDirectMusicSynth *synth;
    IDirectMusicSynthSink *synth_sink;
    BOOL active;
    DMUS_PORTPARAMS params;
    int nrofgroups;
    DMUSIC_PRIVATE_CHANNEL_GROUP group[1];

    struct list downloads;
    DWORD next_dlid;
};

static inline struct synth_port *synth_from_IDirectMusicPort(IDirectMusicPort *iface)
{
    return CONTAINING_RECORD(iface, struct synth_port, IDirectMusicPort_iface);
}

static inline struct synth_port *synth_from_IDirectMusicPortDownload(IDirectMusicPortDownload *iface)
{
    return CONTAINING_RECORD(iface, struct synth_port, IDirectMusicPortDownload_iface);
}

static inline struct synth_port *synth_from_IDirectMusicThru(IDirectMusicThru *iface)
{
    return CONTAINING_RECORD(iface, struct synth_port, IDirectMusicThru_iface);
}

static inline struct synth_port *synth_from_IKsControl(IKsControl *iface)
{
    return CONTAINING_RECORD(iface, struct synth_port, IKsControl_iface);
}

static HRESULT WINAPI synth_port_QueryInterface(IDirectMusicPort *iface, REFIID riid, void **ret_iface)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicPort))
        *ret_iface = &This->IDirectMusicPort_iface;
    else if (IsEqualGUID(riid, &IID_IDirectMusicPortDownload))
        *ret_iface = &This->IDirectMusicPortDownload_iface;
    else if (IsEqualGUID(riid, &IID_IDirectMusicThru))
        *ret_iface = &This->IDirectMusicThru_iface;
    else if (IsEqualGUID(riid, &IID_IKsControl))
        *ret_iface = &This->IKsControl_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        *ret_iface = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);

    return S_OK;
}

static ULONG WINAPI synth_port_AddRef(IDirectMusicPort *iface)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI synth_port_Release(IDirectMusicPort *iface)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    if (!ref)
    {
        struct download_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->downloads, struct download_entry, entry)
        {
            list_remove(&entry->entry);
            IDirectMusicDownload_Release(entry->download);
            free(entry);
        }

        dmusic_remove_port(This->parent, iface);
        IDirectMusicSynthSink_Release(This->synth_sink);
        IDirectMusicSynth_Activate(This->synth, FALSE);
        IDirectMusicSynth_Close(This->synth);
        IDirectMusicSynth_Release(This->synth);
        if (This->dsbuffer)
           IDirectSoundBuffer_Release(This->dsbuffer);
        if (This->dsound)
           IDirectSound_Release(This->dsound);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI synth_port_PlayBuffer(IDirectMusicPort *iface, IDirectMusicBuffer *buffer)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);
    HRESULT hr;
    REFERENCE_TIME time;
    LPBYTE data;
    DWORD size;

    TRACE("(%p, %p)\n", iface, buffer);

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

static HRESULT WINAPI synth_port_SetReadNotificationHandle(IDirectMusicPort *iface, HANDLE event)
{
    TRACE("(%p, %p): method not implemented\n", iface, event);
    return E_NOTIMPL;
}

static HRESULT WINAPI synth_port_Read(IDirectMusicPort *iface, IDirectMusicBuffer *buffer)
{
    TRACE("(%p, %p): method not implemented\n", iface, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI synth_port_DownloadInstrument(IDirectMusicPort *iface, IDirectMusicInstrument *instrument,
        IDirectMusicDownloadedInstrument **downloaded_instrument, DMUS_NOTERANGE *note_ranges, DWORD num_note_ranges)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %p, %p, %p, %ld)\n", iface, instrument, downloaded_instrument, note_ranges, num_note_ranges);

    if (!instrument || !downloaded_instrument || (num_note_ranges && !note_ranges))
        return E_POINTER;

    return instrument_download_to_port(instrument, &This->IDirectMusicPortDownload_iface, downloaded_instrument);
}

static HRESULT WINAPI synth_port_UnloadInstrument(IDirectMusicPort *iface,
        IDirectMusicDownloadedInstrument *downloaded_instrument)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %p)\n", iface, downloaded_instrument);

    if (!downloaded_instrument)
        return E_POINTER;

    return instrument_unload_from_port(downloaded_instrument, &This->IDirectMusicPortDownload_iface);
}

static HRESULT WINAPI synth_port_GetLatencyClock(IDirectMusicPort *iface, IReferenceClock **clock)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %p)\n", iface, clock);

    return IDirectMusicSynth8_GetLatencyClock(This->synth, clock);
}

static HRESULT WINAPI synth_port_GetRunningStats(IDirectMusicPort *iface, DMUS_SYNTHSTATS *stats)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, stats);

    return S_OK;
}

static HRESULT WINAPI synth_port_Compact(IDirectMusicPort *iface)
{
    TRACE("(%p): method not implemented\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI synth_port_GetCaps(IDirectMusicPort *iface, DMUS_PORTCAPS *port_caps)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %p)\n", iface, port_caps);

    return IDirectMusicSynth_GetPortCaps(This->synth, port_caps);
}

static HRESULT WINAPI synth_port_DeviceIoControl(IDirectMusicPort *iface, DWORD io_control_code,
        void *in_buffer, DWORD in_buffer_size, void *out_buffer, DWORD out_buffer_size,
        DWORD *bytes_returned, OVERLAPPED *overlapped)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    FIXME("(%p/%p, %ld, %p, %ld, %p, %ld, %p, %p): stub\n", iface, This, io_control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, bytes_returned, overlapped);

    return S_OK;
}

static HRESULT WINAPI synth_port_SetNumChannelGroups(IDirectMusicPort *iface, DWORD channel_groups)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    FIXME("(%p, %ld): semi-stub\n", iface, channel_groups);

    This->nrofgroups = channel_groups;

    return S_OK;
}

static HRESULT WINAPI synth_port_GetNumChannelGroups(IDirectMusicPort *iface, DWORD *channel_groups)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %p)\n", iface, channel_groups);

    *channel_groups = This->nrofgroups;

    return S_OK;
}

static HRESULT WINAPI synth_port_Activate(IDirectMusicPort *iface, BOOL active)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", iface, This, active);

    if (This->active == active) return S_FALSE;

    if (active)
    {
        if (!This->dsound && FAILED(hr = IDirectMusicPort_SetDirectSound(iface,
                This->parent->dsound, NULL)))
            return hr;
        if (FAILED(hr = IDirectMusicSynthSink_SetDirectSound(This->synth_sink,
                This->dsound, This->dsbuffer)))
            return hr;

        if (FAILED(hr = IDirectMusicSynth_Activate(This->synth, active)))
            return hr;
        This->active = TRUE;
    }
    else
    {
        if (FAILED(hr = IDirectMusicSynth_Activate(This->synth, FALSE))) return hr;
        This->active = FALSE;

        if (FAILED(hr = IDirectMusicSynthSink_SetDirectSound(This->synth_sink, NULL, NULL)))
            return hr;
        hr = IDirectMusicPort_SetDirectSound(iface, NULL, NULL);
    }

    return hr;
}

static HRESULT WINAPI synth_port_SetChannelPriority(IDirectMusicPort *iface, DWORD channel_group,
        DWORD channel, DWORD priority)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    FIXME("(%p/%p, %ld, %ld, %ld): semi-stub\n", iface, This, channel_group, channel, priority);

    if (channel > 16)
    {
        WARN("isn't there supposed to be 16 channels (no. %ld requested)?! (faking as it is ok)\n", channel);
        /*return E_INVALIDARG;*/
    }

    return S_OK;
}

static HRESULT WINAPI synth_port_GetChannelPriority(IDirectMusicPort *iface, DWORD channel_group,
        DWORD channel, DWORD *priority)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    TRACE("(%p, %lu, %lu, %p)\n", iface, channel_group, channel, priority);

    *priority = This->group[channel_group - 1].channel[channel].priority;

    return S_OK;
}

static HRESULT WINAPI synth_port_SetDirectSound(IDirectMusicPort *iface, IDirectSound *dsound,
        IDirectSoundBuffer *dsbuffer)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);

    FIXME("(%p/%p)->(%p, %p): semi-stub\n", iface, This, dsound, dsbuffer);

    if (This->active)
        return DMUS_E_DSOUND_ALREADY_SET;

    if (This->dsound) {
        if (This->dsound != This->parent->dsound)
            ERR("Not the same dsound in the port (%p) and parent dmusic (%p), expect trouble!\n",
                    This->dsound, This->parent->dsound);
        if (!IDirectSound_Release(This->parent->dsound))
            This->parent->dsound = NULL;
    }
    if (This->dsbuffer)
        IDirectSoundBuffer_Release(This->dsbuffer);

    This->dsound = dsound;
    This->dsbuffer = dsbuffer;

    if (This->dsound)
        IDirectSound_AddRef(This->dsound);
    if (This->dsbuffer)
        IDirectSoundBuffer_AddRef(This->dsbuffer);

    return S_OK;
}

static HRESULT WINAPI synth_port_GetFormat(IDirectMusicPort *iface, WAVEFORMATEX *format,
        DWORD *fmtsize, DWORD *bufsize)
{
    struct synth_port *This = synth_from_IDirectMusicPort(iface);
    HRESULT hr;

    TRACE("(%p, %p, %p, %p)\n", This, format, fmtsize, bufsize);

    if (FAILED(hr = IDirectMusicSynth_GetFormat(This->synth, format, fmtsize)))
        return hr;

    if (bufsize)
        hr = IDirectMusicSynthSink_GetDesiredBufferSize(This->synth_sink, bufsize);

    return hr;
}

static const IDirectMusicPortVtbl synth_port_vtbl = {
    /**** IDirectMusicPort IUnknown part methods ***/
    synth_port_QueryInterface,
    synth_port_AddRef,
    synth_port_Release,
    /**** IDirectMusicPort methods ***/
    synth_port_PlayBuffer,
    synth_port_SetReadNotificationHandle,
    synth_port_Read,
    synth_port_DownloadInstrument,
    synth_port_UnloadInstrument,
    synth_port_GetLatencyClock,
    synth_port_GetRunningStats,
    synth_port_Compact,
    synth_port_GetCaps,
    synth_port_DeviceIoControl,
    synth_port_SetNumChannelGroups,
    synth_port_GetNumChannelGroups,
    synth_port_Activate,
    synth_port_SetChannelPriority,
    synth_port_GetChannelPriority,
    synth_port_SetDirectSound,
    synth_port_GetFormat
};

static HRESULT WINAPI synth_port_download_QueryInterface(IDirectMusicPortDownload *iface, REFIID riid, void **ret_iface)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI synth_port_download_AddRef(IDirectMusicPortDownload *iface)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    TRACE("(%p)\n", iface);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI synth_port_download_Release(IDirectMusicPortDownload *iface)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    TRACE("(%p)\n", iface);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

static HRESULT WINAPI synth_port_download_GetBuffer(IDirectMusicPortDownload *iface, DWORD id,
        IDirectMusicDownload **download)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);
    struct download_entry *entry;

    TRACE("(%p/%p, %lu, %p)\n", iface, This, id, download);

    if (!download) return E_POINTER;
    if (id >= This->next_dlid) return DMUS_E_INVALID_DOWNLOADID;

    LIST_FOR_EACH_ENTRY(entry, &This->downloads, struct download_entry, entry)
    {
        if (entry->id == id)
        {
            *download = entry->download;
            IDirectMusicDownload_AddRef(entry->download);
            return S_OK;
        }
    }

    return DMUS_E_NOT_DOWNLOADED_TO_PORT;
}

static HRESULT WINAPI synth_port_download_AllocateBuffer(IDirectMusicPortDownload *iface, DWORD size,
        IDirectMusicDownload **download)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p, %lu, %p)\n", iface, This, size, download);

    if (!download) return E_POINTER;
    if (!size) return E_INVALIDARG;

    return download_create(size, download);
}

static HRESULT WINAPI synth_port_download_GetDLId(IDirectMusicPortDownload *iface, DWORD *first, DWORD count)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    TRACE("(%p/%p, %p, %lu)\n", iface, This, first, count);

    if (!first) return E_POINTER;
    if (!count) return E_INVALIDARG;

    *first = This->next_dlid;
    This->next_dlid += count;

    return S_OK;
}

static HRESULT WINAPI synth_port_download_GetAppend(IDirectMusicPortDownload *iface, DWORD *append)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, append);

    return S_OK;
}

static HRESULT WINAPI synth_port_download_Download(IDirectMusicPortDownload *iface, IDirectMusicDownload *download)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);
    struct download_entry *entry;
    DMUS_DOWNLOADINFO *info;
    HANDLE handle;
    BOOL can_free;
    DWORD size;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", iface, This, download);

    if (!download) return E_POINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->downloads, struct download_entry, entry)
        if (entry->download == download) return DMUS_E_ALREADY_DOWNLOADED;

    if (!(entry = malloc(sizeof(*entry)))) return E_OUTOFMEMORY;
    if (SUCCEEDED(hr = IDirectMusicDownload_GetBuffer(download, (void **)&info, &size))
            && SUCCEEDED(hr = IDirectMusicSynth_Download(This->synth, &handle, info, &can_free)))
    {
        entry->download = download;
        IDirectMusicDownload_AddRef(download);
        entry->id = info->dwDLId;
        entry->handle = handle;
        list_add_tail(&This->downloads, &entry->entry);
    }

    if (FAILED(hr)) free(entry);
    return hr;
}

static HRESULT WINAPI synth_port_download_Unload(IDirectMusicPortDownload *iface, IDirectMusicDownload *download)
{
    struct synth_port *This = synth_from_IDirectMusicPortDownload(iface);
    struct download_entry *entry;
    HANDLE handle = 0;

    TRACE("(%p/%p)->(%p)\n", iface, This, download);

    if (!download) return E_POINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->downloads, struct download_entry, entry)
    {
        if (entry->download == download)
        {
            list_remove(&entry->entry);
            IDirectMusicDownload_Release(entry->download);
            handle = entry->handle;
            free(entry);
            break;
        }
    }

    if (!handle) return S_OK;
    return IDirectMusicSynth_Unload(This->synth, handle, NULL, NULL);
}

static const IDirectMusicPortDownloadVtbl synth_port_download_vtbl = {
    /*** IDirectMusicPortDownload IUnknown part methods ***/
    synth_port_download_QueryInterface,
    synth_port_download_AddRef,
    synth_port_download_Release,
    /*** IDirectMusicPortDownload methods ***/
    synth_port_download_GetBuffer,
    synth_port_download_AllocateBuffer,
    synth_port_download_GetDLId,
    synth_port_download_GetAppend,
    synth_port_download_Download,
    synth_port_download_Unload
};

static HRESULT WINAPI synth_port_thru_QueryInterface(IDirectMusicThru *iface, REFIID riid, void **ret_iface)
{
    struct synth_port *This = synth_from_IDirectMusicThru(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI synth_port_thru_AddRef(IDirectMusicThru *iface)
{
    struct synth_port *This = synth_from_IDirectMusicThru(iface);

    TRACE("(%p)\n", iface);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI synth_port_thru_Release(IDirectMusicThru *iface)
{
    struct synth_port *This = synth_from_IDirectMusicThru(iface);

    TRACE("(%p)\n", iface);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

static HRESULT WINAPI synth_port_thru_ThruChannel(IDirectMusicThru *iface, DWORD source_channel_group,
        DWORD source_channel, DWORD destination_channel_group, DWORD destination_channel,
        IDirectMusicPort *destination_port)
{
    struct synth_port *This = synth_from_IDirectMusicThru(iface);

    FIXME("(%p/%p, %ld, %ld, %ld, %ld, %p): stub\n", iface, This, source_channel_group, source_channel, destination_channel_group, destination_channel, destination_port);

    return S_OK;
}

static const IDirectMusicThruVtbl synth_port_thru_vtbl = {
    /*** IDirectMusicThru IUnknown part methods */
    synth_port_thru_QueryInterface,
    synth_port_thru_AddRef,
    synth_port_thru_Release,
    /*** IDirectMusicThru methods ***/
    synth_port_thru_ThruChannel
};

static HRESULT WINAPI IKsControlImpl_QueryInterface(IKsControl *iface, REFIID riid,
        void **ret_iface)
{
    struct synth_port *This = synth_from_IKsControl(iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI IKsControlImpl_AddRef(IKsControl *iface)
{
    struct synth_port *This = synth_from_IKsControl(iface);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI IKsControlImpl_Release(IKsControl *iface)
{
    struct synth_port *This = synth_from_IKsControl(iface);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

static HRESULT WINAPI IKsControlImpl_KsProperty(IKsControl *iface, KSPROPERTY *prop,
        ULONG prop_len, void *data, ULONG data_len, ULONG *ret_len)
{
    TRACE("(%p, %p, %lu, %p, %lu, %p)\n", iface, prop, prop_len, data, data_len, ret_len);
    TRACE("prop = %s - %lu - %lu\n", debugstr_guid(&prop->Set), prop->Id, prop->Flags);

    if (prop->Flags != KSPROPERTY_TYPE_GET)
    {
        FIXME("prop flags %lu not yet supported\n", prop->Flags);
        return S_FALSE;
    }

    if (data_len <  sizeof(DWORD))
        return E_NOT_SUFFICIENT_BUFFER;

    FIXME("Unknown property %s\n", debugstr_guid(&prop->Set));
    *(DWORD*)data = FALSE;
    *ret_len = sizeof(DWORD);

    return S_OK;
}

static HRESULT WINAPI IKsControlImpl_KsMethod(IKsControl *iface, KSMETHOD *method,
        ULONG method_len, void *data, ULONG data_len, ULONG *ret_len)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, method, method_len, data, data_len, ret_len);

    return E_NOTIMPL;
}

static HRESULT WINAPI IKsControlImpl_KsEvent(IKsControl *iface, KSEVENT *event, ULONG event_len,
        void *data, ULONG data_len, ULONG *ret_len)
{
    FIXME("(%p, %p, %lu, %p, %lu, %p): stub\n", iface, event, event_len, data, data_len, ret_len);

    return E_NOTIMPL;
}

static const IKsControlVtbl ikscontrol_vtbl = {
    IKsControlImpl_QueryInterface,
    IKsControlImpl_AddRef,
    IKsControlImpl_Release,
    IKsControlImpl_KsProperty,
    IKsControlImpl_KsMethod,
    IKsControlImpl_KsEvent
};

HRESULT synth_port_create(IDirectMusic8Impl *parent, DMUS_PORTPARAMS *port_params,
        DMUS_PORTCAPS *port_caps, IDirectMusicPort **port)
{
    struct synth_port *obj;
    HRESULT hr = E_FAIL;
    int i;

    TRACE("(%p, %p)\n", port_params, port);

    *port = NULL;

    obj = calloc(1, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IDirectMusicPort_iface.lpVtbl = &synth_port_vtbl;
    obj->IDirectMusicPortDownload_iface.lpVtbl = &synth_port_download_vtbl;
    obj->IDirectMusicThru_iface.lpVtbl = &synth_port_thru_vtbl;
    obj->IKsControl_iface.lpVtbl = &ikscontrol_vtbl;
    obj->ref = 1;
    obj->parent = parent;
    obj->active = FALSE;
    obj->params = *port_params;
    list_init(&obj->downloads);

    hr = CoCreateInstance(&CLSID_DirectMusicSynth, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynth,
            (void **)&obj->synth);

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_DirectMusicSynthSink, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicSynthSink, (void**)&obj->synth_sink);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_SetSynthSink(obj->synth, obj->synth_sink);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynth_SetMasterClock(obj->synth, obj->parent->master_clock);

    if (SUCCEEDED(hr))
        hr = IDirectMusicSynthSink_SetMasterClock(obj->synth_sink, obj->parent->master_clock);

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

    if (SUCCEEDED(hr)) {
        *port = &obj->IDirectMusicPort_iface;
        return S_OK;
    }

    if (obj->synth)
        IDirectMusicSynth_Release(obj->synth);
    if (obj->synth_sink)
        IDirectMusicSynthSink_Release(obj->synth_sink);
    free(obj);

    return hr;
}

struct midi_port {
    IDirectMusicPort IDirectMusicPort_iface;
    IDirectMusicThru IDirectMusicThru_iface;
    LONG ref;
    IReferenceClock *clock;
};

static inline struct midi_port *impl_from_IDirectMusicPort(IDirectMusicPort *iface)
{
    return CONTAINING_RECORD(iface, struct midi_port, IDirectMusicPort_iface);
}

static HRESULT WINAPI midi_IDirectMusicPort_QueryInterface(IDirectMusicPort *iface, REFIID riid,
        void **ret_iface)
{
    struct midi_port *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicPort))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicThru))
        *ret_iface = &This->IDirectMusicThru_iface;
    else {
        WARN("no interface for %s\n", debugstr_dmguid(riid));
        *ret_iface = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ret_iface);

    return S_OK;
}

static ULONG WINAPI midi_IDirectMusicPort_AddRef(IDirectMusicPort *iface)
{
    struct midi_port *This = impl_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI midi_IDirectMusicPort_Release(IDirectMusicPort *iface)
{
    struct midi_port *This = impl_from_IDirectMusicPort(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %lu\n", iface, ref);

    if (!ref) {
        if (This->clock)
            IReferenceClock_Release(This->clock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI midi_IDirectMusicPort_PlayBuffer(IDirectMusicPort *iface,
        IDirectMusicBuffer *buffer)
{
    FIXME("(%p, %p) stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_SetReadNotificationHandle(IDirectMusicPort *iface,
        HANDLE event)
{
    FIXME("(%p, %p) stub!\n", iface, event);

    return S_OK;
}

static HRESULT WINAPI midi_IDirectMusicPort_Read(IDirectMusicPort *iface,
        IDirectMusicBuffer *buffer)
{
    FIXME("(%p, %p) stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_DownloadInstrument(IDirectMusicPort *iface,
        IDirectMusicInstrument *instrument, IDirectMusicDownloadedInstrument **downloaded,
        DMUS_NOTERANGE *ranges, DWORD num_ranges)
{
    FIXME("(%p, %p, %p, %p, %lu) stub!\n", iface, instrument, downloaded, ranges, num_ranges);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_UnloadInstrument(IDirectMusicPort *iface,
        IDirectMusicDownloadedInstrument *downloaded)
{
    FIXME("(%p, %p) stub!\n", iface, downloaded);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetLatencyClock(IDirectMusicPort *iface,
        IReferenceClock **clock)
{
    struct midi_port *This = impl_from_IDirectMusicPort(iface);

    TRACE("(%p, %p)\n", iface, clock);

    if (!clock)
        return E_POINTER;

    *clock = This->clock;
    IReferenceClock_AddRef(*clock);

    return S_OK;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetRunningStats(IDirectMusicPort *iface,
        DMUS_SYNTHSTATS *stats)
{
    FIXME("(%p, %p) stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_Compact(IDirectMusicPort *iface)
{
    FIXME("(%p) stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetCaps(IDirectMusicPort *iface, DMUS_PORTCAPS *caps)
{
    FIXME("(%p, %p) stub!\n", iface, caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_DeviceIoControl(IDirectMusicPort *iface,
        DWORD io_control_code, void *in, DWORD size_in, void *out, DWORD size_out, DWORD *ret_len,
        OVERLAPPED *overlapped)
{
    FIXME("(%p, %lu, %p, %lu, %p, %lu, %p, %p) stub!\n", iface, io_control_code, in, size_in, out
            , size_out, ret_len, overlapped);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_SetNumChannelGroups(IDirectMusicPort *iface,
        DWORD cgroups)
{
    FIXME("(%p, %lu) stub!\n", iface, cgroups);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetNumChannelGroups(IDirectMusicPort *iface,
        DWORD *cgroups)
{
    FIXME("(%p, %p) stub!\n", iface, cgroups);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_Activate(IDirectMusicPort *iface, BOOL active)
{
    FIXME("(%p, %u) stub!\n", iface, active);

    return S_OK;
}

static HRESULT WINAPI midi_IDirectMusicPort_SetChannelPriority(IDirectMusicPort *iface,
        DWORD channel_group, DWORD channel, DWORD priority)
{
    FIXME("(%p, %lu, %lu, %lu) stub!\n", iface, channel_group, channel, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetChannelPriority(IDirectMusicPort *iface,
        DWORD channel_group, DWORD channel, DWORD *priority)
{
    FIXME("(%p, %lu, %lu, %p) stub!\n", iface, channel_group, channel, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_SetDirectSound(IDirectMusicPort *iface,
        IDirectSound *dsound, IDirectSoundBuffer *dsbuffer)
{
    FIXME("(%p, %p, %p) stub!\n", iface, dsound, dsbuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI midi_IDirectMusicPort_GetFormat(IDirectMusicPort *iface, WAVEFORMATEX *format,
        DWORD *format_size, DWORD *buffer_size)
{
    FIXME("(%p, %p, %p, %p) stub!\n", iface, format, format_size, buffer_size);

    return E_NOTIMPL;
}

static const IDirectMusicPortVtbl midi_port_vtbl = {
    midi_IDirectMusicPort_QueryInterface,
    midi_IDirectMusicPort_AddRef,
    midi_IDirectMusicPort_Release,
    midi_IDirectMusicPort_PlayBuffer,
    midi_IDirectMusicPort_SetReadNotificationHandle,
    midi_IDirectMusicPort_Read,
    midi_IDirectMusicPort_DownloadInstrument,
    midi_IDirectMusicPort_UnloadInstrument,
    midi_IDirectMusicPort_GetLatencyClock,
    midi_IDirectMusicPort_GetRunningStats,
    midi_IDirectMusicPort_Compact,
    midi_IDirectMusicPort_GetCaps,
    midi_IDirectMusicPort_DeviceIoControl,
    midi_IDirectMusicPort_SetNumChannelGroups,
    midi_IDirectMusicPort_GetNumChannelGroups,
    midi_IDirectMusicPort_Activate,
    midi_IDirectMusicPort_SetChannelPriority,
    midi_IDirectMusicPort_GetChannelPriority,
    midi_IDirectMusicPort_SetDirectSound,
    midi_IDirectMusicPort_GetFormat,
};

static inline struct midi_port *impl_from_IDirectMusicThru(IDirectMusicThru *iface)
{
    return CONTAINING_RECORD(iface, struct midi_port, IDirectMusicThru_iface);
}

static HRESULT WINAPI midi_IDirectMusicThru_QueryInterface(IDirectMusicThru *iface, REFIID riid,
        void **ret_iface)
{
    struct midi_port *This = impl_from_IDirectMusicThru(iface);

    return IDirectMusicPort_QueryInterface(&This->IDirectMusicPort_iface, riid, ret_iface);
}

static ULONG WINAPI midi_IDirectMusicThru_AddRef(IDirectMusicThru *iface)
{
    struct midi_port *This = impl_from_IDirectMusicThru(iface);

    return IDirectMusicPort_AddRef(&This->IDirectMusicPort_iface);
}

static ULONG WINAPI midi_IDirectMusicThru_Release(IDirectMusicThru *iface)
{
    struct midi_port *This = impl_from_IDirectMusicThru(iface);

    return IDirectMusicPort_Release(&This->IDirectMusicPort_iface);
}

static HRESULT WINAPI midi_IDirectMusicThru_ThruChannel(IDirectMusicThru *iface, DWORD src_group,
        DWORD src_channel, DWORD dest_group, DWORD dest_channel, IDirectMusicPort *dest_port)
{
    FIXME("(%p, %lu, %lu, %lu, %lu, %p) stub!\n", iface, src_group, src_channel, dest_group,
            dest_channel, dest_port);

    return S_OK;
}

static const IDirectMusicThruVtbl midi_thru_vtbl = {
    midi_IDirectMusicThru_QueryInterface,
    midi_IDirectMusicThru_AddRef,
    midi_IDirectMusicThru_Release,
    midi_IDirectMusicThru_ThruChannel,
};

static HRESULT midi_port_create(IDirectMusic8Impl *parent, DMUS_PORTPARAMS *params,
        DMUS_PORTCAPS *caps, IDirectMusicPort **port)
{
    struct midi_port *obj;
    HRESULT hr;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    obj->IDirectMusicPort_iface.lpVtbl = &midi_port_vtbl;
    obj->IDirectMusicThru_iface.lpVtbl = &midi_thru_vtbl;
    obj->ref = 1;

    hr = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (void **)&obj->clock, NULL);
    if (hr != S_OK) {
        free(obj);
        return hr;
    }

    *port = &obj->IDirectMusicPort_iface;

    return S_OK;
}

HRESULT midi_out_port_create(IDirectMusic8Impl *parent, DMUS_PORTPARAMS *params,
        DMUS_PORTCAPS *caps, IDirectMusicPort **port)
{
    TRACE("(%p, %p, %p, %p)\n", parent, params, caps, port);

    return midi_port_create(parent, params, caps, port);
}

HRESULT midi_in_port_create(IDirectMusic8Impl *parent, DMUS_PORTPARAMS *params,
        DMUS_PORTCAPS *caps, IDirectMusicPort **port)
{
    TRACE("(%p, %p, %p, %p)\n", parent, params, caps, port);

    return midi_port_create(parent, params, caps, port);
}
