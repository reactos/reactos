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

#include "dmusic_midi.h"
#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct sequence_track
{
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;

    DMUS_IO_SEQ_ITEM *items;
    unsigned int count;

    DMUS_IO_CURVE_ITEM *curve_items;
    unsigned int curve_count;
};

static inline struct sequence_track *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, struct sequence_track, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI sequence_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI sequence_track_AddRef(IDirectMusicTrack8 *iface)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI sequence_track_Release(IDirectMusicTrack8 *iface)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI sequence_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI sequence_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI sequence_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI sequence_track_Play(IDirectMusicTrack8 *iface, void *state_data,
        MUSIC_TIME start_time, MUSIC_TIME end_time, MUSIC_TIME time_offset, DWORD segment_flags,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState *segment_state, DWORD track_id)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
    IDirectMusicGraph *graph;
    HRESULT hr;
    UINT i;

    TRACE("(%p, %p, %ld, %ld, %ld, %#lx, %p, %p, %ld)\n", This, state_data, start_time, end_time,
            time_offset, segment_flags, performance, segment_state, track_id);

    if (segment_flags) FIXME("segment_flags %#lx not implemented\n", segment_flags);
    if (segment_state) FIXME("segment_state %p not implemented\n", segment_state);

    if (FAILED(hr = IDirectMusicPerformance_QueryInterface(performance,
            &IID_IDirectMusicGraph, (void **)&graph)))
        return hr;

    for (i = 0; SUCCEEDED(hr) &&i < This->count; i++)
    {
        DMUS_IO_SEQ_ITEM *item = This->items + i;
        DMUS_PMSG *msg;

        if (item->mtTime < start_time) continue;
        if (item->mtTime >= end_time) continue;

        if (item->bStatus == MIDI_NOTE_ON)
        {
            DMUS_NOTE_PMSG *note;
            if (FAILED(hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(*note),
                    (DMUS_PMSG **)&note)))
                break;

            note->dwType = DMUS_PMSGT_NOTE;
            note->mtDuration = item->mtDuration;
            note->wMusicValue = item->bByte1;
            note->nOffset = item->nOffset;
            note->bVelocity = item->bByte2;
            note->bFlags = 1;
            note->bMidiValue = item->bByte1;
            msg = (DMUS_PMSG *)note;
        }
        else
        {
            DMUS_MIDI_PMSG *midi;
            if (FAILED(hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(*midi),
                    (DMUS_PMSG **)&midi)))
                break;

            midi->dwType = DMUS_PMSGT_MIDI;
            midi->bStatus = item->bStatus;
            midi->bByte1 = item->bByte1;
            midi->bByte2 = item->bByte2;
            msg = (DMUS_PMSG *)midi;
        }

        msg->mtTime = item->mtTime + time_offset;
        msg->dwFlags = DMUS_PMSGF_MUSICTIME;
        msg->dwPChannel = item->dwPChannel;
        msg->dwVirtualTrackID = track_id;
        msg->dwGroupID = 1;

        if (FAILED(hr = IDirectMusicGraph_StampPMsg(graph, (DMUS_PMSG *)msg))
                || FAILED(hr = IDirectMusicPerformance_SendPMsg(performance, (DMUS_PMSG *)msg)))
        {
            IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)msg);
            break;
        }
    }

    for (i = 0; SUCCEEDED(hr) &&i < This->curve_count; i++)
    {
        DMUS_IO_CURVE_ITEM *item = This->curve_items + i;
        DMUS_CURVE_PMSG *msg;

        if (item->mtStart < start_time) continue;
        if (item->mtStart >= end_time) continue;

        if (FAILED(hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(*msg),
                (DMUS_PMSG **)&msg)))
            break;

        msg->mtTime = item->mtStart + time_offset;
        msg->dwFlags = DMUS_PMSGF_MUSICTIME;
        msg->dwPChannel = item->dwPChannel;
        msg->dwVirtualTrackID = track_id;
        msg->dwType = DMUS_PMSGT_CURVE;
        msg->mtDuration = item->mtDuration;
        msg->mtOriginalStart = item->mtStart;
        msg->mtResetDuration = item->mtResetDuration;
        msg->nStartValue = item->nStartValue;
        msg->nEndValue = item->nEndValue;
        msg->nResetValue = item->nResetValue;
        msg->nOffset = item->nOffset;
        msg->bType = item->bType;
        msg->bCurveShape = item->bCurveShape;
        msg->bCCData = item->bCCData;
        msg->bFlags = item->bFlags;
        msg->wParamType = item->wParamType;
        msg->wMergeIndex = item->wMergeIndex;

        if (FAILED(hr = IDirectMusicGraph_StampPMsg(graph, (DMUS_PMSG *)msg))
                || FAILED(hr = IDirectMusicPerformance_SendPMsg(performance, (DMUS_PMSG *)msg)))
        {
            IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)msg);
            break;
        }
    }

    IDirectMusicGraph_Release(graph);
    return hr;
}

static HRESULT WINAPI sequence_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): method not implemented\n", This, debugstr_dmguid(type), time,
            next, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p): method not implemented\n", This, debugstr_dmguid(type), time, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI sequence_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI sequence_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, REFERENCE_TIME *next, void *param, void *state, DWORD flags)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %p, %lx): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), next, param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, void *param, void *state, DWORD flags)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %lx): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI sequence_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *newtrack,
        MUSIC_TIME join, IUnknown *context, DWORD trackgroup, IDirectMusicTrack **resulttrack)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %ld, %p, %ld, %p): method not implemented\n", This, newtrack, join, context,
            trackgroup, resulttrack);
    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    sequence_track_QueryInterface,
    sequence_track_AddRef,
    sequence_track_Release,
    sequence_track_Init,
    sequence_track_InitPlay,
    sequence_track_EndPlay,
    sequence_track_Play,
    sequence_track_GetParam,
    sequence_track_SetParam,
    sequence_track_IsParamSupported,
    sequence_track_AddNotificationType,
    sequence_track_RemoveNotificationType,
    sequence_track_Clone,
    sequence_track_PlayEx,
    sequence_track_GetParamEx,
    sequence_track_SetParamEx,
    sequence_track_Compose,
    sequence_track_Join
};

static HRESULT parse_curl_list(struct sequence_track *This, IStream *stream, struct chunk_entry *chunk)
{
    HRESULT hr;
    UINT i;

    if (FAILED(hr = stream_chunk_get_array(stream, chunk, (void **)&This->curve_items,
            &This->curve_count, sizeof(*This->curve_items))))
    {
        /* try again with the older DMUS_IO_CURVE_ITEM size */
        UINT size = offsetof(DMUS_IO_CURVE_ITEM, wParamType);
        BYTE *buffer;

        if (FAILED(hr = stream_reset_chunk_data(stream, chunk))) return hr;
        if (FAILED(hr = stream_chunk_get_array(stream, chunk, (void **)&buffer,
                &This->curve_count, size)))
            return hr;

        if (!(This->curve_items = calloc(This->curve_count, sizeof(*This->curve_items)))) return E_OUTOFMEMORY;
        for (i = 0; i < This->curve_count; i++) memcpy(This->curve_items + i, buffer + size * i, size);
        free(buffer);
    }

    return S_OK;
}

static HRESULT parse_seqt_chunk(struct sequence_track *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_SEQ_LIST:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&This->items,
                    &This->count, sizeof(*This->items));
            break;

        case DMUS_FOURCC_CURVE_LIST:
            hr = parse_curl_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static inline struct sequence_track *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct sequence_track, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI track_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    struct sequence_track *This = impl_from_IPersistStream(iface);
    struct chunk_entry chunk = {0};
    HRESULT hr;

    TRACE("(%p, %p)\n", This, stream);

    if ((hr = stream_get_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_SEQ_TRACK:
            hr = parse_seqt_chunk(This, stream, &chunk);
            break;

        default:
            WARN("Invalid seq track chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            hr = DMUS_E_UNSUPPORTED_STREAM;
            break;
        }
    }

    stream_skip_chunk(stream, &chunk);
    if (FAILED(hr)) return hr;

    if (TRACE_ON(dmime))
    {
        UINT i;

        TRACE("Loaded DirectMusicSeqTrack %p\n", This);

        TRACE("- %u items:\n", This->count);
        for (i = 0; i < This->count; i++)
        {
            TRACE("    - DMUS_IO_SEQ_ITEM[%u]\n", i);
            TRACE("      - mtTime: %ld\n", This->items[i].mtTime);
            TRACE("      - mtDuration: %ld\n", This->items[i].mtDuration);
            TRACE("      - dwPChannel: %ld\n", This->items[i].dwPChannel);
            TRACE("      - nOffset: %d\n", This->items[i].nOffset);
            TRACE("      - bStatus: %d\n", This->items[i].bStatus);
            TRACE("      - bByte1: %#x\n", This->items[i].bByte1);
            TRACE("      - bByte2: %#x\n", This->items[i].bByte2);
        }

        TRACE("- %u curves:\n", This->curve_count);
        for (i = 0; i < This->curve_count; i++)
        {
            TRACE("    - DMUS_IO_CURVE_ITEM[%u]\n", i);
            TRACE("      - mtStart: %ld\n", This->curve_items[i].mtStart);
            TRACE("      - mtDuration: %ld\n", This->curve_items[i].mtDuration);
            TRACE("      - mtResetDuration: %ld\n", This->curve_items[i].mtResetDuration);
            TRACE("      - dwPChannel: %ld\n", This->curve_items[i].dwPChannel);
            TRACE("      - nOffset: %d\n", This->curve_items[i].nOffset);
            TRACE("      - nStartValue: %d\n", This->curve_items[i].nStartValue);
            TRACE("      - nEndValue: %d\n", This->curve_items[i].nEndValue);
            TRACE("      - nResetValue: %d\n", This->curve_items[i].nResetValue);
            TRACE("      - bType: %d\n", This->curve_items[i].bType);
            TRACE("      - bCurveShape: %d\n", This->curve_items[i].bCurveShape);
            TRACE("      - bCCData: %d\n", This->curve_items[i].bCCData);
            TRACE("      - bFlags: %d\n", This->curve_items[i].bFlags);
            TRACE("      - wParamType: %d\n", This->curve_items[i].wParamType);
            TRACE("      - wMergeIndex: %d\n", This->curve_items[i].wMergeIndex);
        }
    }

    return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    track_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmseqtrack(REFIID lpcGUID, void **ppobj)
{
    struct sequence_track *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicSeqTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}

void sequence_track_set_items(IDirectMusicTrack8 *track, DMUS_IO_SEQ_ITEM *items, unsigned int count)
{
    struct sequence_track *This = impl_from_IDirectMusicTrack8(track);
    free(This->items);
    This->items = items;
    This->count = count;
}
