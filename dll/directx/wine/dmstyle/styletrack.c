/* IDirectMusicStyleTrack Implementation
 *
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

#include "dmstyle_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmstyle);

/*****************************************************************************
 * IDirectMusicStyleTrack implementation
 */
struct style_item {
    struct list entry;
    DWORD timestamp;
    IDirectMusicStyle8 *dmstyle;
};

typedef struct IDirectMusicStyleTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
    struct list Items;
} IDirectMusicStyleTrack;

/* IDirectMusicStyleTrack IDirectMusicTrack8 part: */
static inline IDirectMusicStyleTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicStyleTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI style_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI style_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI style_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct style_item *item, *item2;

        LIST_FOR_EACH_ENTRY_SAFE(item, item2, &This->Items, struct style_item, entry) {
            list_remove(&item->entry);
            IDirectMusicStyle8_Release(item->dmstyle);
            free(item);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI style_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI style_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI style_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI style_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI style_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
    struct style_item *item;

    TRACE("(%p, %s, %ld, %p, %p):\n", This, debugstr_dmguid(type), time, next, param);

    if (!type)
        return E_POINTER;

    if (IsEqualGUID(&GUID_IDirectMusicStyle, type)) {
        LIST_FOR_EACH_ENTRY(item, &This->Items, struct style_item, entry) {
            IDirectMusicStyle8_AddRef(item->dmstyle);
            *((IDirectMusicStyle8 **)param) = item->dmstyle;

            return S_OK;
        }

        return DMUS_E_NOT_FOUND;
    } else if (IsEqualGUID(&GUID_TimeSignature, type)) {
        FIXME("GUID_TimeSignature not handled yet\n");
        return S_OK;
    }

    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI style_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (!type)
        return E_POINTER;

    if (IsEqualGUID(type, &GUID_DisableTimeSig)) {
        FIXME("GUID_DisableTimeSig not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_EnableTimeSig)) {
        FIXME("GUID_EnableTimeSig not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_IDirectMusicStyle)) {
        FIXME("GUID_IDirectMusicStyle not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_SeedVariations)) {
        FIXME("GUID_SeedVariations not handled yet\n");
        return S_OK;
    }

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI style_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

        if (!rguidType)
                return E_POINTER;

	if (IsEqualGUID (rguidType, &GUID_DisableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_EnableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_IDirectMusicStyle)
		|| IsEqualGUID (rguidType, &GUID_SeedVariations)
		|| IsEqualGUID (rguidType, &GUID_TimeSignature)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI style_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI style_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI style_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI style_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI style_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI style_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI style_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI style_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicStyleTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    style_track_QueryInterface,
    style_track_AddRef,
    style_track_Release,
    style_track_Init,
    style_track_InitPlay,
    style_track_EndPlay,
    style_track_Play,
    style_track_GetParam,
    style_track_SetParam,
    style_track_IsParamSupported,
    style_track_AddNotificationType,
    style_track_RemoveNotificationType,
    style_track_Clone,
    style_track_PlayEx,
    style_track_GetParamEx,
    style_track_SetParamEx,
    style_track_Compose,
    style_track_Join
};

static HRESULT parse_style_ref(IDirectMusicStyleTrack *This, IStream *stream, const struct chunk_entry *strf)
{
    struct chunk_entry chunk = {.parent = strf};
    IDirectMusicObject *dmobj;
    struct style_item *item;
    HRESULT hr;

    /* First chunk is a timestamp */
    if (stream_get_chunk(stream, &chunk) != S_OK || chunk.id != DMUS_FOURCC_TIME_STAMP_CHUNK)
        return DMUS_E_CHUNKNOTFOUND;
    if (!(item = malloc(sizeof(*item))))
        return E_OUTOFMEMORY;
    hr = stream_chunk_get_data(stream, &chunk, &item->timestamp, sizeof(item->timestamp));
    if (FAILED(hr))
        goto error;

    /* Second chunk is a reference list */
    if (stream_next_chunk(stream, &chunk) != S_OK || chunk.id != FOURCC_LIST ||
            chunk.type != DMUS_FOURCC_REF_LIST) {
        hr = DMUS_E_INVALID_SEGMENTTRIGGERTRACK;
        goto error;
    }
    if (FAILED(hr = dmobj_parsereference(stream, &chunk, &dmobj))) {
        WARN("Failed to load reference: %#lx\n", hr);
        goto error;
    }
    hr = IDirectMusicObject_QueryInterface(dmobj, &IID_IDirectMusicStyle8, (void **)&item->dmstyle);
    if (FAILED(hr)) {
        WARN("Reference not an IDirectMusicStyle8\n");
        IDirectMusicObject_Release(dmobj);
        goto error;
    }

    list_add_tail(&This->Items, &item->entry);
    TRACE("Found reference to style %p with timestamp %lu\n", item->dmstyle, item->timestamp);

    return S_OK;

error:
    free(item);
    return hr;
}

static inline IDirectMusicStyleTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicStyleTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicStyleTrack *This = impl_from_IPersistStream(iface);
    struct chunk_entry track = {0};
    struct chunk_entry chunk = {.parent = &track};
    HRESULT hr;

    TRACE("(%p, %p): Loading\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &track) != S_OK || track.id != FOURCC_LIST ||
            track.type != DMUS_FOURCC_STYLE_TRACK_LIST)
        return DMUS_E_TRACK_NOT_FOUND;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
        if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_STYLE_REF_LIST)
            if (FAILED(hr = parse_style_ref(This, stream, &chunk)))
                break;

    return SUCCEEDED(hr) ? S_OK : hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    IPersistStreamImpl_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmstyletrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicStyleTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicStyleTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init (&track->Items);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
