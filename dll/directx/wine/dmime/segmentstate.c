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

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

static DWORD next_track_id;

struct track_entry
{
    struct list entry;

    IDirectMusicTrack *track;
    void *state_data;
    DWORD track_id;
};

static void track_entry_destroy(struct track_entry *entry)
{
    HRESULT hr;

    if (FAILED(hr = IDirectMusicTrack_EndPlay(entry->track, entry->state_data)))
        WARN("track %p EndPlay failed, hr %#lx\n", entry->track, hr);

    IDirectMusicTrack_Release(entry->track);
    free(entry);
}

struct segment_state
{
    IDirectMusicSegmentState8 IDirectMusicSegmentState8_iface;
    IDirectMusicGraph IDirectMusicGraph_iface;
    LONG ref;

    IDirectMusicGraph *parent_graph;
    IDirectMusicSegment *segment;
    MUSIC_TIME start_time;
    MUSIC_TIME start_point;
    MUSIC_TIME end_point;
    MUSIC_TIME played;

    REFERENCE_TIME actual_duration;
    MUSIC_TIME actual_end_point;

    BOOL auto_download;
    DWORD repeats, actual_repeats;
    DWORD track_flags;

    struct list tracks;
};

static inline struct segment_state *impl_from_IDirectMusicSegmentState8(IDirectMusicSegmentState8 *iface)
{
    return CONTAINING_RECORD(iface, struct segment_state, IDirectMusicSegmentState8_iface);
}

static HRESULT WINAPI segment_state_QueryInterface(IDirectMusicSegmentState8 *iface, REFIID riid, void **ppobj)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState8))
    {
        IDirectMusicSegmentState8_AddRef(iface);
        *ppobj = &This->IDirectMusicSegmentState8_iface;
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IDirectMusicGraph))
    {
        IDirectMusicSegmentState8_AddRef(iface);
        *ppobj = &This->IDirectMusicGraph_iface;
        return S_OK;
    }
    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI segment_state_AddRef(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI segment_state_Release(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (!ref)
    {
        segment_state_end_play((IDirectMusicSegmentState *)iface, NULL);
        if (This->segment) IDirectMusicSegment_Release(This->segment);
        if (This->parent_graph) IDirectMusicGraph_Release(This->parent_graph);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI segment_state_GetRepeats(IDirectMusicSegmentState8 *iface, DWORD *repeats)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    TRACE("(%p, %p)\n", This, repeats);
    *repeats = This->repeats;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSegment(IDirectMusicSegmentState8 *iface, IDirectMusicSegment **segment)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, segment);

    if (!(*segment = This->segment)) return DMUS_E_NOT_FOUND;

    IDirectMusicSegment_AddRef(This->segment);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartTime(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_time);

    *start_time = This->start_time;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSeek(IDirectMusicSegmentState8 *iface, MUSIC_TIME *seek_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): semi-stub\n", This, seek_time);
    *seek_time = 0;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartPoint(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_point)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_point);

    *start_point = This->start_point;
    return S_OK;
}

static HRESULT WINAPI segment_state_SetTrackConfig(IDirectMusicSegmentState8 *iface,
        REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetObjectInPath(IDirectMusicSegmentState8 *iface, DWORD dwPChannel,
        DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void **ppObject)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject), dwIndex, debugstr_dmguid(iidInterface), ppObject);
    return S_OK;
}

static const IDirectMusicSegmentState8Vtbl segment_state_vtbl =
{
    segment_state_QueryInterface,
    segment_state_AddRef,
    segment_state_Release,
    segment_state_GetRepeats,
    segment_state_GetSegment,
    segment_state_GetStartTime,
    segment_state_GetSeek,
    segment_state_GetStartPoint,
    segment_state_SetTrackConfig,
    segment_state_GetObjectInPath,
};

static inline struct segment_state *impl_from_IDirectMusicGraph(IDirectMusicGraph *iface)
{
    return CONTAINING_RECORD(iface, struct segment_state, IDirectMusicGraph_iface);
}

static HRESULT WINAPI segment_state_graph_QueryInterface(IDirectMusicGraph *iface, REFIID riid, void **ret_iface)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicSegmentState8_QueryInterface(&This->IDirectMusicSegmentState8_iface, riid, ret_iface);
}

static ULONG WINAPI segment_state_graph_AddRef(IDirectMusicGraph *iface)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicSegmentState8_AddRef(&This->IDirectMusicSegmentState8_iface);
}

static ULONG WINAPI segment_state_graph_Release(IDirectMusicGraph *iface)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicSegmentState8_Release(&This->IDirectMusicSegmentState8_iface);
}

static HRESULT WINAPI segment_state_graph_StampPMsg(IDirectMusicGraph *iface, DMUS_PMSG *msg)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    HRESULT hr;

    TRACE("(%p, %p)\n", This, msg);

    if (!msg) return E_POINTER;

    hr = IDirectMusicGraph_StampPMsg(This->parent_graph, msg);
    if (SUCCEEDED(hr))
    {
        switch (msg->dwType)
        {
        case DMUS_PMSGT_WAVE:
            if (msg->dwFlags & (DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME))
            {
                if (((DMUS_WAVE_PMSG *)msg)->rtDuration > This->actual_duration)
                    This->actual_duration = ((DMUS_WAVE_PMSG *)msg)->rtDuration;
            }
            break;
        default: ;
        }
    }

    return hr;
}

static HRESULT WINAPI segment_state_graph_InsertTool(IDirectMusicGraph *iface, IDirectMusicTool *tool,
        DWORD *channels, DWORD channels_count, LONG index)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %p, %p, %lu, %ld)\n", This, tool, channels, channels_count, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI segment_state_graph_GetTool(IDirectMusicGraph *iface, DWORD index, IDirectMusicTool **tool)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %lu, %p)\n", This, index, tool);
    return E_NOTIMPL;
}

static HRESULT WINAPI segment_state_graph_RemoveTool(IDirectMusicGraph *iface, IDirectMusicTool *tool)
{
    struct segment_state *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %p)\n", This, tool);
    return E_NOTIMPL;
}

static const IDirectMusicGraphVtbl segment_state_graph_vtbl =
{
    segment_state_graph_QueryInterface,
    segment_state_graph_AddRef,
    segment_state_graph_Release,
    segment_state_graph_StampPMsg,
    segment_state_graph_InsertTool,
    segment_state_graph_GetTool,
    segment_state_graph_RemoveTool,
};

/* for ClassFactory */
HRESULT create_dmsegmentstate(REFIID riid, void **ret_iface)
{
    struct segment_state *obj;
    HRESULT hr;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicSegmentState8_iface.lpVtbl = &segment_state_vtbl;
    obj->IDirectMusicGraph_iface.lpVtbl = &segment_state_graph_vtbl;
    obj->ref = 1;
    obj->start_time = -1;
    list_init(&obj->tracks);

    TRACE("Created IDirectMusicSegmentState %p\n", obj);
    hr = IDirectMusicSegmentState8_QueryInterface(&obj->IDirectMusicSegmentState8_iface, riid, ret_iface);
    IDirectMusicSegmentState8_Release(&obj->IDirectMusicSegmentState8_iface);
    return hr;
}

HRESULT segment_state_create(IDirectMusicSegment *segment, MUSIC_TIME start_time,
        IDirectMusicPerformance8 *performance, IDirectMusicSegmentState **ret_iface)
{
    IDirectMusicSegmentState *iface;
    struct segment_state *This;
    IDirectMusicGraph *graph;
    IDirectMusicTrack *track;
    HRESULT hr;
    UINT i;

    TRACE("(%p, %lu, %p)\n", segment, start_time, ret_iface);

    if (FAILED(hr = create_dmsegmentstate(&IID_IDirectMusicSegmentState, (void **)&iface))) return hr;
    This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    This->segment = segment;
    IDirectMusicSegment_AddRef(This->segment);

    hr = IDirectMusicPerformance8_QueryInterface(performance, &IID_IDirectMusicGraph, (void **)&graph);

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = IDirectMusicPerformance8_GetGlobalParam(performance, &GUID_PerfAutoDownload,
            &This->auto_download, sizeof(This->auto_download))) && This->auto_download)
        hr = IDirectMusicSegment_SetParam(segment, &GUID_DownloadToAudioPath, -1,
                    DMUS_SEG_ALLTRACKS, 0, performance);

    This->start_time = start_time;
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetStartPoint(segment, &This->start_point);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetLength(segment, &This->end_point);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetRepeats(segment, &This->repeats);
    if (SUCCEEDED(hr)) This->actual_repeats = This->repeats;

    for (i = 0; SUCCEEDED(hr); i++)
    {
        DWORD track_id = ++next_track_id;
        struct track_entry *entry;

        if ((hr = IDirectMusicSegment_GetTrack(segment, &GUID_NULL, -1, i, &track)) != S_OK)
        {
            if (hr == DMUS_E_NOT_FOUND) hr = S_OK;
            break;
        }

        if (!(entry = malloc(sizeof(*entry))))
            hr = E_OUTOFMEMORY;
        else if (SUCCEEDED(hr = IDirectMusicTrack_InitPlay(track, iface, (IDirectMusicPerformance *)performance,
                &entry->state_data, track_id, 0)))
        {
            entry->track = track;
            entry->track_id = track_id;
            list_add_tail(&This->tracks, &entry->entry);
        }

        if (FAILED(hr))
        {
            WARN("Failed to initialize track %p, hr %#lx\n", track, hr);
            IDirectMusicTrack_Release(track);
            free(entry);
        }
    }

    if (SUCCEEDED(hr))
    {
        *ret_iface = iface;
        This->parent_graph = graph;
    }
    else
    {
        IDirectMusicSegmentState_Release(iface);
        IDirectMusicGraph_Release(graph);
    }

    return hr;
}

static HRESULT segment_state_play_until(struct segment_state *This, IDirectMusicPerformance8 *performance,
        MUSIC_TIME end_time, DWORD track_flags)
{
    IDirectMusicSegmentState *iface = (IDirectMusicSegmentState *)&This->IDirectMusicSegmentState8_iface;
    struct track_entry *entry;
    HRESULT hr = S_OK;
    MUSIC_TIME played;

    played = min(end_time - This->start_time, This->end_point - This->start_point);

    if (This->track_flags & DMUS_TRACKF_DIRTY)
        This->actual_duration = 0;

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (FAILED(hr = IDirectMusicTrack_Play(entry->track, entry->state_data,
                This->start_point + This->played, This->start_point + played,
                This->start_time, track_flags, (IDirectMusicPerformance *)performance,
                iface, entry->track_id)))
        {
            WARN("Failed to play track %p, hr %#lx\n", entry->track, hr);
            break;
        }
    }

    This->played = played;
    This->track_flags &= ~(DMUS_TRACKF_START|DMUS_TRACKF_DIRTY);
    if (This->start_point + This->played >= This->end_point) return S_FALSE;
    return S_OK;
}

static HRESULT segment_state_play_chunk(struct segment_state *This, IDirectMusicPerformance8 *performance,
        REFERENCE_TIME duration)
{
    IDirectMusicSegmentState *iface = (IDirectMusicSegmentState *)&This->IDirectMusicSegmentState8_iface;
    MUSIC_TIME next_time;
    REFERENCE_TIME time;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPerformance8_MusicToReferenceTime(performance,
            This->start_time + This->played, &time)))
        return hr;
    if (FAILED(hr = IDirectMusicPerformance8_ReferenceToMusicTime(performance,
            time + duration, &next_time)))
        return hr;

    while ((hr = segment_state_play_until(This, performance, next_time, This->track_flags)) == S_FALSE)
    {
        if (!This->actual_repeats)
        {
            MUSIC_TIME end_time = This->start_time + This->played;

            if (FAILED(hr = performance_send_segment_end(performance, end_time, iface, FALSE)))
            {
                ERR("Failed to send segment end, hr %#lx\n", hr);
                return hr;
            }

            return S_FALSE;
        }

        if (FAILED(hr = IDirectMusicSegment_GetLoopPoints(This->segment, &This->played,
                &This->end_point)))
            break;
        if (!This->played && !This->end_point)
        {
            if (!This->actual_end_point && This->actual_duration)
            {
                IDirectMusicPerformance_ReferenceToMusicTime(performance, time + This->actual_duration, &This->actual_end_point);
                This->actual_end_point -= This->start_time + This->played;
            }
            This->end_point = This->actual_end_point;
            if (next_time < This->start_time + This->end_point)
                next_time += This->end_point - This->start_point;
        }
        This->start_time += This->end_point - This->start_point;
        This->actual_repeats--;
        This->track_flags |= DMUS_TRACKF_LOOP | DMUS_TRACKF_SEEK;

        if (next_time <= This->start_time || This->end_point <= This->start_point) break;
    }

    return S_OK;
}

HRESULT segment_state_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    HRESULT hr;

    TRACE("%p %p\n", iface, performance);

    if (FAILED(hr = performance_send_segment_start(performance, This->start_time, iface)))
    {
        ERR("Failed to send segment start, hr %#lx\n", hr);
        return hr;
    }

    This->track_flags = DMUS_TRACKF_START | DMUS_TRACKF_SEEK | DMUS_TRACKF_DIRTY;
    if (FAILED(hr = segment_state_play_chunk(This, performance, 10000000)))
        return hr;

    if (hr == S_FALSE) return S_OK;
    return performance_send_segment_tick(performance, This->start_time, iface);
}

HRESULT segment_state_tick(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    TRACE("%p %p\n", iface, performance);

    return segment_state_play_chunk(This, performance, 10000000);
}

HRESULT segment_state_stop(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    TRACE("%p %p\n", iface, performance);

    This->played = This->end_point - This->start_point;
    return performance_send_segment_end(performance, This->start_time + This->played, iface, TRUE);
}

HRESULT segment_state_end_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    struct track_entry *entry, *next;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tracks, struct track_entry, entry)
    {
        list_remove(&entry->entry);
        track_entry_destroy(entry);
    }

    if (performance && This->auto_download && FAILED(hr = IDirectMusicSegment_SetParam(This->segment,
            &GUID_UnloadFromAudioPath, -1, DMUS_SEG_ALLTRACKS, 0, performance)))
        ERR("Failed to unload segment from performance, hr %#lx\n", hr);

    return S_OK;
}

BOOL segment_state_has_segment(IDirectMusicSegmentState *iface, IDirectMusicSegment *segment)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    return !segment || This->segment == segment;
}

BOOL segment_state_has_track(IDirectMusicSegmentState *iface, DWORD track_id)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    struct track_entry *entry;

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
        if (entry->track_id == track_id) return TRUE;

    return FALSE;
}
