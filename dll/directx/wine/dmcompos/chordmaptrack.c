/* IDirectMusicChordMapTrack Implementation
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

#include "dmcompos_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

/*****************************************************************************
 * IDirectMusicChordMapTrack implementation
 */
typedef struct IDirectMusicChordMapTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicChordMapTrack;

/* IDirectMusicChordMapTrack IDirectMusicTrack8 part: */
static inline IDirectMusicChordMapTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicChordMapTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI chordmap_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI chordmap_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI chordmap_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI chordmap_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), time, next, param);

    if (!type)
        return E_POINTER;
    if (!IsEqualGUID(type, &GUID_IDirectMusicChordMap))
        return DMUS_E_GET_UNSUPPORTED;

    FIXME("GUID_IDirectMusicChordMap not handled yet\n");

    return S_OK;
}

static HRESULT WINAPI chordmap_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (!type)
        return E_POINTER;
    if (!IsEqualGUID(type, &GUID_IDirectMusicChordMap))
        return DMUS_E_SET_UNSUPPORTED;

    FIXME("GUID_IDirectMusicChordMap not handled yet\n");

    return S_OK;
}

static HRESULT WINAPI chordmap_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

        if (!rguidType)
                return E_POINTER;

	if (IsEqualGUID (rguidType, &GUID_IDirectMusicChordMap)) {
		TRACE("param supported\n");
		return S_OK;
	}
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI chordmap_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI chordmap_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI chordmap_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI chordmap_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI chordmap_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicChordMapTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    chordmap_track_QueryInterface,
    chordmap_track_AddRef,
    chordmap_track_Release,
    chordmap_track_Init,
    chordmap_track_InitPlay,
    chordmap_track_EndPlay,
    chordmap_track_Play,
    chordmap_track_GetParam,
    chordmap_track_SetParam,
    chordmap_track_IsParamSupported,
    chordmap_track_AddNotificationType,
    chordmap_track_RemoveNotificationType,
    chordmap_track_Clone,
    chordmap_track_PlayEx,
    chordmap_track_GetParamEx,
    chordmap_track_SetParamEx,
    chordmap_track_Compose,
    chordmap_track_Join
};

static inline IDirectMusicChordMapTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicChordMapTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
        IDirectMusicChordMapTrack *This = impl_from_IPersistStream(iface);
	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	return S_OK;
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
HRESULT create_dmchordmaptrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicChordMapTrack* track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicChordMapTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
