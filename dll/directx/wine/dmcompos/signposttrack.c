/* IDirectMusicSignPostTrack Implementation
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
 * IDirectMusicSignPostTrack implementation
 */
typedef struct IDirectMusicSignPostTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicSignPostTrack;

/* IDirectMusicSignPostTrack IDirectMusicTrack8 part: */
static inline IDirectMusicSignPostTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSignPostTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI signpost_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI signpost_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI signpost_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI signpost_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI signpost_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI signpost_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI signpost_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI signpost_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): method not implemented\n", This, debugstr_dmguid(type), time,
            next, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI signpost_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p): method not implemented\n", This, debugstr_dmguid(type), time, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI signpost_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(type));
    return E_NOTIMPL;
}

static HRESULT WINAPI signpost_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI signpost_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI signpost_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI signpost_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI signpost_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, REFERENCE_TIME *next, void *param, void *state, DWORD flags)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %p, %lx): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), next, param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI signpost_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID type,
        REFERENCE_TIME time, void *param, void *state, DWORD flags)
{
    IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %s, %p, %p, %lx): method not implemented\n", This, debugstr_dmguid(type),
            wine_dbgstr_longlong(time), param, state, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI signpost_track_Compose(IDirectMusicTrack8 *iface, IUnknown *pContext,
        DWORD dwTrackGroup, IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %p): stub\n", This, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static HRESULT WINAPI signpost_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicSignPostTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    signpost_track_QueryInterface,
    signpost_track_AddRef,
    signpost_track_Release,
    signpost_track_Init,
    signpost_track_InitPlay,
    signpost_track_EndPlay,
    signpost_track_Play,
    signpost_track_GetParam,
    signpost_track_SetParam,
    signpost_track_IsParamSupported,
    signpost_track_AddNotificationType,
    signpost_track_RemoveNotificationType,
    signpost_track_Clone,
    signpost_track_PlayEx,
    signpost_track_GetParamEx,
    signpost_track_SetParamEx,
    signpost_track_Compose,
    signpost_track_Join
};

static inline IDirectMusicSignPostTrack * impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSignPostTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
        IDirectMusicSignPostTrack *This = impl_from_IPersistStream(iface);
	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_Save(IPersistStream *iface, IStream *stream,
        BOOL cleardirty)
{
    IDirectMusicSignPostTrack *This = impl_from_IPersistStream(iface);

    FIXME("(%p, %p, %d): stub\n", This, stream, cleardirty);

    if (!stream)
        return E_POINTER;

    return E_NOTIMPL;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    IPersistStreamImpl_Load,
    IPersistStreamImpl_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmsignposttrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicSignPostTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicSignPostTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
