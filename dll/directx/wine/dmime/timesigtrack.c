/* IDirectMusicTimeSigTrack Implementation
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

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

/*****************************************************************************
 * IDirectMusicTimeSigTrack implementation
 */
typedef struct IDirectMusicTimeSigTrack {
    IDirectMusicTrack IDirectMusicTrack_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;

    DMUS_IO_TIMESIGNATURE_ITEM *items;
    unsigned int count;
} IDirectMusicTimeSigTrack;

/* IDirectMusicTimeSigTrack IDirectMusicTrack8 part: */
static inline IDirectMusicTimeSigTrack *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTimeSigTrack, IDirectMusicTrack_iface);
}

static inline IDirectMusicTimeSigTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTimeSigTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack))
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

static ULONG WINAPI IDirectMusicTrackImpl_AddRef(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicTrackImpl_Release(IDirectMusicTrack *iface)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This->items);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Init(IDirectMusicTrack *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_InitPlay(IDirectMusicTrack *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_EndPlay(IDirectMusicTrack *iface, void *pStateData)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Play(IDirectMusicTrack *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_GetParam(IDirectMusicTrack *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), time, next, param);

    if (IsEqualGUID(type, &GUID_TimeSignature)) {
        FIXME("GUID_TimeSignature not handled yet\n");
        return DMUS_E_NOT_FOUND;
    }

    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_SetParam(IDirectMusicTrack *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (IsEqualGUID(type, &GUID_DisableTimeSig)) {
        FIXME("GUID_DisableTimeSig not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_EnableTimeSig)) {
        FIXME("GUID_EnableTimeSig not handled yet\n");
        return S_OK;
    }

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported(IDirectMusicTrack *iface,
        REFGUID rguidType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_DisableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_EnableTimeSig)
		|| IsEqualGUID (rguidType, &GUID_TimeSignature)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Clone(IDirectMusicTrack *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicTimeSigTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static const IDirectMusicTrackVtbl dmtack_vtbl = {
    IDirectMusicTrackImpl_QueryInterface,
    IDirectMusicTrackImpl_AddRef,
    IDirectMusicTrackImpl_Release,
    IDirectMusicTrackImpl_Init,
    IDirectMusicTrackImpl_InitPlay,
    IDirectMusicTrackImpl_EndPlay,
    IDirectMusicTrackImpl_Play,
    IDirectMusicTrackImpl_GetParam,
    IDirectMusicTrackImpl_SetParam,
    IDirectMusicTrackImpl_IsParamSupported,
    IDirectMusicTrackImpl_AddNotificationType,
    IDirectMusicTrackImpl_RemoveNotificationType,
    IDirectMusicTrackImpl_Clone
};

static HRESULT parse_timetrack_list(IDirectMusicTimeSigTrack *This, IStream *stream,
            struct chunk_entry *timesig)
{
    HRESULT hr;
    struct chunk_entry chunk = {.parent = timesig};
    int i;

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(timesig));

    if (FAILED(hr = stream_next_chunk(stream, &chunk))) {
        WARN("Failed to read data of %s\n", debugstr_chunk(&chunk));
        return hr;
    }

    if (chunk.id != DMUS_FOURCC_TIMESIGNATURE_TRACK)
        return DMUS_E_UNSUPPORTED_STREAM;

    hr = stream_chunk_get_array(stream, &chunk, (void **)&This->items, &This->count,
            sizeof(DMUS_IO_TIMESIGNATURE_ITEM));
    if (FAILED(hr))
        return hr;

    for (i = 0; i < This->count; i++)
    {
        TRACE("Found DMUS_IO_TIMESIGNATURE_ITEM\n");
        TRACE("  - lTime %ld\n", This->items[i].lTime);
        TRACE("  - bBeatsPerMeasure %d\n", This->items[i].bBeatsPerMeasure);
        TRACE("  - bBeat %d\n", This->items[i].bBeat);
        TRACE("  - wGridsPerBeat %d\n", This->items[i].wGridsPerBeat);
    }

    return S_OK;
}

static HRESULT WINAPI time_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicTimeSigTrack *This = impl_from_IPersistStream(iface);
    HRESULT hr;
    struct chunk_entry chunk = {0};

    TRACE("%p, %p\n", This, stream);

    if (!stream)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &chunk)) != S_OK)
        return hr;

    if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_TIMESIGTRACK_LIST)
        hr = parse_timetrack_list(This, stream, &chunk);
    else
        hr = DMUS_E_UNSUPPORTED_STREAM;

    return hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    time_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmtimesigtrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicTimeSigTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack_iface.lpVtbl = &dmtack_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicTimeSigTrack,
                  (IUnknown *)&track->IDirectMusicTrack_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack_QueryInterface(&track->IDirectMusicTrack_iface, lpcGUID, ppobj);
    IDirectMusicTrack_Release(&track->IDirectMusicTrack_iface);

    return hr;
}
