/* IDirectMusicTempoTrack Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2004 Raphael Junqueira
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
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicTempoTrack implementation
 */

struct tempo_entry
{
    struct list entry;
    DMUS_IO_TEMPO_ITEM item;
};

typedef struct IDirectMusicTempoTrack {
    IDirectMusicTrack8 IDirectMusicTrack8_iface;
    struct dmobject dmobj;  /* IPersistStream only */
    LONG ref;
    struct list items;
} IDirectMusicTempoTrack;

/* IDirectMusicTempoTrack IDirectMusicTrack8 part: */
static inline IDirectMusicTempoTrack *impl_from_IDirectMusicTrack8(IDirectMusicTrack8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTempoTrack, IDirectMusicTrack8_iface);
}

static HRESULT WINAPI tempo_track_QueryInterface(IDirectMusicTrack8 *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

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

static ULONG WINAPI tempo_track_AddRef(IDirectMusicTrack8 *iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI tempo_track_Release(IDirectMusicTrack8 *iface)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        struct tempo_entry *item, *next_item;
        LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &This->items, struct tempo_entry, entry)
        {
            list_remove(&item->entry);
            free(item);
        }
        free(This);
    }

    return ref;
}

static HRESULT WINAPI tempo_track_Init(IDirectMusicTrack8 *iface, IDirectMusicSegment *pSegment)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  TRACE("(%p, %p): nothing to do here\n", This, pSegment);
  return S_OK;
}

static HRESULT WINAPI tempo_track_InitPlay(IDirectMusicTrack8 *iface,
        IDirectMusicSegmentState *pSegmentState, IDirectMusicPerformance *pPerformance,
        void **ppStateData, DWORD dwVirtualTrack8ID, DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = NULL;

  FIXME("(%p, %p, %p, %p, %ld, %ld): semi-stub\n", This, pSegmentState, pPerformance, ppStateData, dwVirtualTrack8ID, dwFlags);

  if (!(pState = calloc(1, sizeof(*pState)))) return E_OUTOFMEMORY;

  /** TODO real fill useful data */
  pState->dummy = 0;
  *ppStateData = pState;
  return S_OK;
}

static HRESULT WINAPI tempo_track_EndPlay(IDirectMusicTrack8 *iface, void *pStateData)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  LPDMUS_PRIVATE_TEMPO_PLAY_STATE pState = pStateData;

  FIXME("(%p, %p): semi-stub\n", This, pStateData);

  if (NULL == pStateData) {
    return E_POINTER;
  }
  /** TODO real clean up */
  free(pState);
  return S_OK;
}

static HRESULT WINAPI tempo_track_Play(IDirectMusicTrack8 *iface, void *pStateData,
        MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, %ld, %ld, %ld, %ld, %p, %p, %ld): stub\n", This, pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
  /** should use IDirectMusicPerformance_SendPMsg here */
  return S_OK;
}

static HRESULT WINAPI tempo_track_GetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME music_time,
        MUSIC_TIME *out_next_time, void *param)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
    struct tempo_entry *item, *next_item;
    DMUS_TEMPO_PARAM *tempo = param;
    MUSIC_TIME next_time = 0;

    TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(type), music_time, out_next_time, param);

    if (!param) return E_POINTER;
    if (!IsEqualGUID(type, &GUID_TempoParam)) return DMUS_E_GET_UNSUPPORTED;
    if (list_empty(&This->items)) return DMUS_E_NOT_FOUND;

    /* if music_time is before the first item, we return the first item. */
    LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &This->items, struct tempo_entry, entry)
    {
        tempo->mtTime = item->item.lTime - music_time;
        tempo->dblTempo = item->item.dblTempo;

        next_time = tempo->mtTime;
        if (next_time > 0) break;
        next_time = 0;
        if (item->entry.next == &This->items) break;
        next_time = next_item->item.lTime - music_time;
        if (next_time > 0) break;
    }

    if (out_next_time) *out_next_time = next_time;
    return S_OK;
}

static HRESULT WINAPI tempo_track_SetParam(IDirectMusicTrack8 *iface, REFGUID type, MUSIC_TIME time,
        void *param)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s, %ld, %p)\n", This, debugstr_dmguid(type), time, param);

    if (IsEqualGUID(type, &GUID_DisableTempo)) {
        if (!param)
            return DMUS_E_TYPE_DISABLED;
        FIXME("GUID_DisableTempo not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_EnableTempo)) {
        if (!param)
            return DMUS_E_TYPE_DISABLED;
        FIXME("GUID_EnableTempo not handled yet\n");
        return S_OK;
    }
    if (IsEqualGUID(type, &GUID_TempoParam)) {
        struct tempo_entry *item, *next_item;
        DMUS_TEMPO_PARAM *tempo_param = param;
        struct tempo_entry *entry;
        if (!param)
            return E_POINTER;
        if (!(entry = calloc(1, sizeof(*entry))))
            return E_OUTOFMEMORY;
        entry->item.lTime = time;
        entry->item.dblTempo = tempo_param->dblTempo;
        if (list_empty(&This->items))
            list_add_tail(&This->items, &entry->entry);
        else
        {
            LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &This->items, struct tempo_entry, entry)
                if (item->entry.next == &This->items || next_item->item.lTime > time)
                {
                    list_add_after(&item->entry, &entry->entry);
                    break;
                }
        }
        return S_OK;
    }

    return DMUS_E_SET_UNSUPPORTED;
}

static HRESULT WINAPI tempo_track_IsParamSupported(IDirectMusicTrack8 *iface, REFGUID rguidType)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

  TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidType));
  if (IsEqualGUID (rguidType, &GUID_DisableTempo)
      || IsEqualGUID (rguidType, &GUID_EnableTempo)
      || IsEqualGUID (rguidType, &GUID_TempoParam)) {
    TRACE("param supported\n");
    return S_OK;
  }
  TRACE("param unsupported\n");
  return DMUS_E_TYPE_UNSUPPORTED;
}

static HRESULT WINAPI tempo_track_AddNotificationType(IDirectMusicTrack8 *iface, REFGUID notiftype)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_RemoveNotificationType(IDirectMusicTrack8 *iface,
        REFGUID notiftype)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %s): method not implemented\n", This, debugstr_dmguid(notiftype));
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_Clone(IDirectMusicTrack8 *iface, MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd, IDirectMusicTrack **ppTrack)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %ld, %ld, %p): stub\n", This, mtStart, mtEnd, ppTrack);
  return S_OK;
}

static HRESULT WINAPI tempo_track_PlayEx(IDirectMusicTrack8 *iface, void *pStateData,
        REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, REFERENCE_TIME rtOffset, DWORD dwFlags,
        IDirectMusicPerformance *pPerf, IDirectMusicSegmentState *pSegSt, DWORD dwVirtualID)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, 0x%s, 0x%s, 0x%s, %ld, %p, %p, %ld): stub\n", This, pStateData, wine_dbgstr_longlong(rtStart),
      wine_dbgstr_longlong(rtEnd), wine_dbgstr_longlong(rtOffset), dwFlags, pPerf, pSegSt, dwVirtualID);
  return S_OK;
}

static HRESULT WINAPI tempo_track_GetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtNext, void *pParam, void *pStateData,
        DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), prtNext, pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI tempo_track_SetParamEx(IDirectMusicTrack8 *iface, REFGUID rguidType,
        REFERENCE_TIME rtTime, void *pParam, void *pStateData, DWORD dwFlags)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %s, 0x%s, %p, %p, %ld): stub\n", This, debugstr_dmguid(rguidType),
      wine_dbgstr_longlong(rtTime), pParam, pStateData, dwFlags);
  return S_OK;
}

static HRESULT WINAPI tempo_track_Compose(IDirectMusicTrack8 *iface, IUnknown *context,
        DWORD trackgroup, IDirectMusicTrack **track)
{
    IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);

    TRACE("(%p, %p, %ld, %p): method not implemented\n", This, context, trackgroup, track);
    return E_NOTIMPL;
}

static HRESULT WINAPI tempo_track_Join(IDirectMusicTrack8 *iface, IDirectMusicTrack *pNewTrack,
        MUSIC_TIME mtJoin, IUnknown *pContext, DWORD dwTrackGroup,
        IDirectMusicTrack **ppResultTrack)
{
  IDirectMusicTempoTrack *This = impl_from_IDirectMusicTrack8(iface);
  FIXME("(%p, %p, %ld, %p, %ld, %p): stub\n", This, pNewTrack, mtJoin, pContext, dwTrackGroup, ppResultTrack);
  return S_OK;
}

static const IDirectMusicTrack8Vtbl dmtrack8_vtbl = {
    tempo_track_QueryInterface,
    tempo_track_AddRef,
    tempo_track_Release,
    tempo_track_Init,
    tempo_track_InitPlay,
    tempo_track_EndPlay,
    tempo_track_Play,
    tempo_track_GetParam,
    tempo_track_SetParam,
    tempo_track_IsParamSupported,
    tempo_track_AddNotificationType,
    tempo_track_RemoveNotificationType,
    tempo_track_Clone,
    tempo_track_PlayEx,
    tempo_track_GetParamEx,
    tempo_track_SetParamEx,
    tempo_track_Compose,
    tempo_track_Join
};

static inline IDirectMusicTempoTrack *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicTempoTrack, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI tempo_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicTempoTrack *This = impl_from_IPersistStream(iface);
    struct chunk_entry chunk = {0};
    DMUS_IO_TEMPO_ITEM *items;
    ULONG i = 0;
    HRESULT hr;
    UINT count;

    TRACE("%p, %p\n", This, stream);

    if (!stream)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &chunk)) != S_OK)
        return hr;
    if (chunk.id != DMUS_FOURCC_TEMPO_TRACK)
        return DMUS_E_UNSUPPORTED_STREAM;

    hr = stream_chunk_get_array(stream, &chunk, (void **)&items, &count,
            sizeof(DMUS_IO_TEMPO_ITEM));
    if (FAILED(hr))
        return hr;

    for (i = 0; i < count; i++)
    {
        struct tempo_entry *entry = calloc(1, sizeof(*entry));
        if (!entry)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        entry->item = items[i];
        list_add_tail(&This->items, &entry->entry);

        TRACE_(dmfile)("DMUS_IO_TEMPO_ITEM #%lu\n", i);
        TRACE_(dmfile)(" - lTime = %lu\n", items[i].lTime);
        TRACE_(dmfile)(" - dblTempo = %g\n", items[i].dblTempo);
    }

    free(items);
    return hr;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    tempo_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmtempotrack(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicTempoTrack *track;
    HRESULT hr;

    *ppobj = NULL;
    if (!(track = calloc(1, sizeof(*track)))) return E_OUTOFMEMORY;
    track->IDirectMusicTrack8_iface.lpVtbl = &dmtrack8_vtbl;
    track->ref = 1;
    dmobject_init(&track->dmobj, &CLSID_DirectMusicTempoTrack,
                  (IUnknown *)&track->IDirectMusicTrack8_iface);
    track->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&track->items);

    hr = IDirectMusicTrack8_QueryInterface(&track->IDirectMusicTrack8_iface, lpcGUID, ppobj);
    IDirectMusicTrack8_Release(&track->IDirectMusicTrack8_iface);

    return hr;
}
