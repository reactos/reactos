/*
 * IDirectMusicCollection Implementation
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
#include "soundfont.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

struct instrument_entry
{
    struct list entry;
    DWORD patch;
    DMUS_OBJECTDESC desc;
    IDirectMusicInstrument *instrument;
};

struct pool
{
    POOLTABLE table;
    POOLCUE cues[];
};

C_ASSERT(sizeof(struct pool) == offsetof(struct pool, cues[0]));

struct wave_entry
{
    struct list entry;
    IDirectMusicObject *wave;
    DWORD offset;
};

struct collection
{
    IDirectMusicCollection IDirectMusicCollection_iface;
    struct dmobject dmobj;
    LONG internal_ref;
    LONG ref;

    DLSHEADER header;
    struct pool *pool;
    struct list instruments;
    struct list waves;
};

void collection_internal_addref(struct collection *collection)
{
    ULONG ref = InterlockedIncrement( &collection->internal_ref );
    TRACE( "collection %p, internal ref %lu.\n", collection, ref );
}

void collection_internal_release(struct collection *collection)
{
    ULONG ref = InterlockedDecrement( &collection->internal_ref );
    TRACE( "collection %p, internal ref %lu.\n", collection, ref );

    if (!ref)
        free(collection);
}

HRESULT collection_get_wave(struct collection *collection, DWORD index, IDirectMusicObject **out)
{
    struct wave_entry *wave_entry;
    DWORD offset;

    if (index >= collection->pool->table.cCues) return E_INVALIDARG;
    offset = collection->pool->cues[index].ulOffset;

    LIST_FOR_EACH_ENTRY(wave_entry, &collection->waves, struct wave_entry, entry)
    {
        if (offset == wave_entry->offset)
        {
            *out = wave_entry->wave;
            IUnknown_AddRef(wave_entry->wave);
            return S_OK;
        }
    }

    return E_FAIL;
}

static inline struct collection *impl_from_IDirectMusicCollection(IDirectMusicCollection *iface)
{
    return CONTAINING_RECORD(iface, struct collection, IDirectMusicCollection_iface);
}

static inline struct collection *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct collection, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI collection_QueryInterface(IDirectMusicCollection *iface,
        REFIID riid, void **ret_iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicCollection))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else
    {
        WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI collection_AddRef(IDirectMusicCollection *iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI collection_Release(IDirectMusicCollection *iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref)
    {
        struct instrument_entry *instrument_entry;
        struct wave_entry *wave_entry;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(instrument_entry, next, &This->instruments, struct instrument_entry, entry)
        {
            list_remove(&instrument_entry->entry);
            IDirectMusicInstrument_Release(instrument_entry->instrument);
            free(instrument_entry);
        }

        LIST_FOR_EACH_ENTRY_SAFE(wave_entry, next, &This->waves, struct wave_entry, entry)
        {
            list_remove(&wave_entry->entry);
            IDirectMusicInstrument_Release(wave_entry->wave);
            free(wave_entry);
        }

        collection_internal_release(This);
    }

    return ref;
}

static HRESULT WINAPI collection_GetInstrument(IDirectMusicCollection *iface,
        DWORD patch, IDirectMusicInstrument **instrument)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    struct instrument_entry *entry;

    TRACE("(%p, %lu, %p)\n", iface, patch, instrument);

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
    {
        if (patch == entry->patch)
        {
            *instrument = entry->instrument;
            IDirectMusicInstrument_AddRef(entry->instrument);
            TRACE(": returning instrument %p\n", entry->instrument);
            return S_OK;
        }
    }

    TRACE(": instrument not found\n");
    return DMUS_E_INVALIDPATCH;
}

static HRESULT WINAPI collection_EnumInstrument(IDirectMusicCollection *iface,
        DWORD index, DWORD *patch, LPWSTR name, DWORD name_length)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    struct instrument_entry *entry;

    TRACE("(%p, %ld, %p, %p, %ld)\n", iface, index, patch, name, name_length);

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
    {
        if (index--) continue;
        *patch = entry->patch;
        if (name) lstrcpynW(name, entry->desc.wszName, name_length);
        return S_OK;
    }

    return S_FALSE;
}

static const IDirectMusicCollectionVtbl collection_vtbl =
{
    collection_QueryInterface,
    collection_AddRef,
    collection_Release,
    collection_GetInstrument,
    collection_EnumInstrument,
};

static HRESULT parse_lins_list(struct collection *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct instrument_entry *entry;
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_INS):
            if (!(entry = malloc(sizeof(*entry)))) return E_OUTOFMEMORY;
            hr = instrument_create_from_chunk(stream, &chunk, This, &entry->desc, &entry->instrument);
            if (SUCCEEDED(hr)) hr = IDirectMusicInstrument_GetPatch(entry->instrument, &entry->patch);
            if (SUCCEEDED(hr)) list_add_tail(&This->instruments, &entry->entry);
            else free(entry);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_wvpl_list(struct collection *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct wave_entry *entry;
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_wave):
            if (!(entry = malloc(sizeof(*entry)))) return E_OUTOFMEMORY;
            if (FAILED(hr = wave_create_from_chunk(stream, &chunk, &entry->wave))) free(entry);
            else
            {
                entry->offset = chunk.offset.QuadPart - parent->offset.QuadPart - 12;
                list_add_tail(&This->waves, &entry->entry);
            }
            break;

        default:
            FIXME("Skipping unknown chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_ptbl_chunk(struct collection *This, IStream *stream, struct chunk_entry *chunk)
{
    struct pool *pool;
    POOLTABLE table;
    HRESULT hr;
    UINT size;

    if (chunk->size < sizeof(table)) return E_INVALIDARG;
    if (FAILED(hr = stream_read(stream, &table, sizeof(table)))) return hr;
    if (chunk->size != table.cbSize + sizeof(POOLCUE) * table.cCues) return E_INVALIDARG;
    if (table.cbSize != sizeof(table)) return E_INVALIDARG;

    size = offsetof(struct pool, cues[table.cCues]);
    if (!(pool = malloc(size))) return E_OUTOFMEMORY;
    pool->table = table;

    size = sizeof(POOLCUE) * table.cCues;
    if (FAILED(hr = stream_read(stream, pool->cues, size))) free(pool);
    else This->pool = pool;

    return hr;
}

static HRESULT parse_dls_chunk(struct collection *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &This->dmobj.desc,
            DMUS_OBJ_NAME_INFO|DMUS_OBJ_VERSION|DMUS_OBJ_OBJECT|DMUS_OBJ_GUID_DLID))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_DLID:
        case FOURCC_VERS:
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_INFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case FOURCC_COLH:
            hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header));
            break;

        case FOURCC_PTBL:
            hr = parse_ptbl_chunk(This, stream, &chunk);
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LINS):
            hr = parse_lins_list(This, stream, &chunk);
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_WVPL):
            hr = parse_wvpl_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_sdta_list(struct collection *This, IStream *stream, struct chunk_entry *parent,
        struct soundfont *soundfont)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case mmioFOURCC('s','m','p','l'):
            if (soundfont->sdta) return E_INVALIDARG;
            if (!(soundfont->sdta = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->sdta, chunk.size);
            break;

        default:
            FIXME("Skipping unknown chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_pdta_list(struct collection *This, IStream *stream, struct chunk_entry *parent,
        struct soundfont *soundfont)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case mmioFOURCC('p','h','d','r'):
            if (soundfont->phdr) return E_INVALIDARG;
            if (!(soundfont->phdr = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->phdr, chunk.size);
            soundfont->preset_count = chunk.size / sizeof(*soundfont->phdr) - 1;
            break;

        case mmioFOURCC('p','b','a','g'):
            if (soundfont->pbag) return E_INVALIDARG;
            if (!(soundfont->pbag = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->pbag, chunk.size);
            break;

        case mmioFOURCC('p','m','o','d'):
            if (soundfont->pmod) return E_INVALIDARG;
            if (!(soundfont->pmod = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->pmod, chunk.size);
            break;

        case mmioFOURCC('p','g','e','n'):
            if (soundfont->pgen) return E_INVALIDARG;
            if (!(soundfont->pgen = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->pgen, chunk.size);
            break;

        case mmioFOURCC('i','n','s','t'):
            if (soundfont->inst) return E_INVALIDARG;
            if (!(soundfont->inst = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->inst, chunk.size);
            soundfont->instrument_count = chunk.size / sizeof(*soundfont->inst) - 1;
            break;

        case mmioFOURCC('i','b','a','g'):
            if (soundfont->ibag) return E_INVALIDARG;
            if (!(soundfont->ibag = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->ibag, chunk.size);
            break;

        case mmioFOURCC('i','m','o','d'):
            if (soundfont->imod) return E_INVALIDARG;
            if (!(soundfont->imod = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->imod, chunk.size);
            break;

        case mmioFOURCC('i','g','e','n'):
            if (soundfont->igen) return E_INVALIDARG;
            if (!(soundfont->igen = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->igen, chunk.size);
            break;

        case mmioFOURCC('s','h','d','r'):
            if (soundfont->shdr) return E_INVALIDARG;
            if (!(soundfont->shdr = malloc(chunk.size))) return E_OUTOFMEMORY;
            hr = stream_chunk_get_data(stream, &chunk, soundfont->shdr, chunk.size);
            soundfont->sample_count = chunk.size / sizeof(*soundfont->shdr) - 1;
            break;

        default:
            FIXME("Skipping unknown chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_sfbk_chunk(struct collection *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct soundfont soundfont = {0};
    UINT i, j, k;
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &This->dmobj.desc,
            DMUS_OBJ_NAME_INFO|DMUS_OBJ_VERSION|DMUS_OBJ_OBJECT|DMUS_OBJ_GUID_DLID))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_INFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case MAKE_IDTYPE(FOURCC_LIST, mmioFOURCC('s','d','t','a')):
            hr = parse_sdta_list(This, stream, &chunk, &soundfont);
            break;

        case MAKE_IDTYPE(FOURCC_LIST, mmioFOURCC('p','d','t','a')):
            hr = parse_pdta_list(This, stream, &chunk, &soundfont);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (SUCCEEDED(hr))
    {
        TRACE("presets:\n");
        for (i = 0; i < soundfont.preset_count; i++)
        {
            struct sf_preset *preset = soundfont.phdr + i;

            TRACE("preset[%u]:\n", i);
            TRACE("    - name: %s\n", debugstr_a(preset->name));
            TRACE("    - preset: %u\n", preset->preset);
            TRACE("    - bank: %u\n", preset->bank);
            TRACE("    - preset_bag_ndx: %u\n", preset->bag_ndx);
            TRACE("    - library: %lu\n", preset->library);
            TRACE("    - genre: %lu\n", preset->genre);
            TRACE("    - morphology: %#lx\n", preset->morphology);

            for (j = preset->bag_ndx; j < (preset + 1)->bag_ndx; j++)
            {
                struct sf_bag *bag = soundfont.pbag + j;
                TRACE("    - bag[%u]:\n", j);
                TRACE("        - gen_ndx: %u\n", bag->gen_ndx);
                TRACE("        - mod_ndx: %u\n", bag->mod_ndx);

                for (k = bag->gen_ndx; k < (bag + 1)->gen_ndx; k++)
                {
                    struct sf_gen *gen = soundfont.pgen + k;
                    TRACE("        - gen[%u]: %s\n", k, debugstr_sf_gen(gen));
                }

                for (k = bag->mod_ndx; k < (bag + 1)->mod_ndx; k++)
                {
                    struct sf_mod *mod = soundfont.pmod + k;
                    TRACE("        - mod[%u]: %s\n", k, debugstr_sf_mod(mod));
                }
            }
        }

        TRACE("instruments:\n");
        for (i = 0; i < soundfont.instrument_count; i++)
        {
            struct sf_instrument *instrument = soundfont.inst + i;
            TRACE("instrument[%u]:\n", i);
            TRACE("    - name: %s\n", debugstr_a(instrument->name));
            TRACE("    - bag_ndx: %u\n", instrument->bag_ndx);

            for (j = instrument->bag_ndx; j < (instrument + 1)->bag_ndx; j++)
            {
                struct sf_bag *bag = soundfont.ibag + j;
                TRACE("    - bag[%u]:\n", j);
                TRACE("        - wGenNdx: %u\n", bag->gen_ndx);
                TRACE("        - wModNdx: %u\n", bag->mod_ndx);

                for (k = bag->gen_ndx; k < (bag + 1)->gen_ndx; k++)
                {
                    struct sf_gen *gen = soundfont.igen + k;
                    TRACE("        - gen[%u]: %s\n", k, debugstr_sf_gen(gen));
                }

                for (k = bag->mod_ndx; k < (bag + 1)->mod_ndx; k++)
                {
                    struct sf_mod *mod = soundfont.imod + k;
                    TRACE("        - mod[%u]: %s\n", k, debugstr_sf_mod(mod));
                }
            }
        }

        TRACE("samples:\n");
        for (i = 0; i < soundfont.sample_count; i++)
        {
            struct sf_sample *sample = soundfont.shdr + i;

            TRACE("sample[%u]:\n", i);
            TRACE("    - name: %s\n", debugstr_a(sample->name));
            TRACE("    - start: %lu\n", sample->start);
            TRACE("    - end: %lu\n", sample->end);
            TRACE("    - start_loop: %lu\n", sample->start_loop);
            TRACE("    - end_loop: %lu\n", sample->end_loop);
            TRACE("    - sample_rate: %lu\n", sample->sample_rate);
            TRACE("    - original_key: %u\n", sample->original_key);
            TRACE("    - correction: %d\n", sample->correction);
            TRACE("    - sample_link: %#x\n", sample->sample_link);
            TRACE("    - sample_type: %#x\n", sample->sample_type);
        }
    }

    for (i = 0; SUCCEEDED(hr) && i < soundfont.preset_count; i++)
    {
        struct instrument_entry *entry;

        if (!(entry = malloc(sizeof(*entry)))) return E_OUTOFMEMORY;
        hr = instrument_create_from_soundfont(&soundfont, i, This, &entry->desc, &entry->instrument);
        if (SUCCEEDED(hr)) hr = IDirectMusicInstrument_GetPatch(entry->instrument, &entry->patch);
        if (SUCCEEDED(hr)) list_add_tail(&This->instruments, &entry->entry);
        else free(entry);
    }

    if (SUCCEEDED(hr))
    {
        UINT size = offsetof(struct pool, cues[soundfont.sample_count]);
        if (!(This->pool = calloc(1, size))) return E_OUTOFMEMORY;
        This->pool->table.cbSize = sizeof(This->pool->table);
    }

    for (i = 0; SUCCEEDED(hr) && i < soundfont.sample_count; i++)
    {
        struct wave_entry *entry;

        if (!(entry = malloc(sizeof(*entry)))) return E_OUTOFMEMORY;
        hr = wave_create_from_soundfont(&soundfont, i, &entry->wave);
        if (FAILED(hr)) free(entry);
        else
        {
            entry->offset = i;
            This->pool->table.cCues++;
            This->pool->cues[i].ulOffset = i;
            list_add_tail(&This->waves, &entry->entry);
        }
    }

    free(soundfont.phdr);
    free(soundfont.pbag);
    free(soundfont.pmod);
    free(soundfont.pgen);
    free(soundfont.inst);
    free(soundfont.ibag);
    free(soundfont.imod);
    free(soundfont.igen);
    free(soundfont.shdr);
    free(soundfont.sdta);

    return hr;
}

static HRESULT WINAPI collection_object_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || (riff.type != FOURCC_DLS && riff.type != mmioFOURCC('s','f','b','k'))) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_NOTADLSCOL;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc, DMUS_OBJ_NAME_INFO|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicCollection;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl collection_object_vtbl =
{
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    collection_object_ParseDescriptor,
};

static HRESULT WINAPI collection_stream_Load(IPersistStream *iface, IStream *stream)
{
    struct collection *This = impl_from_IPersistStream(iface);
    struct chunk_entry chunk = {0};
    HRESULT hr;

    TRACE("(%p, %p)\n", This, stream);

    if ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_RIFF, FOURCC_DLS):
            hr = parse_dls_chunk(This, stream, &chunk);
            break;

        case MAKE_IDTYPE(FOURCC_RIFF, mmioFOURCC('s','f','b','k')):
            hr = parse_sfbk_chunk(This, stream, &chunk);
            break;

        default:
            WARN("Invalid collection chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            hr = DMUS_E_UNSUPPORTED_STREAM;
            break;
        }
    }

    if (FAILED(hr)) return hr;

    if (TRACE_ON(dmusic))
    {
        struct instrument_entry *entry;
        struct wave_entry *wave_entry;
        int i = 0;

        TRACE("*** IDirectMusicCollection (%p) ***\n", &This->IDirectMusicCollection_iface);
        dump_DMUS_OBJECTDESC(&This->dmobj.desc);

        TRACE(" - Collection header:\n");
        TRACE("    - cInstruments: %ld\n", This->header.cInstruments);
        TRACE(" - Instruments:\n");

        LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
        {
            TRACE("    - Instrument[%i]: %p\n", i, entry->instrument);
            i++;
        }

        TRACE(" - cues:\n");
        for (i = 0; This->pool && i < This->pool->table.cCues; i++)
            TRACE("    - index: %u, offset: %lu\n", i, This->pool->cues[i].ulOffset);

        TRACE(" - waves:\n");
        LIST_FOR_EACH_ENTRY(wave_entry, &This->waves, struct wave_entry, entry)
            TRACE("    - offset: %lu, wave %p\n", wave_entry->offset, wave_entry->wave);
    }

    stream_skip_chunk(stream, &chunk);
    return S_OK;
}

static const IPersistStreamVtbl collection_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    unimpl_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    collection_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

HRESULT collection_create(IUnknown **ret_iface)
{
    struct collection *collection;

    *ret_iface = NULL;
    if (!(collection = calloc(1, sizeof(*collection)))) return E_OUTOFMEMORY;
    collection->IDirectMusicCollection_iface.lpVtbl = &collection_vtbl;
    collection->internal_ref = 1;
    collection->ref = 1;
    dmobject_init(&collection->dmobj, &CLSID_DirectMusicCollection,
            (IUnknown *)&collection->IDirectMusicCollection_iface);
    collection->dmobj.IDirectMusicObject_iface.lpVtbl = &collection_object_vtbl;
    collection->dmobj.IPersistStream_iface.lpVtbl = &collection_stream_vtbl;
    list_init(&collection->instruments);
    list_init(&collection->waves);

    TRACE("Created DirectMusicCollection %p\n", collection);
    *ret_iface = (IUnknown *)&collection->IDirectMusicCollection_iface;
    return S_OK;
}
