/*
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
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

struct style_band {
    struct list entry;
    IDirectMusicBand *pBand;
};

struct style_part_ref
{
    struct list entry;
    DMUS_OBJECTDESC desc;
    DMUS_IO_PARTREF header;
};

struct style_part
{
    struct list entry;
    DMUS_OBJECTDESC desc;
    DMUS_IO_STYLEPART header;
    DMUS_IO_STYLENOTE *notes;
    UINT notes_count;
    DMUS_IO_STYLECURVE *curves;
    UINT curves_count;
    DMUS_IO_STYLEMARKER *markers;
    UINT markers_count;
    DMUS_IO_STYLERESOLUTION *resolutions;
    UINT resolutions_count;
    DMUS_IO_STYLE_ANTICIPATION *anticipations;
    UINT anticipations_count;
};

static void style_part_destroy(struct style_part *part)
{
    free(part->notes);
    free(part->curves);
    free(part->markers);
    free(part->resolutions);
    free(part->anticipations);
    free(part);
}

struct style_pattern
{
    struct list entry;
    DWORD dwRhythm;
    DMUS_IO_PATTERN pattern;
    DMUS_OBJECTDESC desc;
    DMUS_IO_MOTIFSETTINGS settings;
    IDirectMusicBand *band;
    struct list part_refs;
};

static void style_pattern_destroy(struct style_pattern *pattern)
{
    struct style_part_ref *part_ref, *next;

    LIST_FOR_EACH_ENTRY_SAFE(part_ref, next, &pattern->part_refs, struct style_part_ref, entry)
    {
        list_remove(&part_ref->entry);
        free(part_ref);
    }

    if (pattern->band) IDirectMusicBand_Release(pattern->band);
    free(pattern);
}

struct style
{
    IDirectMusicStyle8 IDirectMusicStyle8_iface;
    struct dmobject dmobj;
    LONG ref;
    DMUS_IO_STYLE style;
    struct list patterns;
    struct list bands;
    struct list parts;
};

static inline struct style *impl_from_IDirectMusicStyle8(IDirectMusicStyle8 *iface)
{
    return CONTAINING_RECORD(iface, struct style, IDirectMusicStyle8_iface);
}

static HRESULT WINAPI style_QueryInterface(IDirectMusicStyle8 *iface, REFIID riid,
        void **ret_iface)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicStyle) ||
            IsEqualIID(riid, &IID_IDirectMusicStyle8))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI style_AddRef(IDirectMusicStyle8 *iface)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI style_Release(IDirectMusicStyle8 *iface)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        struct style_band *band, *band2;
        struct style_pattern *pattern;
        struct style_part *part;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(band, band2, &This->bands, struct style_band, entry) {
            list_remove(&band->entry);
            if (band->pBand)
                IDirectMusicBand_Release(band->pBand);
            free(band);
        }

        LIST_FOR_EACH_ENTRY_SAFE(pattern, next, &This->patterns, struct style_pattern, entry)
        {
            list_remove(&pattern->entry);
            style_pattern_destroy(pattern);
        }

        LIST_FOR_EACH_ENTRY_SAFE(part, next, &This->parts, struct style_part, entry)
        {
            list_remove(&part->entry);
            style_part_destroy(part);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI style_GetBand(IDirectMusicStyle8 *iface, WCHAR *name,
        IDirectMusicBand **band)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    struct style_band *sband;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", This, debugstr_w(name), band);

    if (!name)
        return E_POINTER;

    LIST_FOR_EACH_ENTRY(sband, &This->bands, struct style_band, entry) {
        IDirectMusicObject *obj;

        hr = IDirectMusicBand_QueryInterface(sband->pBand, &IID_IDirectMusicObject, (void**)&obj);
        if (SUCCEEDED(hr)) {
            DMUS_OBJECTDESC desc;

            if (IDirectMusicObject_GetDescriptor(obj, &desc) == S_OK) {
                if (desc.dwValidData & DMUS_OBJ_NAME && !lstrcmpW(name, desc.wszName)) {
                    IDirectMusicObject_Release(obj);
                    IDirectMusicBand_AddRef(sband->pBand);
                    *band = sband->pBand;
                    return S_OK;
                }
            }

            IDirectMusicObject_Release(obj);
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI style_EnumBand(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
    return S_OK;
}

static HRESULT WINAPI style_GetDefaultBand(IDirectMusicStyle8 *iface,
        IDirectMusicBand **band)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p): stub\n", This, band);

    if (!band)
        return E_POINTER;

    *band = NULL;

    return S_FALSE;
}

static HRESULT WINAPI style_EnumMotif(IDirectMusicStyle8 *iface, DWORD index,
        WCHAR *name)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    const struct style_pattern *pattern = NULL;
    const struct list *cursor;
    unsigned int i = 0;

    TRACE("(%p, %lu, %p)\n", This, index, name);

    if (!name)
        return E_POINTER;

    /* index is zero based */
    LIST_FOR_EACH(cursor, &This->patterns)
    {
        if (i == index) {
            pattern = LIST_ENTRY(cursor, struct style_pattern, entry);
            break;
        }
        i++;
    }
    if (!pattern)
        return S_FALSE;

    if (pattern->desc.dwValidData & DMUS_OBJ_NAME)
        lstrcpynW(name, pattern->desc.wszName, DMUS_MAX_NAME);
    else
        name[0] = 0;

    TRACE("returning name: %s\n", debugstr_w(name));
    return S_OK;
}

static HRESULT WINAPI style_GetMotif(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicSegment **ppSegment)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszName), ppSegment);
    return S_FALSE;
}

static HRESULT WINAPI style_GetDefaultChordMap(IDirectMusicStyle8 *iface,
        IDirectMusicChordMap **ppChordMap)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p): stub\n", This, ppChordMap);
    return S_OK;
}

static HRESULT WINAPI style_EnumChordMap(IDirectMusicStyle8 *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
    return S_OK;
}

static HRESULT WINAPI style_GetChordMap(IDirectMusicStyle8 *iface, WCHAR *pwszName,
        IDirectMusicChordMap **ppChordMap)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p, %p): stub\n", This, pwszName, ppChordMap);
    return S_OK;
}

static HRESULT WINAPI style_GetTimeSignature(IDirectMusicStyle8 *iface,
        DMUS_TIMESIGNATURE *pTimeSig)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p): stub\n", This, pTimeSig);
    return S_OK;
}

static HRESULT WINAPI style_GetEmbellishmentLength(IDirectMusicStyle8 *iface,
        DWORD dwType, DWORD dwLevel, DWORD *pdwMin, DWORD *pdwMax)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %ld, %ld, %p, %p): stub\n", This, dwType, dwLevel, pdwMin, pdwMax);
    return S_OK;
}

static HRESULT WINAPI style_GetTempo(IDirectMusicStyle8 *iface, double *pTempo)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %p): stub\n", This, pTempo);
    return S_OK;
}

static HRESULT WINAPI style_EnumPattern(IDirectMusicStyle8 *iface, DWORD dwIndex,
        DWORD dwPatternType, WCHAR *pwszName)
{
    struct style *This = impl_from_IDirectMusicStyle8(iface);
    FIXME("(%p, %ld, %ld, %p): stub\n", This, dwIndex, dwPatternType, pwszName);
    return S_OK;
}

static const IDirectMusicStyle8Vtbl dmstyle8_vtbl = {
    style_QueryInterface,
    style_AddRef,
    style_Release,
    style_GetBand,
    style_EnumBand,
    style_GetDefaultBand,
    style_EnumMotif,
    style_GetMotif,
    style_GetDefaultChordMap,
    style_EnumChordMap,
    style_GetChordMap,
    style_GetTimeSignature,
    style_GetEmbellishmentLength,
    style_GetTempo,
    style_EnumPattern
};

static HRESULT WINAPI style_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_STYLE_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicStyle;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    style_IDirectMusicObject_ParseDescriptor
};

static inline struct style *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct style, dmobj.IPersistStream_iface);
}

static HRESULT load_band(IStream *pClonedStream, IDirectMusicBand **ppBand)
{
  HRESULT hr = E_FAIL;
  IPersistStream* pPersistStream = NULL;
  
  hr = CoCreateInstance (&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicBand, (LPVOID*) ppBand);
  if (FAILED(hr)) {
    ERR(": could not create object\n");
    return hr;
  }
  /* acquire PersistStream interface */
  hr = IDirectMusicBand_QueryInterface (*ppBand, &IID_IPersistStream, (LPVOID*) &pPersistStream);
  if (FAILED(hr)) {
    ERR(": could not acquire IPersistStream\n");
    return hr;
  }
  /* load */
  hr = IPersistStream_Load (pPersistStream, pClonedStream);
  if (FAILED(hr)) {
    ERR(": failed to load object\n");
    return hr;
  }
  
  /* release all loading-related stuff */
  IPersistStream_Release (pPersistStream);

  return S_OK;
}

static HRESULT parse_pref_list(struct style *This, IStream *stream, struct chunk_entry *parent,
        struct list *list)
{
    struct chunk_entry chunk = {.parent = parent};
    struct style_part_ref *part_ref;
    DMUS_OBJECTDESC desc;
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &desc, DMUS_OBJ_NAME))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    if (!(part_ref = calloc(1, sizeof(*part_ref)))) return E_OUTOFMEMORY;
    part_ref->desc = desc;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_PARTREF_CHUNK:
            hr = stream_chunk_get_data(stream, &chunk, &part_ref->header, sizeof(part_ref->header));
            break;

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) free(part_ref);
    else list_add_tail(list, &part_ref->entry);

    return S_OK;
}

static HRESULT parse_part_list(struct style *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct style_part *part;
    DMUS_OBJECTDESC desc;
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &desc, DMUS_OBJ_NAME))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    if (!(part = calloc(1, sizeof(*part)))) return E_OUTOFMEMORY;
    part->desc = desc;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case DMUS_FOURCC_PART_CHUNK:
            hr = stream_chunk_get_data(stream, &chunk, &part->header, sizeof(part->header));
            break;

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case DMUS_FOURCC_NOTE_CHUNK:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&part->notes,
                    &part->notes_count, sizeof(*part->notes));
            break;

        case DMUS_FOURCC_CURVE_CHUNK:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&part->curves,
                    &part->curves_count, sizeof(*part->curves));
            break;

        case DMUS_FOURCC_MARKER_CHUNK:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&part->markers,
                    &part->markers_count, sizeof(*part->markers));
            break;

        case DMUS_FOURCC_RESOLUTION_CHUNK:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&part->resolutions,
                    &part->resolutions_count, sizeof(*part->resolutions));
            break;

        case DMUS_FOURCC_ANTICIPATION_CHUNK:
            hr = stream_chunk_get_array(stream, &chunk, (void **)&part->anticipations,
                    &part->anticipations_count, sizeof(*part->anticipations));
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) style_part_destroy(part);
    else list_add_tail(&This->parts, &part->entry);

    return hr;
}

static HRESULT parse_pttn_list(struct style *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct style_pattern *pattern;
    DMUS_OBJECTDESC desc;
    HRESULT hr;

    if (FAILED(hr = dmobj_parsedescriptor(stream, parent, &desc, DMUS_OBJ_NAME))
            || FAILED(hr = stream_reset_chunk_data(stream, parent)))
        return hr;

    if (!(pattern = calloc(1, sizeof(*pattern)))) return E_OUTOFMEMORY;
    list_init(&pattern->part_refs);
    pattern->desc = desc;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
            /* already parsed by dmobj_parsedescriptor */
            break;

        case DMUS_FOURCC_PATTERN_CHUNK:
            hr = stream_chunk_get_data(stream, &chunk, &pattern->pattern, sizeof(pattern->pattern));
            break;

        case DMUS_FOURCC_RHYTHM_CHUNK:
            if (chunk.size > sizeof(pattern->dwRhythm)) FIXME("Unsupported rythm chunk size\n");
            hr = stream_read(stream, &pattern->dwRhythm, sizeof(pattern->dwRhythm));
            break;

        case DMUS_FOURCC_MOTIFSETTINGS_CHUNK:
            hr = stream_chunk_get_data(stream, &chunk, &pattern->settings, sizeof(pattern->settings));
            break;

        case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_BAND_FORM):
        {
            IPersistStream *persist;

            if (pattern->band) IDirectMusicBand_Release(pattern->band);

            if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IDirectMusicBand, (void **)&pattern->band)))
                break;

            if (SUCCEEDED(hr = IDirectMusicBand_QueryInterface(pattern->band, &IID_IPersistStream, (void **)&persist)))
            {
                if (SUCCEEDED(hr = stream_reset_chunk_start(stream, &chunk)))
                    hr = IPersistStream_Load(persist, stream);
                IPersistStream_Release(persist);
            }

            break;
        }

        case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_PARTREF_LIST):
            hr = parse_pref_list(This, stream, &chunk, &pattern->part_refs);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) style_pattern_destroy(pattern);
    else list_add_tail(&This->patterns, &pattern->entry);

    return hr;
}

static HRESULT parse_style_form(struct style *This, DMUS_PRIVATE_CHUNK *pChunk, IStream *pStm)
{
  HRESULT hr = E_FAIL;
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */

  IDirectMusicBand* pBand = NULL;

  if (pChunk->fccID != DMUS_FOURCC_STYLE_FORM) {
    ERR_(dmfile)(": %s chunk should be a STYLE form\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }  

  StreamSize = pChunk->dwSize - sizeof(FOURCC);
  StreamCount = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);

    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, &This->dmobj.desc);
    if (FAILED(hr)) return hr;

    if (hr == S_FALSE) {
      switch (Chunk.fccID) {
      case DMUS_FOURCC_STYLE_CHUNK: {
	TRACE_(dmfile)(": Style chunk\n");
	IStream_Read (pStm, &This->style, sizeof(DMUS_IO_STYLE), NULL);
	/** TODO dump DMUS_IO_TIMESIG style.timeSig */
	TRACE_(dmfile)(" - dblTempo: %g\n", This->style.dblTempo);
	break;
      }   
      case FOURCC_RIFF: {
	/**
	 * should be embedded Bands into style
	 */
	IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
	TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
	ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
	ListCount[0] = 0;
	switch (Chunk.fccID) {
	case DMUS_FOURCC_BAND_FORM: { 
          ULARGE_INTEGER save;
          struct style_band *pNewBand;

	  TRACE_(dmfile)(": BAND RIFF\n");

          /* Can be application provided IStream without Clone method */
	  liMove.QuadPart = 0;
	  liMove.QuadPart -= sizeof(FOURCC) + (sizeof(FOURCC)+sizeof(DWORD));
          IStream_Seek(pStm, liMove, STREAM_SEEK_CUR, &save);

          hr = load_band(pStm, &pBand);
	  if (FAILED(hr)) {
	    ERR(": could not load track\n");
	    return hr;
	  }

	  if (!(pNewBand = calloc(1, sizeof(*pNewBand)))) return E_OUTOFMEMORY;
	  pNewBand->pBand = pBand;
	  IDirectMusicBand_AddRef(pBand);
	  list_add_tail(&This->bands, &pNewBand->entry);

	  IDirectMusicTrack_Release(pBand); pBand = NULL;  /* now we can release it as it's inserted */
	
	  /** now safely move the cursor */
          liMove.QuadPart = save.QuadPart - liMove.QuadPart + ListSize[0];
          IStream_Seek(pStm, liMove, STREAM_SEEK_SET, NULL);

	  break;
	}
	default: {
	  TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	  liMove.QuadPart = ListSize[0];
	  IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	  break;
	}
	}
	break;
      }
      case FOURCC_LIST: {
	IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
	TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
	ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
	ListCount[0] = 0;
	switch (Chunk.fccID) {
	case DMUS_FOURCC_UNFO_LIST: { 
	  TRACE_(dmfile)(": UNFO list\n");
	  do {
	    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
            TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);

            hr = IDirectMusicUtils_IPersistStream_ParseUNFOGeneric(&Chunk, pStm, &This->dmobj.desc);
	    if (FAILED(hr)) return hr;
	    
	    if (hr == S_FALSE) {
	      switch (Chunk.fccID) {
	      default: {
		TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
		liMove.QuadPart = Chunk.dwSize;
		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
		break;				
	      }
	      }
	    }  
            TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
	  } while (ListCount[0] < ListSize[0]);
	  break;
	}
	case DMUS_FOURCC_PART_LIST: {
          static const LARGE_INTEGER zero = {0};
          struct chunk_entry chunk = {FOURCC_LIST, .size = Chunk.dwSize, .type = Chunk.fccID};
	  TRACE_(dmfile)(": PART list\n");
          IStream_Seek(pStm, zero, STREAM_SEEK_CUR, &chunk.offset);
          chunk.offset.QuadPart -= 12;
          hr = parse_part_list(This, pStm, &chunk);
	  if (FAILED(hr)) return hr;
	  break;
	}
	case  DMUS_FOURCC_PATTERN_LIST: {
          static const LARGE_INTEGER zero = {0};
          struct chunk_entry chunk = {FOURCC_LIST, .size = Chunk.dwSize, .type = Chunk.fccID};
	  TRACE_(dmfile)(": PATTERN list\n");
          IStream_Seek(pStm, zero, STREAM_SEEK_CUR, &chunk.offset);
          chunk.offset.QuadPart -= 12;
          hr = parse_pttn_list(This, pStm, &chunk);
	  if (FAILED(hr)) return hr;
	  break;
	}
	default: {
	  TRACE_(dmfile)(": unknown (skipping)\n");
	  liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
	  IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	  break;						
	}
	}
	break;
      }
      default: {
	TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	liMove.QuadPart = Chunk.dwSize;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
      }
    }
    TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
  } while (StreamCount < StreamSize);  

  return S_OK;
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
  struct style *This = impl_from_IPersistStream(iface);
  DMUS_PRIVATE_CHUNK Chunk;
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  FIXME("(%p, %p): Loading\n", This, pStm);

  IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
  TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
  switch (Chunk.fccID) {
  case FOURCC_RIFF: {
    IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
    switch (Chunk.fccID) {
    case DMUS_FOURCC_STYLE_FORM: {
      TRACE_(dmfile)(": Style form\n");
      hr = parse_style_form(This, &Chunk, pStm);
      if (FAILED(hr)) return hr;
      break;
    }
    default: {
      TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
      liMove.QuadPart = Chunk.dwSize;
      IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
      return E_FAIL;
    }
    }
    TRACE_(dmfile)(": reading finished\n");
    break;
  }
  default: {
    TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
    liMove.QuadPart = Chunk.dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
    return E_FAIL;
  }
  }
  
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

HRESULT create_dmstyle(REFIID lpcGUID, void **ppobj)
{
    struct style *obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicStyle8_iface.lpVtbl = &dmstyle8_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicStyle, (IUnknown *)&obj->IDirectMusicStyle8_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&obj->parts);
    list_init(&obj->bands);
    list_init(&obj->patterns);

    hr = IDirectMusicStyle8_QueryInterface(&obj->IDirectMusicStyle8_iface, lpcGUID, ppobj);
    IDirectMusicStyle8_Release(&obj->IDirectMusicStyle8_iface);

    return hr;
}
