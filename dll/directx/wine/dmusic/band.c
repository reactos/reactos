/* IDirectMusicBand Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmband);

void dump_DMUS_IO_INSTRUMENT(DMUS_IO_INSTRUMENT *inst)
{
    TRACE("DMUS_IO_INSTRUMENT: %p\n", inst);
    TRACE(" - dwPatch: %lu\n", inst->dwPatch);
    TRACE(" - dwAssignPatch: %lu\n", inst->dwAssignPatch);
    TRACE(" - dwNoteRanges[0]: %lu\n", inst->dwNoteRanges[0]);
    TRACE(" - dwNoteRanges[1]: %lu\n", inst->dwNoteRanges[1]);
    TRACE(" - dwNoteRanges[2]: %lu\n", inst->dwNoteRanges[2]);
    TRACE(" - dwNoteRanges[3]: %lu\n", inst->dwNoteRanges[3]);
    TRACE(" - dwPChannel: %lu\n", inst->dwPChannel);
    TRACE(" - dwFlags: %lx\n", inst->dwFlags);
    TRACE(" - bPan: %u\n", inst->bPan);
    TRACE(" - bVolume: %u\n", inst->bVolume);
    TRACE(" - nTranspose: %d\n", inst->nTranspose);
    TRACE(" - dwChannelPriority: %lu\n", inst->dwChannelPriority);
    TRACE(" - nPitchBendRange: %d\n", inst->nPitchBendRange);
}

struct instrument_entry
{
    struct list entry;
    DMUS_IO_INSTRUMENT instrument;
    IDirectMusicCollection *collection;

    IDirectMusicDownloadedInstrument *download;
    IDirectMusicPort *download_port;
};

static HRESULT instrument_entry_unload(struct instrument_entry *entry)
{
    HRESULT hr;

    if (!entry->download) return S_OK;

    if (FAILED(hr = IDirectMusicPort_UnloadInstrument(entry->download_port, entry->download)))
        WARN("Failed to unload entry instrument, hr %#lx\n", hr);
    IDirectMusicDownloadedInstrument_Release(entry->download);
    entry->download = NULL;
    IDirectMusicPort_Release(entry->download_port);
    entry->download_port = NULL;

    return hr;
}

static void instrument_entry_destroy(struct instrument_entry *entry)
{
    instrument_entry_unload(entry);
    if (entry->collection) IDirectMusicCollection_Release(entry->collection);
    free(entry);
}

struct band
{
    IDirectMusicBand IDirectMusicBand_iface;
    struct dmobject dmobj;
    LONG ref;
    struct list instruments;
    IDirectMusicCollection *collection;
};

static inline struct band *impl_from_IDirectMusicBand(IDirectMusicBand *iface)
{
    return CONTAINING_RECORD(iface, struct band, IDirectMusicBand_iface);
}

static HRESULT WINAPI band_QueryInterface(IDirectMusicBand *iface, REFIID riid,
        void **ret_iface)
{
    struct band *This = impl_from_IDirectMusicBand(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicBand))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IDirectMusicBand_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI band_AddRef(IDirectMusicBand *iface)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI band_Release(IDirectMusicBand *iface)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        struct instrument_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->instruments, struct instrument_entry, entry)
        {
            list_remove(&entry->entry);
            instrument_entry_destroy(entry);
        }

        if (This->collection) IDirectMusicCollection_Release(This->collection);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI band_CreateSegment(IDirectMusicBand *iface,
        IDirectMusicSegment **segment)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    HRESULT hr;
    DMUS_BAND_PARAM bandparam;

    FIXME("(%p, %p): semi-stub\n", This, segment);

    hr = CoCreateInstance(&CLSID_DirectMusicSegment, NULL, CLSCTX_INPROC,
            &IID_IDirectMusicSegment, (void**)segment);
    if (FAILED(hr))
        return hr;

    bandparam.mtTimePhysical = 0;
    bandparam.pBand = &This->IDirectMusicBand_iface;
    IDirectMusicBand_AddRef(bandparam.pBand);
    hr = IDirectMusicSegment_SetParam(*segment, &GUID_BandParam, 0xffffffff, DMUS_SEG_ALLTRACKS,
            0, &bandparam);
    IDirectMusicBand_Release(bandparam.pBand);

    return hr;
}

static HRESULT WINAPI band_Download(IDirectMusicBand *iface,
        IDirectMusicPerformance *performance)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    struct instrument_entry *entry;
    HRESULT hr = S_OK;

    TRACE("(%p, %p)\n", This, performance);

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
    {
        IDirectMusicCollection *collection;
        IDirectMusicInstrument *instrument;

        if (FAILED(hr = instrument_entry_unload(entry))) break;
        if (!(collection = entry->collection) && !(collection = This->collection)) continue;

        if (SUCCEEDED(hr = IDirectMusicCollection_GetInstrument(collection, entry->instrument.dwPatch, &instrument)))
        {
            hr = IDirectMusicPerformance_DownloadInstrument(performance, instrument, entry->instrument.dwPChannel,
                        &entry->download, NULL, 0, &entry->download_port, NULL, NULL);
            IDirectMusicInstrument_Release(instrument);
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) WARN("Failed to download instruments, hr %#lx\n", hr);
    return hr;
}

static HRESULT WINAPI band_Unload(IDirectMusicBand *iface, IDirectMusicPerformance *performance)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    struct instrument_entry *entry;
    HRESULT hr = S_OK;

	TRACE("(%p, %p)\n", This, performance);

    if (performance) FIXME("performance parameter not implemented\n");

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
        if (FAILED(hr = instrument_entry_unload(entry))) break;

    if (FAILED(hr)) WARN("Failed to unload instruments, hr %#lx\n", hr);
    return hr;
}

static const IDirectMusicBandVtbl band_vtbl =
{
    band_QueryInterface,
    band_AddRef,
    band_Release,
    band_CreateSegment,
    band_Download,
    band_Unload,
};

static HRESULT parse_lbin_list(struct band *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    IDirectMusicCollection *collection = NULL;
    struct instrument_entry *entry;
    DMUS_IO_INSTRUMENT inst = {0};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_INSTRUMENT_CHUNK:
        {
            UINT size = sizeof(inst);
            if (chunk.size == offsetof(DMUS_IO_INSTRUMENT, nPitchBendRange)) size = chunk.size;
            if (FAILED(hr = stream_chunk_get_data(stream, &chunk, &inst, size))) break;
            break;
        }

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_REF_LIST):
        {
            IDirectMusicObject *object;
            if (FAILED(hr = dmobj_parsereference(stream, &chunk, &object))) break;
            hr = IDirectMusicObject_QueryInterface(object, &IID_IDirectMusicCollection, (void **)&collection);
            IDirectMusicObject_Release(object);
            break;
        }

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) return hr;

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    memcpy(&entry->instrument, &inst, sizeof(DMUS_IO_INSTRUMENT));
    entry->collection = collection;
    list_add_tail(&This->instruments, &entry->entry);

    return hr;
}

static HRESULT parse_lbil_list(struct band *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_INSTRUMENT_LIST):
            hr = parse_lbin_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_dmbd_chunk(struct band *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &This->dmobj.desc,
                DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION))
                || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_GUID_CHUNK:
        case DMUS_FOURCC_VERSION_CHUNK:
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_INSTRUMENTS_LIST):
            hr = parse_lbil_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT WINAPI band_object_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    STATSTG stat;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_BAND_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_INVALID_BAND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicBand;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    if (desc->dwValidData & DMUS_OBJ_CATEGORY) {
        IStream_Stat(stream, &stat, STATFLAG_NONAME);
        desc->ftDate = stat.mtime;
        desc->dwValidData |= DMUS_OBJ_DATE;
    }

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl band_object_vtbl =
{
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    band_object_ParseDescriptor,
};

static inline struct band *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct band, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI band_persist_stream_Load(IPersistStream *iface, IStream *stream)
{
    struct band *This = impl_from_IPersistStream(iface);
    DMUS_OBJECTDESC default_desc =
    {
        .dwSize = sizeof(DMUS_OBJECTDESC),
        .dwValidData = DMUS_OBJ_OBJECT | DMUS_OBJ_CLASS,
        .guidClass = CLSID_DirectMusicCollection,
        .guidObject = GUID_DefaultGMCollection,
    };
    struct chunk_entry chunk = {0};
    HRESULT hr;

    TRACE("%p, %p\n", iface, stream);

    if (This->collection) IDirectMusicCollection_Release(This->collection);
    if (FAILED(hr = stream_get_object(stream, &default_desc, &IID_IDirectMusicCollection,
            (void **)&This->collection)))
        WARN("Failed to load default collection from loader, hr %#lx\n", hr);

    if ((hr = stream_get_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_BAND_FORM):
            hr = parse_dmbd_chunk(This, stream, &chunk);
            break;

        default:
            WARN("Invalid band chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            hr = DMUS_E_UNSUPPORTED_STREAM;
            break;
        }
    }

    stream_skip_chunk(stream, &chunk);
    if (FAILED(hr)) return hr;

    if (TRACE_ON(dmband))
    {
        struct instrument_entry *entry;

        TRACE("Loaded IDirectMusicBand %p\n", This);
        dump_DMUS_OBJECTDESC(&This->dmobj.desc);

        TRACE(" - Instruments:\n");
        LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
        {
            dump_DMUS_IO_INSTRUMENT(&entry->instrument);
            TRACE(" - collection: %p\n", entry->collection);
        }
    }

    return S_OK;
}

static const IPersistStreamVtbl band_persist_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    unimpl_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    band_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

HRESULT create_dmband(REFIID lpcGUID, void **ppobj)
{
  struct band* obj;
  HRESULT hr;

  *ppobj = NULL;
  if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
  obj->IDirectMusicBand_iface.lpVtbl = &band_vtbl;
  obj->ref = 1;
  dmobject_init(&obj->dmobj, &CLSID_DirectMusicBand, (IUnknown *)&obj->IDirectMusicBand_iface);
  obj->dmobj.IDirectMusicObject_iface.lpVtbl = &band_object_vtbl;
  obj->dmobj.IPersistStream_iface.lpVtbl = &band_persist_stream_vtbl;
  list_init(&obj->instruments);

  hr = IDirectMusicBand_QueryInterface(&obj->IDirectMusicBand_iface, lpcGUID, ppobj);
  IDirectMusicBand_Release(&obj->IDirectMusicBand_iface);

  return hr;
}

HRESULT band_connect_to_collection(IDirectMusicBand *iface, IDirectMusicCollection *collection)
{
    struct band *This = impl_from_IDirectMusicBand(iface);

    TRACE("%p, %p\n", iface, collection);

    if (This->collection) IDirectMusicCollection_Release(This->collection);
    if ((This->collection = collection)) IDirectMusicCollection_AddRef(This->collection);

    return S_OK;
}

HRESULT band_send_messages(IDirectMusicBand *iface, IDirectMusicPerformance *performance,
        IDirectMusicGraph *graph, MUSIC_TIME time, DWORD track_id)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    struct instrument_entry *entry;
    HRESULT hr = S_OK;

    LIST_FOR_EACH_ENTRY_REV(entry, &This->instruments, struct instrument_entry, entry)
    {
        DWORD patch = entry->instrument.dwPatch;
        DMUS_PATCH_PMSG *msg;

        if (FAILED(hr = IDirectMusicPerformance_AllocPMsg(performance, sizeof(*msg),
                (DMUS_PMSG **)&msg)))
            break;

        msg->mtTime = time;
        msg->dwFlags = DMUS_PMSGF_MUSICTIME;
        msg->dwPChannel = entry->instrument.dwPChannel;
        msg->dwVirtualTrackID = track_id;
        msg->dwType = DMUS_PMSGT_PATCH;
        msg->dwGroupID = 1;
        msg->byInstrument = entry->instrument.dwPatch;

        msg->byInstrument = patch & 0x7F;
        patch >>= 8;
        msg->byLSB = patch & 0x7f;
        patch >>= 8;
        msg->byMSB = patch & 0x7f;
        patch >>= 8;

        if (FAILED(hr = IDirectMusicGraph_StampPMsg(graph, (DMUS_PMSG *)msg))
                || FAILED(hr = IDirectMusicPerformance_SendPMsg(performance, (DMUS_PMSG *)msg)))
        {
            IDirectMusicPerformance_FreePMsg(performance, (DMUS_PMSG *)msg);
            break;
        }
    }

    return hr;
}

HRESULT band_add_instrument(IDirectMusicBand *iface, DMUS_IO_INSTRUMENT *instrument)
{
    struct band *This = impl_from_IDirectMusicBand(iface);
    struct instrument_entry *entry;

    TRACE("%p, %p\n", iface, instrument);

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    entry->instrument = *instrument;
    list_add_tail(&This->instruments, &entry->entry);

    return S_OK;
}
