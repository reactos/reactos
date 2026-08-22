/* IDirectMusicMarkerTrack Implementation
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
 * IDirectMusicMarkerTrack implementation
 */
typedef struct IDirectMusicMarkerTrack {
    IDirectMusicTrack IDirectMusicTrack_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicMarkerTrack;

/* IDirectMusicMarkerTrack IDirectMusicTrack8 part: */
static inline IDirectMusicMarkerTrack *impl_from_IDirectMusicTrack(IDirectMusicTrack *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicMarkerTrack, IDirectMusicTrack_iface);
}

static HRESULT WINAPI IDirectMusicTrackImpl_QueryInterface(IDirectMusicTrack *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);

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
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicTrackImpl_Release(IDirectMusicTrack *iface)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Init(IDirectMusicTrack *iface,
        IDirectMusicSegment *pSegment)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_InitPlay(IDirectMusicTrack *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_EndPlay(IDirectMusicTrack *iface, void *pStateData)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Play(IDirectMusicTrack *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_GetParam(IDirectMusicTrack *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), time, next, param);

    if (!param)
        return E_POINTER;

    if (IsEqualGUID(type, &GUID_Play_Marker)) {
        FIXME("GUID_Play_Marker not handled yet\n");
        return S_FALSE;
    }
    if (IsEqualGUID(type, &GUID_Valid_Start_Time)) {
        FIXME("GUID_Valid_Start_Time not handled yet\n");
        return DMUS_E_NOT_FOUND;
    }

    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_SetParam(IDirectMusicTrack *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);

    TRACE("(%p, %s, %ld, %p): not supported\n", This, debugstr_dmguid(type), time, param);
    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_IsParamSupported(IDirectMusicTrack *iface,
        REFGUID rguidType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
	if (IsEqualGUID (rguidType, &GUID_Play_Marker)
		|| IsEqualGUID (rguidType, &GUID_Valid_Start_Time)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI IDirectMusicTrackImpl_AddNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_RemoveNotificationType(IDirectMusicTrack *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI IDirectMusicTrackImpl_Clone(IDirectMusicTrack *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicMarkerTrack *This = impl_from_IDirectMusicTrack(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static const IDirectMusicTrackVtbl dmtrack_vtbl = {
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

static HRESULT WINAPI marker_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    marker_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmmarkertrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicMarkerTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack_iface.lpVtbl = &dmtrack_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicMarkerTrack,
                  (IUnknown *)&track->IDirectMusicTrack_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack_QueryInterface(&track->IDirectMusicTrack_iface, lpcGUID, ppobj);
    IDirectMusicTrack_Release(&track->IDirectMusicTrack_iface);

    return hr;
}
