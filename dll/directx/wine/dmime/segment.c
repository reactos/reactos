/* IDirectMusicSegment8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
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

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct track_entry
{
    struct list entry;
    DWORD dwGroupBits;
    DWORD flags;
    IDirectMusicTrack *pTrack;
};

static void track_entry_destroy(struct track_entry *entry)
{
    HRESULT hr;

    if (FAILED(hr = IDirectMusicTrack_Init(entry->pTrack, NULL)))
        WARN("Failed to de-init track %p, hr %#lx\n", entry->pTrack, hr);
    IDirectMusicTrack_Release(entry->pTrack);

    free(entry);
}

struct segment
{
    IDirectMusicSegment8 IDirectMusicSegment8_iface;
    struct dmobject dmobj;
    LONG ref;
    DMUS_IO_SEGMENT_HEADER header;
    IDirectMusicGraph *pGraph;
    struct list tracks;
};

static struct segment *segment_create(void);

static inline struct segment *impl_from_IDirectMusicSegment8(IDirectMusicSegment8 *iface)
{
    return CONTAINING_RECORD(iface, struct segment, IDirectMusicSegment8_iface);
}

static HRESULT WINAPI segment_QueryInterface(IDirectMusicSegment8 *iface, REFIID riid, void **ret_iface)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDirectMusicSegment) ||
            IsEqualIID(riid, &IID_IDirectMusicSegment2) ||
            IsEqualIID (riid, &IID_IDirectMusicSegment8))
        *ret_iface = iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI segment_AddRef(IDirectMusicSegment8 *iface)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI segment_Release(IDirectMusicSegment8 *iface)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct track_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tracks, struct track_entry, entry)
        {
            list_remove(&entry->entry);
            track_entry_destroy(entry);
        }
        free(This);
    }

    return ref;
}

static HRESULT WINAPI segment_GetLength(IDirectMusicSegment8 *iface, MUSIC_TIME *length)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %p)\n", This, length);

    if (!length) return E_POINTER;
    *length = This->header.mtLength;

    return S_OK;
}

static HRESULT WINAPI segment_SetLength(IDirectMusicSegment8 *iface, MUSIC_TIME length)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld)\n", This, length);

    This->header.mtLength = length;

    return S_OK;
}

static HRESULT WINAPI segment_GetRepeats(IDirectMusicSegment8 *iface, DWORD *repeats)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %p)\n", This, repeats);

    if (!repeats) return E_POINTER;
    *repeats = This->header.dwRepeats;

    return S_OK;
}

static HRESULT WINAPI segment_SetRepeats(IDirectMusicSegment8 *iface, DWORD repeats)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld)\n", This, repeats);
    This->header.dwRepeats = repeats;

    return S_OK;
}

static HRESULT WINAPI segment_GetDefaultResolution(IDirectMusicSegment8 *iface, DWORD *resolution)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %p)\n", This, resolution);

    if (!resolution) return E_POINTER;
    *resolution = This->header.dwResolution;

    return S_OK;
}

static HRESULT WINAPI segment_SetDefaultResolution(IDirectMusicSegment8 *iface, DWORD resolution)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld)\n", This, resolution);
    This->header.dwResolution = resolution;

    return S_OK;
}

static HRESULT WINAPI segment_GetTrack(IDirectMusicSegment8 *iface, REFGUID type, DWORD group,
        DWORD index, IDirectMusicTrack **ret_track)
{
  struct segment *This = impl_from_IDirectMusicSegment8(iface);
  struct track_entry *entry;
  HRESULT hr = S_OK;

  TRACE("(%p, %s, %#lx, %#lx, %p)\n", This, debugstr_dmguid(type), group, index, ret_track);

  if (!ret_track) return E_POINTER;

  LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
  {
      if (group != -1 && !(entry->dwGroupBits & group)) continue;

      if (!IsEqualGUID(&GUID_NULL, type))
      {
          CLSID entry_type = GUID_NULL;
          IPersistStream *persist;

          if (SUCCEEDED(hr = IDirectMusicTrack_QueryInterface(entry->pTrack, &IID_IPersistStream, (void **)&persist)))
          {
              hr = IPersistStream_GetClassID(persist, &entry_type);
              if (SUCCEEDED(hr)) TRACE(" - %p -> %s\n", entry, debugstr_dmguid(&entry_type));
              IPersistStream_Release(persist);
          }

          if (!IsEqualGUID(&entry_type, type)) continue;
      }

      if (!index--)
      {
          *ret_track = entry->pTrack;
          IDirectMusicTrack_AddRef(entry->pTrack);
          return S_OK;
      }
  }

  return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI segment_GetTrackGroup(IDirectMusicSegment8 *iface, IDirectMusicTrack *track, DWORD *ret_group)
{
  struct segment *This = impl_from_IDirectMusicSegment8(iface);
  struct track_entry *entry;

  TRACE("(%p, %p, %p)\n", This, track, ret_group);

  if (!ret_group) return E_POINTER;

  LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
  {
      if (entry->pTrack == track)
      {
          *ret_group = entry->dwGroupBits;
          return S_OK;
      }
  }

  return DMUS_E_NOT_FOUND;
}

static HRESULT segment_append_track(struct segment *This, IDirectMusicTrack *track, DWORD group, DWORD flags)
{
    struct track_entry *entry;
    HRESULT hr;

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    entry->dwGroupBits = group;
    entry->flags = flags;
    entry->pTrack = track;
    IDirectMusicTrack_AddRef(track);

    hr = IDirectMusicTrack_Init(track, (IDirectMusicSegment *)&This->IDirectMusicSegment8_iface);
    if (FAILED(hr)) track_entry_destroy(entry);
    else list_add_tail(&This->tracks, &entry->entry);

    return hr;
}

static HRESULT WINAPI segment_InsertTrack(IDirectMusicSegment8 *iface, IDirectMusicTrack *track, DWORD group)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    struct track_entry *entry;

    TRACE("(%p, %p, %#lx)\n", This, track, group);

    if (!group) return E_INVALIDARG;

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
        if (entry->pTrack == track) return E_FAIL;

    return segment_append_track(This, track, group, 0);
}

static HRESULT WINAPI segment_RemoveTrack(IDirectMusicSegment8 *iface, IDirectMusicTrack *track)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    struct track_entry *entry;

    TRACE("(%p, %p)\n", This, track);

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (entry->pTrack == track)
        {
            list_remove(&entry->entry);
            track_entry_destroy(entry);
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI segment_InitPlay(IDirectMusicSegment8 *iface,
        IDirectMusicSegmentState **state, IDirectMusicPerformance *performance, DWORD flags)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    HRESULT hr;

    FIXME("(%p, %p, %p, %ld): semi-stub\n", This, state, performance, flags);

    if (!state) return E_POINTER;
    if (FAILED(hr = create_dmsegmentstate(&IID_IDirectMusicSegmentState, (void **)state))) return hr;

    /* TODO: DMUS_SEGF_FLAGS */
    return S_OK;
}

static HRESULT WINAPI segment_GetGraph(IDirectMusicSegment8 *iface, IDirectMusicGraph **graph)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    FIXME("(%p, %p): semi-stub\n", This, graph);

    if (!graph) return E_POINTER;
    if (!(*graph = This->pGraph)) return DMUS_E_NOT_FOUND;
    IDirectMusicGraph_AddRef(This->pGraph);

    return S_OK;
}

static HRESULT WINAPI segment_SetGraph(IDirectMusicSegment8 *iface, IDirectMusicGraph *graph)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    FIXME("(%p, %p): to complete\n", This, graph);

    if (This->pGraph) IDirectMusicGraph_Release(This->pGraph);
    if ((This->pGraph = graph)) IDirectMusicGraph_AddRef(This->pGraph);

    return S_OK;
}

static HRESULT WINAPI segment_AddNotificationType(IDirectMusicSegment8 *iface, REFGUID rguidNotificationType)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
    return S_OK;
}

static HRESULT WINAPI segment_RemoveNotificationType(IDirectMusicSegment8 *iface, REFGUID rguidNotificationType)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
    return S_OK;
}

static HRESULT WINAPI segment_GetParam(IDirectMusicSegment8 *iface, REFGUID type, DWORD group,
        DWORD index, MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    IDirectMusicTrack *track;
    unsigned int i, count;
    HRESULT hr = DMUS_E_TRACK_NOT_FOUND;

    TRACE("(%p, %s, %#lx, %lu, %ld, %p, %p)\n", This, debugstr_dmguid(type), group, index, time,
            next, param);

    if (!type)
        return E_POINTER;

    /* Index is relative to the search pattern: group bits and supported param type */
    for (i = 0, count = 0; i < DMUS_SEG_ANYTRACK && count <= index; i++) {
        if (FAILED(segment_GetTrack(iface, &GUID_NULL, group, i, &track))) break;
        if (FAILED(IDirectMusicTrack_IsParamSupported(track, type)))
        {
            IDirectMusicTrack_Release(track);
            continue;
        }
        if (index == count || index == DMUS_SEG_ANYTRACK)
            hr = IDirectMusicTrack_GetParam(track, type, time, next, param);
        IDirectMusicTrack_Release(track);

        if (SUCCEEDED(hr))
            return hr;
        count++;
    }

    TRACE("(%p): not found\n", This);

    return hr;
}

static HRESULT WINAPI segment_SetParam(IDirectMusicSegment8 *iface, REFGUID type,
        DWORD group, DWORD index, MUSIC_TIME music_time, void *param)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    struct track_entry *entry;
    HRESULT hr;

    TRACE("(%p, %s, %#lx, %ld, %ld, %p)\n", This, debugstr_dmguid(type), group,
            index, music_time, param);

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (group != -1)
        {
            if (!(group & entry->dwGroupBits)) continue;
            if (index != DMUS_SEG_ALLTRACKS && index--) continue;
        }

        if (SUCCEEDED(hr = IDirectMusicTrack_IsParamSupported(entry->pTrack, type))
                && FAILED(hr = IDirectMusicTrack_SetParam(entry->pTrack, type, music_time, param)))
            WARN("SetParam for track %p failed, hr %#lx\n", entry->pTrack, hr);
    }

    return S_OK;
}

static HRESULT WINAPI segment_Clone(IDirectMusicSegment8 *iface, MUSIC_TIME start, MUSIC_TIME end,
        IDirectMusicSegment **segment)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    struct segment *clone;
    IDirectMusicTrack *track;
    struct track_entry *entry;
    HRESULT hr = S_OK;

    TRACE("(%p, %ld, %ld, %p)\n", This, start, end, segment);

    if (!segment) return E_POINTER;

    if (!(clone = segment_create()))
    {
        *segment = NULL;
        return E_OUTOFMEMORY;
    }

    clone->header = This->header;
    if ((clone->pGraph = This->pGraph)) IDirectMusicGraph_AddRef(clone->pGraph);

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (FAILED(hr = IDirectMusicTrack_Clone(entry->pTrack, start, end, &track))) break;
        hr = segment_append_track(clone, track, entry->dwGroupBits, entry->flags);
        IDirectMusicTrack_Release(track);
        if (FAILED(hr)) break;
    }

    *segment = (IDirectMusicSegment *)&clone->IDirectMusicSegment8_iface;
    return FAILED(hr) ? S_FALSE : S_OK;
}

static HRESULT WINAPI segment_SetStartPoint(IDirectMusicSegment8 *iface, MUSIC_TIME start)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld)\n", This, start);

    if (start >= This->header.mtLength) return DMUS_E_OUT_OF_RANGE;
    This->header.mtPlayStart = start;

    return S_OK;
}

static HRESULT WINAPI segment_GetStartPoint(IDirectMusicSegment8 *iface, MUSIC_TIME *start)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %p)\n", This, start);
    if (!start) return E_POINTER;
    *start = This->header.mtPlayStart;

    return S_OK;
}

static HRESULT WINAPI segment_SetLoopPoints(IDirectMusicSegment8 *iface, MUSIC_TIME start, MUSIC_TIME end)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %ld, %ld)\n", This, start, end);

    if ((end || start) && (start >= This->header.mtLength || end > This->header.mtLength || start > end))
        return DMUS_E_OUT_OF_RANGE;
    This->header.mtLoopStart = start;
    This->header.mtLoopEnd = end;

    return S_OK;
}

static HRESULT WINAPI segment_GetLoopPoints(IDirectMusicSegment8 *iface, MUSIC_TIME *start, MUSIC_TIME *end)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);

    TRACE("(%p, %p, %p)\n", This, start, end);

    if (!start || !end) return E_POINTER;
    *start = This->header.mtLoopStart;
    *end = This->header.mtLoopEnd;

    return S_OK;
}

static HRESULT WINAPI segment_SetPChannelsUsed(IDirectMusicSegment8 *iface, DWORD dwNumPChannels, DWORD *paPChannels)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %ld, %p): stub\n", This, dwNumPChannels, paPChannels);
    return S_OK;
}

static HRESULT WINAPI segment_SetTrackConfig(IDirectMusicSegment8 *iface, REFGUID rguidTrackClassID,
        DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %s, %#lx, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID),
            dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
    return S_OK;
}

static HRESULT WINAPI segment_GetAudioPathConfig(IDirectMusicSegment8 *iface, IUnknown **ppAudioPathConfig)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %p): stub\n", This, ppAudioPathConfig);
    return S_OK;
}

static HRESULT WINAPI segment_Compose(IDirectMusicSegment8 *iface, MUSIC_TIME mtTime,
        IDirectMusicSegment *pFromSegment, IDirectMusicSegment *pToSegment, IDirectMusicSegment **ppComposedSegment)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    FIXME("(%p, %ld, %p, %p, %p): stub\n", This, mtTime, pFromSegment, pToSegment, ppComposedSegment);
    return S_OK;
}

static HRESULT WINAPI segment_Download(IDirectMusicSegment8 *iface, IUnknown *audio_path)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    TRACE("(%p, %p)\n", This, audio_path);
    return IDirectMusicSegment8_SetParam(iface, &GUID_DownloadToAudioPath, -1, DMUS_SEG_ALLTRACKS, 0, audio_path);
}

static HRESULT WINAPI segment_Unload(IDirectMusicSegment8 *iface, IUnknown *audio_path)
{
    struct segment *This = impl_from_IDirectMusicSegment8(iface);
    TRACE("(%p, %p)\n", This, audio_path);
    return IDirectMusicSegment8_SetParam(iface, &GUID_UnloadFromAudioPath, -1, DMUS_SEG_ALLTRACKS, 0, audio_path);
}

static const IDirectMusicSegment8Vtbl segment_vtbl =
{
    segment_QueryInterface,
    segment_AddRef,
    segment_Release,
    segment_GetLength,
    segment_SetLength,
    segment_GetRepeats,
    segment_SetRepeats,
    segment_GetDefaultResolution,
    segment_SetDefaultResolution,
    segment_GetTrack,
    segment_GetTrackGroup,
    segment_InsertTrack,
    segment_RemoveTrack,
    segment_InitPlay,
    segment_GetGraph,
    segment_SetGraph,
    segment_AddNotificationType,
    segment_RemoveNotificationType,
    segment_GetParam,
    segment_SetParam,
    segment_Clone,
    segment_SetStartPoint,
    segment_GetStartPoint,
    segment_SetLoopPoints,
    segment_GetLoopPoints,
    segment_SetPChannelsUsed,
    segment_SetTrackConfig,
    segment_GetAudioPathConfig,
    segment_Compose,
    segment_Download,
    segment_Unload,
};

static HRESULT WINAPI segment_object_ParseDescriptor(IDirectMusicObject *iface, IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    DWORD supported = DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || !(riff.type == DMUS_FOURCC_SEGMENT_FORM ||
                riff.type == mmioFOURCC('W','A','V','E'))) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return E_FAIL;
    }

    if (riff.type == DMUS_FOURCC_SEGMENT_FORM)
        supported |= DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY;
    else
        supported |= DMUS_OBJ_NAME_INFO;
    hr = dmobj_parsedescriptor(stream, &riff, desc, supported);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicSegment;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl segment_object_vtbl =
{
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    segment_object_ParseDescriptor,
};

static HRESULT parse_track_form(struct segment *This, IStream *stream, const struct chunk_entry *riff)
{
    struct chunk_entry chunk = {.parent = riff};
    IDirectMusicTrack *track = NULL;
    IPersistStream *ps = NULL;
    IStream *clone;
    DMUS_IO_TRACK_HEADER thdr;
    DMUS_IO_TRACK_EXTRAS_HEADER txhdr = {0};
    HRESULT hr;

    TRACE("Parsing track form in %p: %s\n", stream, debugstr_chunk(riff));

    /* First chunk must be the track header */
    if (FAILED(hr = stream_get_chunk(stream, &chunk)))
        return hr;
    if (chunk.id != DMUS_FOURCC_TRACK_CHUNK)
        return DMUS_E_TRACK_HDR_NOT_FIRST_CK;
    if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &thdr, sizeof(thdr))))
        return hr;
    TRACE("Found DMUS_IO_TRACK_HEADER\n");
    TRACE("\tclass: %s\n", debugstr_guid (&thdr.guidClassID));
    TRACE("\tdwGroup: %#lx\n", thdr.dwGroup);
    TRACE("\tckid: %s\n", debugstr_fourcc (thdr.ckid));
    TRACE("\tfccType: %s\n", debugstr_fourcc (thdr.fccType));

    if (!!thdr.ckid == !!thdr.fccType) {
        WARN("One and only one of the ckid (%s) and fccType (%s) need to be set\n",
                debugstr_fourcc(thdr.ckid), debugstr_fourcc(thdr.fccType));
        return DMUS_E_INVALID_TRACK_HDR;
    }

    /* Optional chunks */
    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        if ((thdr.ckid && chunk.id == thdr.ckid) ||
                (!thdr.ckid && (chunk.id == FOURCC_LIST || chunk.id == FOURCC_RIFF) &&
                 chunk.type == thdr.fccType))
            break;

        if (chunk.id == DMUS_FOURCC_TRACK_EXTRAS_CHUNK &&
                SUCCEEDED(stream_chunk_get_data(stream, &chunk, &txhdr, sizeof(txhdr)))) {
            FIXME("DMUS_IO_TRACK_EXTRAS_HEADER chunk not fully handled\n");
            TRACE("dwFlags: %#lx, dwPriority: %lu\n", txhdr.dwFlags, txhdr.dwPriority);
        }
    }
    if (hr != S_OK)
        return hr == S_FALSE ? DMUS_E_TRACK_NOT_FOUND : hr;

    /* Some DirectMusicTrack implementation expect the stream to start with their data chunk */
    if (FAILED(hr = IStream_Clone(stream, &clone)))
        return hr;
    stream_reset_chunk_start(clone, &chunk);

    /* Load the track */
    hr = CoCreateInstance(&thdr.guidClassID, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicTrack,
            (void **)&track);
    if (FAILED(hr))
        goto done;
    hr = IDirectMusicTrack_QueryInterface(track, &IID_IPersistStream, (void **)&ps);
    if (FAILED(hr))
        goto done;
    hr = IPersistStream_Load(ps, clone);
    if (FAILED(hr))
        goto done;

    hr = segment_append_track(This, track, thdr.dwGroup, txhdr.dwFlags);

done:
    if (ps)
        IPersistStream_Release(ps);
    if (track)
        IDirectMusicTrack_Release(track);
    IStream_Release(clone);

    return hr;
}

static HRESULT parse_track_list(struct segment *This, IStream *stream, const struct chunk_entry *trkl)
{
    struct chunk_entry chunk = {.parent = trkl};
    HRESULT hr;

    TRACE("Parsing track list in %p: %s\n", stream, debugstr_chunk(trkl));

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_RIFF && chunk.type == DMUS_FOURCC_TRACK_FORM)
            hr = parse_track_form(This, stream, &chunk);

    return SUCCEEDED(hr) ? S_OK : hr;
}

static inline void dump_segment_header(DMUS_IO_SEGMENT_HEADER *h, DWORD size)
{
    unsigned int dx = 9;

    if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLength))
        dx = 7;
    else if (size == offsetof(DMUS_IO_SEGMENT_HEADER, rtLoopStart))
        dx = 8;
    TRACE("Found DirectX%d DMUS_IO_SEGMENT_HEADER\n", dx);
    TRACE("\tdwRepeats: %lu\n", h->dwRepeats);
    TRACE("\tmtLength: %lu\n", h->mtLength);
    TRACE("\tmtPlayStart: %lu\n", h->mtPlayStart);
    TRACE("\tmtLoopStart: %lu\n", h->mtLoopStart);
    TRACE("\tmtLoopEnd: %lu\n", h->mtLoopEnd);
    TRACE("\tdwResolution: %lu\n", h->dwResolution);
    if (dx >= 8) {
        TRACE("\trtLength: %s\n", wine_dbgstr_longlong(h->rtLength));
        TRACE("\tdwFlags: %lu\n", h->dwFlags);
        TRACE("\tdwReserved: %lu\n", h->dwReserved);
    }
    if (dx == 9) {
        TRACE("\trtLoopStart: %s\n", wine_dbgstr_longlong(h->rtLoopStart));
        TRACE("\trtLoopEnd: %s\n", wine_dbgstr_longlong(h->rtLoopEnd));
        if (size > offsetof(DMUS_IO_SEGMENT_HEADER, rtPlayStart))
            TRACE("\trtPlayStart: %s\n", wine_dbgstr_longlong(h->rtPlayStart));
    }
}

static HRESULT parse_dmsg_chunk(struct segment *This, IStream *stream, const struct chunk_entry *riff)
{
    struct chunk_entry chunk = {.parent = riff};
    HRESULT hr;

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(riff));

    if (FAILED(hr = dmobj_parsedescriptor(stream, riff, &This->dmobj.desc, DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY))
                || FAILED(hr = stream_reset_chunk_data(stream, riff)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        switch (chunk.id) {
            case DMUS_FOURCC_SEGMENT_CHUNK:
                /* DX9 without rtPlayStart field */
                if (chunk.size == offsetof(DMUS_IO_SEGMENT_HEADER, rtPlayStart))
                    WARN("Missing rtPlayStart field in %s\n", debugstr_chunk(&chunk));
                /* DX7, DX8 and DX9 structure sizes */
                else if (chunk.size != offsetof(DMUS_IO_SEGMENT_HEADER, rtLength) &&
                        chunk.size != offsetof(DMUS_IO_SEGMENT_HEADER, rtLoopStart) &&
                        chunk.size != sizeof(DMUS_IO_SEGMENT_HEADER)) {
                    WARN("Invalid size of %s\n", debugstr_chunk(&chunk));
                    break;
                }
                if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &This->header, chunk.size))) {
                    WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
                    return hr;
                }
                dump_segment_header(&This->header, chunk.size);
                break;
            case FOURCC_LIST:
                if (chunk.type == DMUS_FOURCC_TRACK_LIST)
                    if (FAILED(hr = parse_track_list(This, stream, &chunk)))
                        return hr;
                break;
            case FOURCC_RIFF:
                FIXME("Loading of embedded RIFF form %s\n", debugstr_fourcc(chunk.type));
                break;
        }
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

static inline struct segment *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct segment, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI segment_persist_stream_Load(IPersistStream *iface, IStream *stream)
{
    struct segment *This = impl_from_IPersistStream(iface);
    struct chunk_entry chunk = {0};
    HRESULT hr;

    TRACE("(%p, %p): Loading\n", This, stream);

    if (!stream) return E_POINTER;

    if ((hr = stream_get_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_SEGMENT_FORM):
            hr = parse_dmsg_chunk(This, stream, &chunk);
            break;

        case mmioFOURCC('M','T','h','d'):
            hr = parse_midi(stream, &This->IDirectMusicSegment8_iface);
            break;

        case MAKE_IDTYPE(FOURCC_RIFF, mmioFOURCC('W','A','V','E')):
        {
            IDirectMusicTrack8 *track;

            TRACE("Loading segment %p from wave file\n", This);

            This->header.mtLength = 1;
            if (FAILED(hr = wave_track_create_from_chunk(stream, &chunk, &track))) break;
            hr = segment_append_track(This, (IDirectMusicTrack *)track, 1, 0);
            IDirectMusicTrack8_Release(track);
            break;
        }

        default:
            WARN("Invalid segment chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            hr = DMUS_E_UNSUPPORTED_STREAM;
            break;
        }
    }

    if (chunk.id != mmioFOURCC('M', 'T', 'h', 'd'))
        stream_skip_chunk(stream, &chunk);

    if (FAILED(hr))
    {
        WARN("Failed to load segment from stream %p, hr %#lx\n", stream, hr);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    This->dmobj.desc.guidClass = CLSID_DirectMusicSegment;
    This->dmobj.desc.dwValidData |= DMUS_OBJ_CLASS;

    return S_OK;
}

static const IPersistStreamVtbl segment_persist_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    segment_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

static struct segment *segment_create(void)
{
    struct segment *obj;

    if (!(obj = calloc(1, sizeof(*obj)))) return NULL;
    obj->IDirectMusicSegment8_iface.lpVtbl = &segment_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicSegment, (IUnknown *)&obj->IDirectMusicSegment8_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &segment_object_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &segment_persist_stream_vtbl;
    list_init(&obj->tracks);

    return obj;
}

/* for ClassFactory */
HRESULT create_dmsegment(REFIID guid, void **ret_iface)
{
    struct segment *obj;
    HRESULT hr;

    if (!(obj = segment_create()))
    {
        *ret_iface = NULL;
        return E_OUTOFMEMORY;
    }

    hr = IDirectMusicSegment8_QueryInterface(&obj->IDirectMusicSegment8_iface, guid, ret_iface);
    IDirectMusicSegment8_Release(&obj->IDirectMusicSegment8_iface);

    return hr;
}
