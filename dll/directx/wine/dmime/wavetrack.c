/*
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmime_private.h"
#include "dmusic_wave.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct wave_item {
    struct list entry;
    DMUS_IO_WAVE_ITEM_HEADER header;
    IDirectMusicObject *object;
    IDirectSoundBuffer *buffer;
};

struct wave_part {
    struct list entry;
    DMUS_IO_WAVE_PART_HEADER header;
    struct list items;
};

struct wave_track
{
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
    DMUS_IO_WAVE_TRACK_HEADER header;
    struct list parts;
};

/* struct wave_track IDirectMusicTrack8 part: */
static inline struct wave_track *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, struct wave_track, IDirectMusicTrack8_iface);
}

static inline struct wave_track *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct wave_track, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI wave_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack) ||
            IsEqualIID(riid, &IID_IDirectMusicTrack8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI wave_track_AddRef(IDirectMusicTrack8 *iface)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI wave_track_Release(IDirectMusicTrack8 *iface)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct wave_item *item, *item2;
        struct wave_part *part, *part2;

        LIST_FOR_EACH_ENTRY_SAFE(part, part2, &This->parts, struct wave_part, entry)
        {
            list_remove(&part->entry);

            LIST_FOR_EACH_ENTRY_SAFE(item, item2, &part->items, struct wave_item, entry)
            {
                list_remove(&item->entry);
                if (item->buffer) IDirectSoundBuffer_Release(item->buffer);
                if (item->object) IDirectMusicObject_Release(item->object);
                free(item);
            }

            free(part);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI wave_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p): stub\n", This, pSegment);
    return S_OK;
}

static HRESULT WINAPI wave_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData,
            dwVirtualTrack8ID, dwFlags);
    return S_OK;
}

static HRESULT WINAPI wave_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p): stub\n", This, pStateData);
    return S_OK;
}

static HRESULT WINAPI wave_track_Play(IDirectMusicTrack8 *iface, void *state_data,
        MUSIC_TIME start_time, MUSIC_TIME end_time, MUSIC_TIME time_offset, DWORD track_flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state, DWORD track_id)
{
    static const DWORD handled_track_flags = DMUS_TRACKF_START | DMUS_TRACKF_SEEK | DMUS_TRACKF_DIRTY | DMUS_TRACKF_LOOP;
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG volume = This->header.lVolume;
    IDirectMusicGraph *graph;
    struct wave_part *part;
    struct wave_item *item;
    HRESULT hr;

    TRACE("(%p, %p, %ld, %ld, %ld, %#lx, %p, %p, %ld)\n", This, state_data, start_time, end_time,
            time_offset, track_flags, performance, segment_state, track_id);

    if (track_flags & ~handled_track_flags)
        FIXME("track_flags %#lx not implemented\n", track_flags & ~handled_track_flags);
    if (!(track_flags & (DMUS_TRACKF_START | DMUS_TRACKF_LOOP))) return S_OK;

    if (FAILED(hr = IDirectMusicSegmentState_QueryInterface(segment_state,
            &IID_IDirectMusicGraph, (void **)&graph)) &&
        FAILED(hr = IDirectMusicPerformance_QueryInterface(performance,
            &IID_IDirectMusicGraph, (void **)&graph)))
        return hr;

    LIST_FOR_EACH_ENTRY(part, &This->parts, struct wave_part, entry)
    {
        volume += part->header.lVolume;

        LIST_FOR_EACH_ENTRY(item, &part->items, struct wave_item, entry)
        {
            DMUS_WAVE_PMSG *msg;

            if (!item->buffer) continue;
            if (item->header.rtTime < start_time) continue;
            if (item->header.rtTime >= end_time) continue;

            if (FAILED(hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(*msg),
                    (DMUS_PMSG **)&msg)))
                break;

            msg->mtTime = item->header.rtTime + time_offset;
            msg->dwFlags = DMUS_PMSGF_MUSICTIME;
            msg->dwPChannel = part->header.dwPChannel;
            msg->dwVirtualTrackID = track_id;
            msg->dwType = DMUS_PMSGT_WAVE;
            msg->punkUser = (IUnknown *)item->buffer;
            IDirectSoundBuffer_AddRef(item->buffer);

            msg->rtStartOffset = item->header.rtStartOffset;
            msg->rtDuration = item->header.rtDuration;
            msg->lVolume = volume + item->header.lVolume;
            msg->lPitch = item->header.lPitch;

            if (FAILED(hr = IDirectMusicGraph_StampPMsg(graph, (DMUS_PMSG *)msg))
                    || FAILED(hr = IDirectMusicPerformance_SendPMsg(performance, (DMUS_PMSG *)msg)))
            {
                IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)msg);
                break;
            }
        }

        volume -= part->header.lVolume;
    }

    IDirectMusicGraph_Release(graph);
    return hr;
}

static HRESULT WINAPI wave_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *next, void *param)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): not supported\n", This, debugstr_dmguid(type), time, next, param);
    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (IsEqualGUID(type, &GUID_Disable_Auto_Download)) {
        FIXME("GUID_Disable_Auto_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_Download)) {
        FIXME("GUID_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_DownloadToAudioPath))
    {
        IDirectMusicPerformance8 *performance;
        IDirectMusicAudioPath *audio_path;
        IUnknown *object = param;
        struct wave_part *part;
        struct wave_item *item;
        IDirectSound *dsound;
        HRESULT hr;

        if (FAILED(hr = IDirectMusicAudioPath_QueryInterface(object, &IID_IDirectMusicPerformance8, (void **)&performance))
                && SUCCEEDED(hr = IDirectMusicAudioPath_QueryInterface(object, &IID_IDirectMusicAudioPath, (void **)&audio_path)))
        {
            hr = IDirectMusicAudioPath_GetObjectInPath(audio_path, DMUS_PCHANNEL_ALL, DMUS_PATH_PERFORMANCE, 0,
                    &GUID_All_Objects, 0, &IID_IDirectMusicPerformance8, (void **)&performance);
            IDirectMusicAudioPath_Release(audio_path);
        }

        if (SUCCEEDED(hr))
            hr = performance_get_dsound(performance, &dsound);
        IDirectMusicPerformance_Release(performance);

        if (FAILED(hr))
        {
            WARN("Failed to get direct sound from param %p, hr %#lx\n", param, hr);
            return hr;
        }

        LIST_FOR_EACH_ENTRY(part, &This->parts, struct wave_part, entry)
        {
            LIST_FOR_EACH_ENTRY(item, &part->items, struct wave_item, entry)
            {
                if (item->buffer) continue;
                if (FAILED(hr = wave_download_to_dsound(item->object, dsound, &item->buffer)))
                {
                    WARN("Failed to download wave %p to direct sound, hr %#lx\n", item->object, hr);
                    return hr;
                }
            }
        }

        return hr;
    }
    if (IsEqualGUID(type, &GUID_Enable_Auto_Download)) {
        FIXME("GUID_Enable_Auto_Download not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_Unload)) {
        FIXME("GUID_Unload not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_UnloadFromAudioPath))
    {
        struct wave_part *part;
        struct wave_item *item;

        LIST_FOR_EACH_ENTRY(part, &This->parts, struct wave_part, entry)
        {
            LIST_FOR_EACH_ENTRY(item, &part->items, struct wave_item, entry)
            {
                if (!item->buffer) continue;
                IDirectSoundBuffer_Release(item->buffer);
                item->buffer = NULL;
            }
        }

        return S_OK;
    }

    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    static const GUID *valid[] = {
        &GUID_Disable_Auto_Download,
        &GUID_Download,
        &GUID_DownloadToAudioPath,
        &GUID_Enable_Auto_Download,
        &GUID_Unload,
        &GUID_UnloadFromAudioPath
    };
    unsigned int i;

    TRACE("(%p, %s)\n", This, debugstr_dmguid(type));

    for (i = 0; i < ARRAY_SIZE(valid); i++)
        if (IsEqualGUID(type, valid[i]))
            return S_OK;

    TRACE("param unsupported\n");
    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI wave_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
    return S_OK;
}

static HRESULT WINAPI wave_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData,
            wine_dbgstr_longlong(rtStart), wine_dbgstr_longlong(rtEnd),
            wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
    return S_OK;
}

static HRESULT WINAPI wave_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
            wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
    return S_OK;
}

static HRESULT WINAPI wave_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
            wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
    return S_OK;
}

static HRESULT WINAPI wave_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI wave_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *newtrack,
        MUSIC_TIME join, IUnknown *context, DWORD trackgroup, IDirectMusicTrack **resulttrack)
{
    struct wave_track *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %ld, %p, %ld, %p): method not implemented\n", This, newtrack, join, context,
            trackgroup, resulttrack);
    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    wave_track_QueryInterface,
    wave_track_AddRef,
    wave_track_Release,
    wave_track_Init,
    wave_track_InitPlay,
    wave_track_EndPlay,
    wave_track_Play,
    wave_track_GetParam,
    wave_track_SetParam,
    wave_track_IsParamSupported,
    wave_track_AddNotificationType,
    wave_track_RemoveNotificationType,
    wave_track_Clone,
    wave_track_PlayEx,
    wave_track_GetParamEx,
    wave_track_SetParamEx,
    wave_track_Compose,
    wave_track_Join
};

static HRESULT parse_wave_item(struct wave_part *part, IStream *stream, struct chunk_entry *wavi)
{
    struct chunk_entry wave = {.parent = wavi};
    struct chunk_entry chunk = {.parent = &wave};
    struct wave_item *item;
    HRESULT hr;

    /* Nested list with two chunks */
    if (FAILED(hr = stream_next_chunk(stream, &wave)))
        return hr;
    if (wave.id != FOURCC_LIST || wave.type != DMUS_FOURCC_WAVE_LIST)
        return DMUS_E_UNSUPPORTED_STREAM;

    if (!(item = calloc(1, sizeof(*item)))) return E_OUTOFMEMORY;

    /* Wave item header chunk */
    if (FAILED(hr = stream_next_chunk(stream, &chunk)))
        goto error;
    if (chunk.id != DMUS_FOURCC_WAVEITEM_CHUNK) {
        hr = DMUS_E_UNSUPPORTED_STREAM;
        goto error;
    }

    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &item->header, sizeof(item->header)))) {
        WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
        goto error;
    }

    TRACE("Found DMUS_IO_WAVE_ITEM_HEADER\n");
    TRACE("\tlVolume %ld\n", item->header.lVolume);
    TRACE("\tdwVariations %ld\n", item->header.dwVariations);
    TRACE("\trtTime %s\n", wine_dbgstr_longlong(item->header.rtTime));
    TRACE("\trtStartOffset %s\n", wine_dbgstr_longlong(item->header.rtStartOffset));
    TRACE("\trtReserved %s\n", wine_dbgstr_longlong(item->header.rtReserved));
    TRACE("\trtDuration %s\n", wine_dbgstr_longlong(item->header.rtDuration));
    TRACE("\tdwLoopStart %ld\n", item->header.dwLoopStart);
    TRACE("\tdwLoopEnd %ld\n", item->header.dwLoopEnd);
    TRACE("\tdwFlags %#08lx\n", item->header.dwFlags);
    TRACE("\twVolumeRange %d\n", item->header.wVolumeRange);
    TRACE("\twPitchRange %d\n", item->header.wPitchRange);

    /* Second chunk is a reference list */
    if (stream_next_chunk(stream, &chunk) != S_OK || chunk.id != FOURCC_LIST ||
            chunk.type != DMUS_FOURCC_REF_LIST) {
        hr = DMUS_E_UNSUPPORTED_STREAM;
        goto error;
    }
    if (FAILED(hr = dmobj_parsereference(stream, &chunk, &item->object)))
        goto error;

    list_add_tail(&part->items, &item->entry);

    return S_OK;

error:
    free(item);
    return hr;
}

static HRESULT parse_wave_part(struct wave_track *This, IStream *stream, struct chunk_entry *wavp)
{
    struct chunk_entry chunk = {.parent = wavp};
    struct wave_part *part;
    HRESULT hr;

    /* Wave part header chunk */
    if (FAILED(hr = stream_next_chunk(stream, &chunk)))
        return hr;
    if (chunk.id != DMUS_FOURCC_WAVEPART_CHUNK)
        return DMUS_E_UNSUPPORTED_STREAM;

    if (!(part = calloc(1, sizeof(*part)))) return E_OUTOFMEMORY;
    list_init(&part->items);

    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &part->header, sizeof(part->header)))) {
        WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
        goto error;
    }

    TRACE("Found DMUS_IO_WAVE_PART_HEADER\n");
    TRACE("\tlVolume %ld\n", part->header.lVolume);
    TRACE("\tdwVariations %ld\n", part->header.dwVariations);
    TRACE("\tdwPChannel %ld\n", part->header.dwPChannel);
    TRACE("\tdwLockToPart %ld\n", part->header.dwLockToPart);
    TRACE("\tdwFlags %#08lx\n", part->header.dwFlags);
    TRACE("\tdwIndex %ld\n", part->header.dwIndex);

    /* Array of wave items */
    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_WAVEITEM_LIST)
            if (FAILED(hr = parse_wave_item(part, stream, &chunk)))
                break;

    if (FAILED(hr))
        goto error;

    list_add_tail(&This->parts, &part->entry);

    return S_OK;

error:
    free(part);
    return hr;
}

static HRESULT WINAPI wave_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    struct wave_track *This = impl_from_IPersistStream(iface);
    struct chunk_entry wavt = {0};
    struct chunk_entry chunk = {.parent = &wavt};
    HRESULT hr;

    TRACE("%p, %p\n", This, stream);

    if (!stream)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &wavt) != S_OK))
        return hr;
    if (wavt.id != FOURCC_LIST || wavt.type != DMUS_FOURCC_WAVETRACK_LIST)
        return DMUS_E_UNSUPPORTED_STREAM;

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(&wavt));

    /* Track header chunk */
    if (FAILED(hr = stream_next_chunk(stream, &chunk)))
        return hr;
    if (chunk.id != DMUS_FOURCC_WAVETRACK_CHUNK)
        return DMUS_E_UNSUPPORTED_STREAM;
    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header))))
        return hr;

    TRACE("Found DMUS_IO_WAVE_TRACK_HEADER\n");
    TRACE("\tlVolume %ld\n", This->header.lVolume);
    TRACE("\tdwFlags %#08lx\n", This->header.dwFlags);

    /* Array of wave parts */
    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_WAVEPART_LIST)
            if (FAILED(hr = parse_wave_part(This, stream, &chunk)))
                break;

    return SUCCEEDED(hr) ? S_OK : hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    wave_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmwavetrack(REFIID lpcGUID, void **ppobj)
{
    struct wave_track *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicWaveTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&track->parts);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}

HRESULT wave_track_create_from_chunk(IStream *stream, struct chunk_entry *parent,
        IDirectMusicTrack8 **ret_iface)
{
    IDirectMusicTrack8 *iface;
    struct wave_track *This;
    struct wave_item *item;
    struct wave_part *part;
    HRESULT hr;

    if (FAILED(hr = create_dmwavetrack(&IID_IDirectMusicTrack8, (void **)&iface))) return hr;
    This = impl_from_IDirectMusicTrack8(iface);

    if (!(part = calloc(1, sizeof(*part))))
    {
        IDirectMusicTrack8_Release(iface);
        return E_OUTOFMEMORY;
    }
    list_init(&part->items);
    list_add_tail(&This->parts, &part->entry);

    if (!(item = calloc(1, sizeof(*item)))
            || FAILED(hr = wave_create_from_chunk(stream, parent, &item->object)))
    {
        IDirectMusicTrack8_Release(iface);
        free(item);
        return hr;
    }
    if (FAILED(hr = wave_get_duration(item->object, &item->header.rtDuration)))
        WARN("Failed to get wave duration, hr %#lx\n", hr);
    list_add_tail(&part->items, &item->entry);

    *ret_iface = iface;
    return S_OK;
}
