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

#include "dmband_private.h"
#include "dmusic_band.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);

struct band_entry
{
    struct list entry;
    DMUS_IO_BAND_ITEM_HEADER2 head;
    IDirectMusicBand *band;
};

static void band_entry_destroy(struct band_entry *entry)
{
    IDirectMusicTrack_Release(entry->band);
    free(entry);
}

struct band_track
{
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj; /* IPersistStream only */
    LONG ref;
    DMUS_IO_BAND_TRACK_HEADER header;
    struct list bands;
};

static inline struct band_track *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, struct band_track, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI band_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI band_track_AddRef(IDirectMusicTrack8 *iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI band_track_Release(IDirectMusicTrack8 *iface)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        struct band_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->bands, struct band_entry, entry)
        {
            list_remove(&entry->entry);
            band_entry_destroy(entry);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI band_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *segment)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %p): stub\n", This, segment);

    if (!segment) return E_POINTER;
    return S_OK;
}

static HRESULT WINAPI band_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *segment_state, IDirectMusicPerformance *performance,
        void **state_data, DWORD virtual_track8id, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    struct band_entry *entry;
    HRESULT hr;

    FIXME("(%p, %p, %p, %p, %ld, %lx): semi-stub\n", This, segment_state, performance, state_data, virtual_track8id, flags);

    if (!performance) return E_POINTER;

    if (This->header.bAutoDownload)
    {
        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
        {
            if (FAILED(hr = IDirectMusicBand_Download(entry->band, performance)))
                return hr;
        }
    }

    return S_OK;
}

static HRESULT WINAPI band_track_EndPlay(IDirectMusicTrack8 *iface, void *state_data)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    struct band_entry *entry;
    HRESULT hr;

    FIXME("(%p, %p): semi-stub\n", This, state_data);

    if (This->header.bAutoDownload)
    {
        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
        {
            if (FAILED(hr = IDirectMusicBand_Unload(entry->band, NULL)))
                return hr;
        }
    }

    return S_OK;
}

static HRESULT WINAPI band_track_Play(IDirectMusicTrack8 *iface, void *state_data,
        MUSIC_TIME start_time, MUSIC_TIME end_time, MUSIC_TIME time_offset, DWORD track_flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state, DWORD track_id)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    IDirectMusicGraph *graph;
    struct band_entry *entry;
    HRESULT hr;

    TRACE("(%p, %p, %ld, %ld, %ld, %#lx, %p, %p, %ld)\n", This, state_data, start_time, end_time,
            time_offset, track_flags, performance, segment_state, track_id);

    if (!performance) return DMUS_S_END;

    if (track_flags) FIXME("track_flags %#lx not implemented\n", track_flags);
    if (segment_state) FIXME("segment_state %p not implemented\n", segment_state);

    if (FAILED(hr = IDirectMusicPerformance_QueryInterface(performance,
            &IID_IDirectMusicGraph, (void **)&graph)))
        return hr;

    LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
    {
        MUSIC_TIME music_time = entry->head.lBandTimeLogical;
        if (music_time == -1 && !(track_flags & DMUS_TRACKF_START)) continue;
        else if (music_time != -1)
        {
            if (music_time < start_time || music_time >= end_time) continue;
            music_time += time_offset;
        }

        if (FAILED(hr = band_send_messages(entry->band, performance, graph, music_time, track_id)))
            break;
    }

    IDirectMusicGraph_Release(graph);
    return hr;
}

static HRESULT WINAPI band_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        MUSIC_TIME *out_next, void *param)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    struct band_entry *band, *next_band;
    DMUS_BAND_PARAM *bandparam;

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), time, out_next, param);

    if (!type)
        return E_POINTER;
    if (!IsEqualGUID(type, &GUID_BandParam))
        return DMUS_E_GET_UNSUPPORTED;
    if (list_empty(&This->bands))
        return DMUS_E_NOT_FOUND;

    bandparam = param;
    if (out_next) *out_next = 0;

    LIST_FOR_EACH_ENTRY_SAFE(band, next_band, &This->bands, struct band_entry, entry)
    {
        /* we want to return the first band even when there's nothing with lBandTime <= time */
        bandparam->pBand = band->band;
        bandparam->mtTimePhysical = band->head.lBandTimePhysical;
        if (band->entry.next == &This->bands) break;
        if (out_next) *out_next = next_band->head.lBandTimeLogical;
        if (next_band->head.lBandTimeLogical > time) break;
    }

    IDirectMusicBand_AddRef(bandparam->pBand);
    return S_OK;
}

static HRESULT WINAPI band_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (!type)
        return E_POINTER;
    if (FAILED(IDirectMusicTrack8_IsParamSupported(iface, type)))
        return DMUS_E_TYPE_UNSUPPORTED;

    if (IsEqualGUID(type, &GUID_BandParam))
    {
        struct band_entry *new_entry = NULL, *entry, *next_entry;
        DMUS_BAND_PARAM *band_param = param;
        if (!band_param || !band_param->pBand)
            return E_POINTER;
        if (!(new_entry = calloc(1, sizeof(*new_entry))))
            return E_OUTOFMEMORY;
        new_entry->band = band_param->pBand;
        new_entry->head.lBandTimeLogical = time;
        new_entry->head.lBandTimePhysical = band_param->mtTimePhysical;
        IDirectMusicBand_AddRef(new_entry->band);
        if (list_empty(&This->bands))
            list_add_tail(&This->bands, &new_entry->entry);
        else
        {
            LIST_FOR_EACH_ENTRY_SAFE(entry, next_entry, &This->bands, struct band_entry, entry)
                if (entry->entry.next == &This->bands || next_entry->head.lBandTimeLogical > time)
                {
                    list_add_after(&entry->entry, &new_entry->entry);
                    break;
                }
        }
    }
    else if (IsEqualGUID(type, &GUID_Clear_All_Bands))
        FIXME("GUID_Clear_All_Bands not handled yet\n");
    else if (IsEqualGUID(type, &GUID_ConnectToDLSCollection))
    {
        struct band_entry *entry;

        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
            band_connect_to_collection(entry->band, param);
    }
    else if (IsEqualGUID(type, &GUID_Disable_Auto_Download))
        This->header.bAutoDownload = FALSE;
    else if (IsEqualGUID(type, &GUID_Download))
        FIXME("GUID_Download not handled yet\n");
    else if (IsEqualGUID(type, &GUID_DownloadToAudioPath))
    {
        IDirectMusicPerformance *performance;
        IDirectMusicAudioPath *audio_path;
        IUnknown *object = param;
        struct band_entry *entry;
        HRESULT hr;

        if (FAILED(hr = IDirectMusicAudioPath_QueryInterface(object, &IID_IDirectMusicPerformance8, (void **)&performance))
                && SUCCEEDED(hr = IDirectMusicAudioPath_QueryInterface(object, &IID_IDirectMusicAudioPath, (void **)&audio_path)))
        {
            hr = IDirectMusicAudioPath_GetObjectInPath(audio_path, DMUS_PCHANNEL_ALL, DMUS_PATH_PERFORMANCE, 0,
                    &GUID_All_Objects, 0, &IID_IDirectMusicPerformance8, (void **)&performance);
            IDirectMusicAudioPath_Release(audio_path);
        }

        if (FAILED(hr))
        {
            WARN("Failed to get IDirectMusicPerformance from param %p\n", param);
            return hr;
        }

        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
            if (FAILED(hr = IDirectMusicBand_Download(entry->band, performance))) break;

        IDirectMusicPerformance_Release(performance);
    }
    else if (IsEqualGUID(type, &GUID_Enable_Auto_Download))
        This->header.bAutoDownload = TRUE;
    else if (IsEqualGUID(type, &GUID_IDirectMusicBand))
        FIXME("GUID_IDirectMusicBand not handled yet\n");
    else if (IsEqualGUID(type, &GUID_StandardMIDIFile))
        FIXME("GUID_StandardMIDIFile not handled yet\n");
    else if (IsEqualGUID(type, &GUID_UnloadFromAudioPath))
    {
        struct band_entry *entry;
        HRESULT hr;

        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
            if (FAILED(hr = IDirectMusicBand_Unload(entry->band, NULL))) break;
    }

    return S_OK;
}

static HRESULT WINAPI band_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
  struct band_track *This = impl_from_IDirectMusicTrack8(iface);

  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

  if (!rguidType)
    return E_POINTER;

  if (IsEqualGUID (rguidType, &GUID_BandParam)
      || IsEqualGUID (rguidType, &GUID_Clear_All_Bands)
      || IsEqualGUID (rguidType, &GUID_ConnectToDLSCollection)
      || IsEqualGUID (rguidType, &GUID_Disable_Auto_Download)
      || IsEqualGUID (rguidType, &GUID_Download)
      || IsEqualGUID (rguidType, &GUID_DownloadToAudioPath)
      || IsEqualGUID (rguidType, &GUID_Enable_Auto_Download)
      || IsEqualGUID (rguidType, &GUID_IDirectMusicBand)
      || IsEqualGUID (rguidType, &GUID_StandardMIDIFile)
      || IsEqualGUID (rguidType, &GUID_Unload)
      || IsEqualGUID (rguidType, &GUID_UnloadFromAudioPath)) {
    TRACE("param supported\n");
    return S_OK;
  }
  
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI band_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
    return S_OK;
}

static HRESULT WINAPI band_track_PlayEx(IDirectMusicTrack8 *iface, void *state_data,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state,
        DWORD virtual_id)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %lx, %p, %p, %ld): stub\n", This, state_data, wine_dbgstr_longlong(rtStart),
        wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), flags, performance, segment_state, virtual_id);

    return S_OK;
}

static HRESULT WINAPI band_track_GetParamEx(IDirectMusicTrack8 *iface,
        REFGUID rguidType, REFERENCE_TIME rtTime, REFERENCE_TIME *rtNext, void *param,
        void *state_data, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %p, %lx): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), rtNext, param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI band_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *param, void *state_data, DWORD flags)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    FIXME("(%p, %s, 0x%s, %p, %p, %lx): stub\n", This, debugstr_dmguid(rguidType),
        wine_dbgstr_longlong(rtTime), param, state_data, flags);

    return S_OK;
}

static HRESULT WINAPI band_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI band_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
    struct band_track *This = impl_from_IDirectMusicTrack8(iface);
    FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
    return S_OK;
}

static const IDirectMusicTrack8Vtbl band_track_vtbl =
{
    band_track_QueryInterface,
    band_track_AddRef,
    band_track_Release,
    band_track_Init,
    band_track_InitPlay,
    band_track_EndPlay,
    band_track_Play,
    band_track_GetParam,
    band_track_SetParam,
    band_track_IsParamSupported,
    band_track_AddNotificationType,
    band_track_RemoveNotificationType,
    band_track_Clone,
    band_track_PlayEx,
    band_track_GetParamEx,
    band_track_SetParamEx,
    band_track_Compose,
    band_track_Join,
};

static HRESULT parse_lbnd_list(struct band_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    DMUS_IO_BAND_ITEM_HEADER2 header2;
    struct band_entry *entry;
    IDirectMusicBand *band;
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_BANDITEM_CHUNK:
        {
            DMUS_IO_BAND_ITEM_HEADER header;

            if (SUCCEEDED(hr = stream_chunk_get_data(stream, &chunk, &header, sizeof(header))))
            {
                header2.lBandTimeLogical = header.lBandTime;
                header2.lBandTimePhysical = header.lBandTime;
            }

            break;
        }

        case DMUS_FOURCC_BANDITEM_CHUNK2:
            hr = stream_chunk_get_data(stream, &chunk, &header2, sizeof(header2));
            break;

        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_BAND_FORM):
        {
            IPersistStream *persist;

            if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IDirectMusicBand, (void **)&band)))
                break;

            if (SUCCEEDED(hr = IDirectMusicBand_QueryInterface(band, &IID_IPersistStream, (void **)&persist)))
            {
                if (SUCCEEDED(hr = stream_reset_chunk_start(stream, &chunk)))
                    hr = IPersistStream_Load(persist, stream);
                IPersistStream_Release(persist);
            }

            break;
        }

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) return hr;

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    entry->head = header2;
    entry->band = band;
    IDirectMusicBand_AddRef(band);
    list_add_tail(&This->bands, &entry->entry);

    return S_OK;
}

static HRESULT parse_lbdl_list(struct band_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_BAND_LIST):
            hr = parse_lbnd_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return S_OK;
}

static HRESULT parse_dmbt_chunk(struct band_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &This->dmobj.desc,
                DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_VERSION))
                || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_GUID_CHUNK:
        case DMUS_FOURCC_VERSION_CHUNK:
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case DMUS_FOURCC_BANDTRACK_CHUNK:
            hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header));
            break;

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_BANDS_LIST):
            hr = parse_lbdl_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static inline struct band_track *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct band_track, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI band_track_persist_stream_Load(IPersistStream *iface, IStream *stream)
{
    struct band_track *This = impl_from_IPersistStream(iface);
    struct chunk_entry chunk = {0};
    HRESULT hr;

    TRACE("(%p, %p)\n", This, stream);

    if ((hr = stream_get_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_BANDTRACK_FORM):
            hr = parse_dmbt_chunk(This, stream, &chunk);
            break;

        default:
            WARN("Invalid band track chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            hr = DMUS_E_UNSUPPORTED_STREAM;
            break;
        }
    }

    stream_skip_chunk(stream, &chunk);
    if (FAILED(hr)) return hr;

    if (TRACE_ON(dmband))
    {
        struct band_entry *entry;
        int i = 0;

        TRACE("Loaded DirectMusicBandTrack %p\n", This);
        dump_DMUS_OBJECTDESC(&This->dmobj.desc);

        TRACE(" - header:\n");
        TRACE("    - bAutoDownload: %u\n", This->header.bAutoDownload);

        TRACE(" - bands:\n");
        LIST_FOR_EACH_ENTRY(entry, &This->bands, struct band_entry, entry)
        {
            TRACE("    - band[%u]: %p\n", i++, entry->band);
            TRACE("        - lBandTimeLogical: %ld\n", entry->head.lBandTimeLogical);
            TRACE("        - lBandTimePhysical: %ld\n", entry->head.lBandTimePhysical);
        }
    }

    return S_OK;
}

static const IPersistStreamVtbl band_track_persist_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    band_track_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

/* for ClassFactory */
HRESULT create_dmbandtrack(REFIID lpcGUID, void **ppobj)
{
    struct band_track *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &band_track_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicBandTrack, (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &band_track_persist_stream_vtbl;
    list_init(&track->bands);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
