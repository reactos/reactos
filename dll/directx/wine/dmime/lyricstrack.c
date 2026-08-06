/* IDirectMusicLyricsTrack Implementation
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
 * IDirectMusicLyricsTrack implementation
 */
typedef struct IDirectMusicLyricsTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
} IDirectMusicLyricsTrack;

/* IDirectMusicLyricsTrack IDirectMusicTrack8 part: */
static inline IDirectMusicLyricsTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicLyricsTrack, IDirectMusicTrack8_iface);
}

static inline IDirectMusicLyricsTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicLyricsTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI lyrics_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI lyrics_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI lyrics_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI lyrics_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pSegment);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %p, %p, %ld, %ld): stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p): stub\n", This, pStateData);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, MUSIC_TIME *next, void *param)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p, %p): not supported\n", This, debugstr_dmguid(type), time, next, param);
    return DMUS_E_GET_UNSUPPORTED;
}

static HRESULT WINAPI lyrics_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type,
        MUSIC_TIME time, void *param)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p): not supported\n", This, debugstr_dmguid(type), time, param);
    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI lyrics_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID  type)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): param type not supported\n", This, debugstr_dmguid(type));
    return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI lyrics_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI lyrics_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI lyrics_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
	    wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
        IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
	FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
	    wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
	return S_OK;
}

static HRESULT WINAPI lyrics_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI lyrics_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *newtrack,
        MUSIC_TIME join, IUnknown *context, DWORD trackgroup, IDirectMusicTrack **resulttrack)
{
    IDirectMusicLyricsTrack *This = impl_from_IDirectMusicTrack8(iface);
    TRACE("(%p, %p, %ld, %p, %ld, %p): method not implemented\n", This, newtrack, join, context,
            trackgroup, resulttrack);
    return E_NOTIMPL;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    lyrics_track_QueryInterface,
    lyrics_track_AddRef,
    lyrics_track_Release,
    lyrics_track_Init,
    lyrics_track_InitPlay,
    lyrics_track_EndPlay,
    lyrics_track_Play,
    lyrics_track_GetParam,
    lyrics_track_SetParam,
    lyrics_track_IsParamSupported,
    lyrics_track_AddNotificationType,
    lyrics_track_RemoveNotificationType,
    lyrics_track_Clone,
    lyrics_track_PlayEx,
    lyrics_track_GetParamEx,
    lyrics_track_SetParamEx,
    lyrics_track_Compose,
    lyrics_track_Join
};

static HRESULT parse_lyrics_track_events(IDirectMusicLyricsTrack *This, IStream *stream,
                struct chunk_entry *lyric)
{
    struct chunk_entry chunk = {.parent = lyric};
    HRESULT hr;
    DMUS_IO_LYRICSTRACK_EVENTHEADER header;
    WCHAR name[256];

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(lyric));

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK) {
        if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_LYRICSTRACKEVENT_LIST) {
            struct chunk_entry child = {.parent = &chunk};

            if (FAILED(hr = stream_next_chunk(stream, &child)))
                return  hr;

            if (child.id != DMUS_FOURCC_LYRICSTRACKEVENTHEADER_CHUNK)
                return DMUS_E_UNSUPPORTED_STREAM;

            if (FAILED(hr = stream_chunk_get_data(stream, &child, &header, child.size))) {
                WARN("Failed to read data of %s\n", debugstr_chunk(&child));
                return hr;
            }

            TRACE("Found DMUS_IO_LYRICSTRACK_EVENTHEADER\n");
            TRACE("  - dwFlags %#lx\n", header.dwFlags);
            TRACE("  - dwTimingFlags %#lx\n", header.dwTimingFlags);
            TRACE("  - lTimeLogical %ld\n", header.lTimeLogical);
            TRACE("  - lTimePhysical %ld\n", header.lTimePhysical);

            if (FAILED(hr = stream_next_chunk(stream, &child)))
                return  hr;

            if (child.id != DMUS_FOURCC_LYRICSTRACKEVENTTEXT_CHUNK)
                return DMUS_E_UNSUPPORTED_STREAM;

            if (FAILED(hr = stream_chunk_get_data(stream, &child, &name, child.size))) {
                WARN("Failed to read data of %s\n", debugstr_chunk(&child));
                return hr;
            }

            TRACE("Found DMUS_FOURCC_LYRICSTRACKEVENTTEXT_CHUNK\n");
            TRACE("  - name %s\n", debugstr_w(name));
        }
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

static HRESULT parse_lyricstrack_list(IDirectMusicLyricsTrack *This, IStream *stream, struct chunk_entry *lyric)
{
    HRESULT hr;
    struct chunk_entry chunk = {.parent = lyric};

    TRACE("Parsing segment form in %p: %s\n", stream, debugstr_chunk(lyric));

    if (FAILED(hr = stream_next_chunk(stream, &chunk)))
        return hr;

    if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_LYRICSTRACKEVENTS_LIST)
        hr = parse_lyrics_track_events(This, stream, &chunk);
    else
        hr = DMUS_E_UNSUPPORTED_STREAM;

    return SUCCEEDED(hr) ? S_OK : hr;
}

static HRESULT WINAPI lyrics_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicLyricsTrack *This = impl_from_IPersistStream(iface);
    HRESULT hr;
    struct chunk_entry chunk = {0};

    TRACE("%p, %p\n", This, stream);

    if (!stream)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &chunk)) != S_OK)
        return hr;

    if (chunk.id == FOURCC_LIST && chunk.type == DMUS_FOURCC_LYRICSTRACK_LIST)
        hr = parse_lyricstrack_list(This, stream, &chunk);
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
    lyrics_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmlyricstrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicLyricsTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicLyricsTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
