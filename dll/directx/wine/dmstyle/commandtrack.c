/* IDirectMusicCommandTrack Implementation
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
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicCommandTrack implementation
 */
typedef struct IDirectMusicCommandTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
    struct list Commands;
} IDirectMusicCommandTrack;

/* IDirectMusicCommandTrack IDirectMusicTrack8 part: */
static inline IDirectMusicCommandTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicCommandTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI command_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI command_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI command_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI command_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI command_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI command_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI command_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI command_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p):\n", This, debugstr_dmguid(type), time, next, param);

    if (!type)
        return E_POINTER;

    if (IsEqualGUID(type, &GUID_CommandParam)) {
        FIXME("GUID_CommandParam not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_CommandParam2)) {
        FIXME("GUID_CommandParam2 not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_CommandParamNext)) {
        FIXME("GUID_CommandParamNext not handled yet\n");
        return S_OK;
    }

    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI command_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (!type)
        return E_POINTER;

    if (IsEqualGUID(type, &GUID_CommandParam)) {
        FIXME("GUID_CommandParam not handled yet\n");
        return S_OK;
    } else if (IsEqualGUID(type, &GUID_CommandParamNext)) {
        FIXME("GUID_CommandParamNext not handled yet\n");
        return S_OK;
    }

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI command_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);

	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));

        if (!rguidType)
                return E_POINTER;

	if (IsEqualGUID (rguidType, &GUID_CommandParam)
		|| IsEqualGUID (rguidType, &GUID_CommandParam2)
		|| IsEqualGUID (rguidType, &GUID_CommandParamNext)) {
		TRACE("param supported\n");
		return S_OK;
	}
	
	TRACE("param unsupported\n");
	return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI command_track_AddNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI command_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID rguidNotificationType)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s): stub\n", This, debugstr_dmguid(rguidNotificationType));
	return S_OK;
}

static HRESULT WINAPI command_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI command_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI command_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam,
        void *pStateData, DWORD dwFlags)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI command_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI command_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
        IDirectMusicCommandTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
	return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    command_track_QueryInterface,
    command_track_AddRef,
    command_track_Release,
    command_track_Init,
    command_track_InitPlay,
    command_track_EndPlay,
    command_track_Play,
    command_track_GetParam,
    command_track_SetParam,
    command_track_IsParamSupported,
    command_track_AddNotificationType,
    command_track_RemoveNotificationType,
    command_track_Clone,
    command_track_PlayEx,
    command_track_GetParamEx,
    command_track_SetParamEx,
    command_track_Compose,
    command_track_Join
};

static inline IDirectMusicCommandTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicCommandTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
        IDirectMusicCommandTrack *This = impl_from_IPersistStream(iface);
	FOURCC chunkID;
	DWORD chunkSize, dwSizeOfStruct, nrCommands;
	LARGE_INTEGER liMove; /* used when skipping chunks */
	
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {
		case DMUS_FOURCC_COMMANDTRACK_CHUNK: {
			DWORD count;
			TRACE_(dmfile)(": command track chunk\n");
			IStream_Read (pStm, &dwSizeOfStruct, sizeof(DWORD), NULL);
			if (dwSizeOfStruct != sizeof(DMUS_IO_COMMAND)) {
				TRACE_(dmfile)(": declared size of struct (=%ld) != actual size; indicates older direct music version\n", dwSizeOfStruct);
			}
			chunkSize -= sizeof(DWORD); /* now chunk size is one DWORD shorter */
			nrCommands = chunkSize/dwSizeOfStruct; /* and this is the number of commands */
			/* load each command separately in new entry */
			for (count = 0; count < nrCommands; count++) {
				LPDMUS_PRIVATE_COMMAND pNewCommand = calloc(1, sizeof(*pNewCommand));
				IStream_Read (pStm, &pNewCommand->pCommand, dwSizeOfStruct, NULL);
				list_add_tail (&This->Commands, &pNewCommand->entry);
			}
			TRACE_(dmfile)(": reading finished\n");
			This->dmobj.desc.dwValidData |= DMUS_OBJ_LOADED;
			break;
		}	
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = chunkSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	/* DEBUG: dumps whole band track object tree: */
	if (TRACE_ON(dmstyle)) {
		int r = 0;
		DMUS_PRIVATE_COMMAND *tmpEntry;
		struct list *listEntry;
		TRACE("*** IDirectMusicCommandTrack (%p) ***\n", This);
		TRACE(" - Commands:\n");
		LIST_FOR_EACH (listEntry, &This->Commands) {
			tmpEntry = LIST_ENTRY (listEntry, DMUS_PRIVATE_COMMAND, entry);
			TRACE("    - Command[%i]:\n", r);
			TRACE("       - mtTime = %li\n", tmpEntry->pCommand.mtTime);
			TRACE("       - wMeasure = %d\n", tmpEntry->pCommand.wMeasure);
			TRACE("       - bBeat = %i\n", tmpEntry->pCommand.bBeat);
			TRACE("       - bCommand = %i\n", tmpEntry->pCommand.bCommand);
			TRACE("       - bGrooveLevel = %i\n", tmpEntry->pCommand.bGrooveLevel);
			TRACE("       - bGrooveRange = %i\n", tmpEntry->pCommand.bGrooveRange);
			TRACE("       - bRepeatMode = %i\n", tmpEntry->pCommand.bRepeatMode);			
			r++;
		}
	}

	return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_Save(IPersistStream *iface, IStream *stream,
        BOOL cleardirty)
{
    IDirectMusicCommandTrack *This = impl_from_IPersistStream(iface);

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
HRESULT create_dmcommandtrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicCommandTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicCommandTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init (&track->Commands);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface,
                                           lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
