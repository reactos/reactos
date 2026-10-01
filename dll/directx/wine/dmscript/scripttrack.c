/* IDirectMusicScriptTrack Implementation
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

#include "dmscript_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

/*****************************************************************************
 * IDirectMusicScriptTrack implementation
 */

typedef struct DirectMusicScriptTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    IPersistStream IPersistStream_iface;
    LONG ref;
    DMUS_OBJECTDESC desc;
} DirectMusicScriptTrack;

static inline DirectMusicScriptTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, DirectMusicScriptTrack, IDirectMusicTrack8_iface);
}

static inline DirectMusicScriptTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, DirectMusicScriptTrack, IPersistStream_iface);
}

/* DirectMusicScriptTrack IDirectMusicTrack8 part: */
static HRESULT WINAPI script_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicTrack) ||
            IsEqualIID(riid, &IID_IDirectMusicTrack8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI script_track_AddRef(IDirectMusicTrack8 *iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI script_track_Release(IDirectMusicTrack8 *iface)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI script_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI script_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI script_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI script_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI script_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): not supported\n", This, debugstr_dmguid(type), time, next, param);

    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI script_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p): not supported\n", This, debugstr_dmguid(type), time, param);

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI script_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID type)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): param type not supported\n", This, debugstr_dmguid(type));

    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI script_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID type)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI script_track_RemoveNotificationType(IDirectMusicTrack8 *iface, REFGUID type)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI script_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI script_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI script_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI script_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
	DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI script_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD group, IDirectMusicTrack **res_track)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, group, res_track);

    return E_NOTIMPL;
}

static HRESULT WINAPI script_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *track2,
        MUSIC_TIME join, IUnknown *context, DWORD group, IDirectMusicTrack **res_track)
{
    DirectMusicScriptTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p, %ld, %p): method not implemented\n", This, track2, join, context,
            group, res_track);

    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    script_track_QueryInterface,
    script_track_AddRef,
    script_track_Release,
    script_track_Init,
    script_track_InitPlay,
    script_track_EndPlay,
    script_track_Play,
    script_track_GetParam,
    script_track_SetParam,
    script_track_IsParamSupported,
    script_track_AddNotificationType,
    script_track_RemoveNotificationType,
    script_track_Clone,
    script_track_PlayEx,
    script_track_GetParamEx,
    script_track_SetParamEx,
    script_track_Compose,
    script_track_Join
};

/* IDirectMusicScriptTrack IPersistStream part: */
static HRESULT WINAPI IPersistStreamImpl_QueryInterface(IPersistStream *iface, REFIID riid,
        void **ret_iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_QueryInterface(&This->IDirectMusicTrack8_iface, riid, ret_iface);
}

static ULONG WINAPI IPersistStreamImpl_AddRef(IPersistStream *iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_AddRef(&This->IDirectMusicTrack8_iface);
}

static ULONG WINAPI IPersistStreamImpl_Release(IPersistStream *iface)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);
    return IDirectMusicTrack8_Release(&This->IDirectMusicTrack8_iface);
}

static HRESULT WINAPI IPersistStreamImpl_GetClassID(IPersistStream *iface, CLSID *pClassID)
{
    DirectMusicScriptTrack *This = impl_from_IPersistStream(iface);

    TRACE("(%p, %p)\n", This, pClassID);

    if (!pClassID)
        return E_POINTER;

    *pClassID = This->desc.guidClass;
    return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_IsDirty(IPersistStream *iface)
{
    return S_FALSE;
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_Save(IPersistStream *iface, IStream *pStm,
        BOOL fClearDirty)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IPersistStreamImpl_GetSizeMax(IPersistStream *iface, ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

static const IPersistStreamVtbl persist_vtbl = {
    IPersistStreamImpl_QueryInterface,
    IPersistStreamImpl_AddRef,
    IPersistStreamImpl_Release,
    IPersistStreamImpl_GetClassID,
    IPersistStreamImpl_IsDirty,
    IPersistStreamImpl_Load,
    IPersistStreamImpl_Save,
    IPersistStreamImpl_GetSizeMax
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicScriptTrack(REFIID riid, void **ret_iface, IUnknown *pUnkOuter)
{
    DirectMusicScriptTrack *track;
    HRESULT hr;

    *ret_iface = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->IPersistStream_iface.lpVtbl = &persist_vtbl;
    track->desc.dwSize = sizeof(track->desc);
    track->desc.dwValidData = DMUS_OBJ_CLASS;
    track->desc.guidClass = CLSID_DirectMusicScriptTrack;
    track->ref = 1;

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, riid, ret_iface);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
