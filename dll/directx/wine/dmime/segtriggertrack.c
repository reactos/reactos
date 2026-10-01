/* IDirectMusicSegTriggerTrack Implementation
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

/*****************************************************************************
 * IDirectMusicSegTriggerTrack implementation
 */
struct segment_item {
    struct list entry;
    DMUS_IO_SEGMENT_ITEM_HEADER header;
    IDirectMusicObject *dmobj;
    WCHAR name[DMUS_MAX_NAME];
};

typedef struct IDirectMusicSegTriggerTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;/* IPersistStream only */
    LONG ref;
    struct list Items;
} IDirectMusicSegTriggerTrack;

/* IDirectMusicSegTriggerTrack IDirectMusicTrack8 part: */
static inline IDirectMusicSegTriggerTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSegTriggerTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI segment_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI segment_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI segment_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct list *cursor, *cursor2;
        struct segment_item *item;

        LIST_FOR_EACH_SAFE(cursor, cursor2, &This->Items) {
            item = LIST_ENTRY(cursor, struct segment_item, entry);
            list_remove(cursor);

            if (item->dmobj)
                IDirectMusicObject_Release(item->dmobj);
            free(item);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI segment_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI segment_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI segment_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI segment_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI segment_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): not supported\n", This, debugstr_dmguid(type), time, next, param);
    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI segment_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p): not supported\n", This, debugstr_dmguid(type), time, param);
    return S_OK;
}

static HRESULT WINAPI segment_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s)\n", This, debugstr_dmguid(type));
    return S_OK;
}

static HRESULT WINAPI segment_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI segment_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI segment_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI segment_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI segment_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI segment_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI segment_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI segment_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *newtrack,
        MUSIC_TIME join, IUnknown *context, DWORD trackgroup, IDirectMusicTrack **resulttrack)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %ld, %p, %ld, %p): method not implemented\n", This, newtrack, join, context,
            trackgroup, resulttrack);
    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    segment_track_QueryInterface,
    segment_track_AddRef,
    segment_track_Release,
    segment_track_Init,
    segment_track_InitPlay,
    segment_track_EndPlay,
    segment_track_Play,
    segment_track_GetParam,
    segment_track_SetParam,
    segment_track_IsParamSupported,
    segment_track_AddNotificationType,
    segment_track_RemoveNotificationType,
    segment_track_Clone,
    segment_track_PlayEx,
    segment_track_GetParamEx,
    segment_track_SetParamEx,
    segment_track_Compose,
    segment_track_Join
};

static inline IDirectMusicSegTriggerTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSegTriggerTrack, dmobj.IPersistStream_iface);
}

static HRESULT parse_segment_item(IDirectMusicSegTriggerTrack *This, IStream *stream,
        const struct chunk_entry *lseg)
{
    struct chunk_entry chunk = {.parent = lseg};
    struct segment_item *item;
    HRESULT hr;

    /* First chunk is a header */
    if (stream_get_chunk(stream, &chunk) != S_OK || chunk.id != DMUS_FOURCC_SEGMENTITEM_CHUNK)
        return DMUS_E_TRACK_HDR_NOT_FIRST_CK;
    if (!(item = calloc(1, sizeof(*item)))) return E_OUTOFMEMORY;
    hr = stream_chunk_get_data(stream, &chunk, &item->header, sizeof(DMUS_IO_SEGMENT_ITEM_HEADER));
    if (FAILED(hr))
        goto error;

    TRACE("Found DMUS_IO_SEGMENT_ITEM_HEADER\n");
    TRACE("\tlTimePhysical: %lu\n", item->header.lTimeLogical);
    TRACE("\tlTimePhysical: %lu\n", item->header.lTimePhysical);
    TRACE("\tdwPlayFlags: %#08lx\n", item->header.dwPlayFlags);
    TRACE("\tdwFlags: %#08lx\n", item->header.dwFlags);

    /* Second chunk is a reference list */
    if (stream_next_chunk(stream, &chunk) != S_OK || chunk.id != FOURCC_LIST ||
            chunk.type != DMUS_FOURCC_REF_LIST) {
        hr = DMUS_E_INVALID_SEGMENTTRIGGERTRACK;
        goto error;
    }
    if (FAILED(hr = dmobj_parsereference(stream, &chunk, &item->dmobj)))
        goto error;

    /* Optional third chunk if the reference is a motif */
    if (item->header.dwFlags & DMUS_SEGMENTTRACKF_MOTIF) {
        if (FAILED(hr = stream_next_chunk(stream, &chunk)))
            goto error;
        if (chunk.id == DMUS_FOURCC_SEGMENTITEMNAME_CHUNK)
            if (FAILED(hr = stream_chunk_get_wstr(stream, &chunk, item->name, DMUS_MAX_NAME)))
                goto error;

        TRACE("Found motif name: %s\n", debugstr_w(item->name));
    }

    list_add_tail(&This->Items, &item->entry);

    return S_OK;

error:
    free(item);
    return hr;
}

static HRESULT parse_segments_list(IDirectMusicSegTriggerTrack *This, IStream *stream,
        const struct chunk_entry *lsgl)
{
    struct chunk_entry chunk = {.parent = lsgl};
    HRESULT hr;

    TRACE("Parsing segment list in %p: %s\n", stream, debugstr_chunk(lsgl));

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_SEGMENT_LIST)
            if (FAILED(hr = parse_segment_item(This, stream, &chunk)))
                break;

    return SUCCEEDED(hr) ? S_OK : hr;
}

static HRESULT WINAPI trigger_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicSegTriggerTrack *This = impl_from_IPersistStream(iface);
    struct chunk_entry segt = {0};
    struct chunk_entry chunk = {.parent = &segt};
    DMUS_IO_SEGMENT_TRACK_HEADER header;
    HRESULT hr;

    TRACE("(%p, %p): Loading\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &segt) != S_OK || segt.id != FOURCC_LIST ||
            segt.type != DMUS_FOURCC_SEGTRACK_LIST)
        return DMUS_E_INVALID_SEGMENTTRIGGERTRACK;

    if ((hr = stream_get_chunk(stream, &chunk)) != S_OK)
        return FAILED(hr) ? hr : DMUS_E_INVALID_SEGMENTTRIGGERTRACK;

    /* Optional and useless header chunk */
    if (chunk.id == DMUS_FOURCC_SEGTRACK_CHUNK) {
        hr = stream_chunk_get_data(stream, &chunk, &header, sizeof(DMUS_IO_SEGMENT_TRACK_HEADER));
        if (FAILED(hr))
            return hr;
        if (header.dwFlags)
            WARN("Got flags %#lx; must be zero\n", header.dwFlags);

        if ((hr = stream_get_chunk(stream, &chunk)) != S_OK)
            return FAILED(hr) ? hr : DMUS_E_INVALID_SEGMENTTRIGGERTRACK;
    }

    if (chunk.id != FOURCC_LIST || chunk.type != DMUS_FOURCC_SEGMENTS_LIST)
        return DMUS_E_INVALID_SEGMENTTRIGGERTRACK;

    return parse_segments_list(This, stream, &chunk);
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    trigger_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmsegtriggertrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicSegTriggerTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicSegTriggerTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&track->Items);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
