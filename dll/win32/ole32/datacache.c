/*
 *	OLE 2 Data cache
 *
 *      Copyright 1999  Francis Beaudet
 *      Copyright 2000  Abey George
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *    The OLE2 data cache supports a whole whack of
 *    interfaces including:
 *       IDataObject, IPersistStorage, IViewObject2,
 *       IOleCache2 and IOleCacheControl.
 *
 *    Most of the implementation details are taken from: Inside OLE
 *    second edition by Kraig Brockschmidt,
 *
 * NOTES
 *  -  This implementation of the datacache will let your application
 *     load documents that have embedded OLE objects in them and it will
 *     also retrieve the metafile representation of those objects.
 *  -  This implementation of the datacache will also allow your
 *     application to save new documents with OLE objects in them.
 *  -  The main thing that it doesn't do is allow you to activate
 *     or modify the OLE objects in any way.
 *  -  I haven't found any good documentation on the real usage of
 *     the streams created by the data cache. In particular, How to
 *     determine what the XXX stands for in the stream name
 *     "\002OlePresXXX". It appears to just be a counter.
 *  -  Also, I don't know the real content of the presentation stream
 *     header. I was able to figure-out where the extent of the object
 *     was stored and the aspect, but that's about it.
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/****************************************************************************
 * PresentationDataHeader
 *
 * This structure represents the header of the \002OlePresXXX stream in
 * the OLE object storage.
 */
typedef struct PresentationDataHeader
{
  /* clipformat:
   *  - standard clipformat:
   *  DWORD length = 0xffffffff;
   *  DWORD cfFormat;
   *  - or custom clipformat:
   *  DWORD length;
   *  CHAR format_name[length]; (null-terminated)
   */
  DWORD unknown3;	/* 4, possibly TYMED_ISTREAM */
  DVASPECT dvAspect;
  DWORD lindex;
  DWORD tymed;
  DWORD unknown7;	/* 0 */
  DWORD dwObjectExtentX;
  DWORD dwObjectExtentY;
  DWORD dwSize;
} PresentationDataHeader;

enum stream_type
{
    no_stream,
    pres_stream,
    contents_stream
};

typedef struct DataCacheEntry
{
  struct list entry;
  /* format of this entry */
  FORMATETC fmtetc;
  /* the clipboard format of the data */
  CLIPFORMAT data_cf;
  /* cached data */
  STGMEDIUM stgmedium;
  /*
   * This stream pointer is set through a call to
   * IPersistStorage_Load. This is where the visual
   * representation of the object is stored.
   */
  IStream *stream;
  enum stream_type stream_type;
  /* connection ID */
  DWORD id;
  /* dirty flag */
  BOOL dirty;
  /* stream number (-1 if not set ) */
  unsigned short stream_number;
  /* sink id set when object is running */
  DWORD sink_id;
  /* Advise sink flags */
  DWORD advise_flags;
} DataCacheEntry;

/****************************************************************************
 * DataCache
 */
struct DataCache
{
  /*
   * List all interface here
   */
  IDataObject       IDataObject_iface;
  IUnknown          IUnknown_iface;
  IPersistStorage   IPersistStorage_iface;
  IViewObject2      IViewObject2_iface;
  IOleCache2        IOleCache2_iface;
  IOleCacheControl  IOleCacheControl_iface;

  /* The sink that is connected to a remote object.
     The other interfaces are not available by QI'ing the sink and vice-versa */
  IAdviseSink       IAdviseSink_iface;

  /*
   * Reference count of this object
   */
  LONG ref;

  /*
   * IUnknown implementation of the outer object.
   */
  IUnknown* outerUnknown;

  /*
   * The user of this object can setup ONE advise sink
   * connection with the object. These parameters describe
   * that connection.
   */
  DWORD        sinkAspects;
  DWORD        sinkAdviseFlag;
  IAdviseSink* sinkInterface;
  IStorage *presentationStorage;

  /* list of cache entries */
  struct list cache_list;
  /* last id assigned to an entry */
  DWORD last_cache_id;
  /* dirty flag */
  BOOL dirty;
  /* running object set by OnRun */
  IDataObject *running_object;
};

typedef struct DataCache DataCache;

/*
 * Here, I define utility macros to help with the casting of the
 * "this" parameter.
 * There is a version to accommodate all of the VTables implemented
 * by this object.
 */

static inline DataCache *impl_from_IDataObject( IDataObject *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IDataObject_iface);
}

static inline DataCache *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IUnknown_iface);
}

static inline DataCache *impl_from_IPersistStorage( IPersistStorage *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IPersistStorage_iface);
}

static inline DataCache *impl_from_IViewObject2( IViewObject2 *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IViewObject2_iface);
}

static inline DataCache *impl_from_IOleCache2( IOleCache2 *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IOleCache2_iface);
}

static inline DataCache *impl_from_IOleCacheControl( IOleCacheControl *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IOleCacheControl_iface);
}

static inline DataCache *impl_from_IAdviseSink( IAdviseSink *iface )
{
    return CONTAINING_RECORD(iface, DataCache, IAdviseSink_iface);
}

const char *debugstr_formatetc(const FORMATETC *formatetc)
{
    return wine_dbg_sprintf("{ cfFormat = 0x%x, ptd = %p, dwAspect = %d, lindex = %d, tymed = %d }",
        formatetc->cfFormat, formatetc->ptd, formatetc->dwAspect,
        formatetc->lindex, formatetc->tymed);
}

static void DataCacheEntry_Destroy(DataCache *cache, DataCacheEntry *cache_entry)
{
    list_remove(&cache_entry->entry);
    if (cache_entry->stream)
        IStream_Release(cache_entry->stream);
    HeapFree(GetProcessHeap(), 0, cache_entry->fmtetc.ptd);
    ReleaseStgMedium(&cache_entry->stgmedium);
    if(cache_entry->sink_id)
        IDataObject_DUnadvise(cache->running_object, cache_entry->sink_id);

    HeapFree(GetProcessHeap(), 0, cache_entry);
}

static void DataCache_Destroy(
  DataCache* ptrToDestroy)
{
  DataCacheEntry *cache_entry, *next_cache_entry;

  TRACE("()\n");

  if (ptrToDestroy->sinkInterface != NULL)
  {
    IAdviseSink_Release(ptrToDestroy->sinkInterface);
    ptrToDestroy->sinkInterface = NULL;
  }

  LIST_FOR_EACH_ENTRY_SAFE(cache_entry, next_cache_entry, &ptrToDestroy->cache_list, DataCacheEntry, entry)
    DataCacheEntry_Destroy(ptrToDestroy, cache_entry);

  if (ptrToDestroy->presentationStorage != NULL)
  {
    IStorage_Release(ptrToDestroy->presentationStorage);
    ptrToDestroy->presentationStorage = NULL;
  }

  /*
   * Free the datacache pointer.
   */
  HeapFree(GetProcessHeap(), 0, ptrToDestroy);
}

static DataCacheEntry *DataCache_GetEntryForFormatEtc(DataCache *This, const FORMATETC *formatetc)
{
    DataCacheEntry *cache_entry;
    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        /* FIXME: also compare DVTARGETDEVICEs */
        if ((!cache_entry->fmtetc.cfFormat || !formatetc->cfFormat || (formatetc->cfFormat == cache_entry->fmtetc.cfFormat)) &&
            (formatetc->dwAspect == cache_entry->fmtetc.dwAspect) &&
            (formatetc->lindex == cache_entry->fmtetc.lindex) &&
            (!cache_entry->fmtetc.tymed || !formatetc->tymed || (formatetc->tymed == cache_entry->fmtetc.tymed)))
            return cache_entry;
    }
    return NULL;
}

/* checks that the clipformat and tymed are valid and returns an error if they
* aren't and CACHE_S_NOTSUPPORTED if they are valid, but can't be rendered by
* DataCache_Draw */
static HRESULT check_valid_clipformat_and_tymed(CLIPFORMAT cfFormat, DWORD tymed, BOOL load)
{
    if (!cfFormat || !tymed ||
        (cfFormat == CF_METAFILEPICT && (tymed == TYMED_MFPICT || load)) ||
        (cfFormat == CF_BITMAP && tymed == TYMED_GDI) ||
        (cfFormat == CF_DIB && tymed == TYMED_HGLOBAL) ||
        (cfFormat == CF_ENHMETAFILE && tymed == TYMED_ENHMF))
        return S_OK;
    else if (tymed == TYMED_HGLOBAL)
        return CACHE_S_FORMATETC_NOTSUPPORTED;
    else
    {
        WARN("invalid clipformat/tymed combination: %d/%d\n", cfFormat, tymed);
        return DV_E_TYMED;
    }
}

static HRESULT DataCache_CreateEntry(DataCache *This, const FORMATETC *formatetc, DataCacheEntry **cache_entry, BOOL load)
{
    HRESULT hr;

    hr = check_valid_clipformat_and_tymed(formatetc->cfFormat, formatetc->tymed, load);
    if (FAILED(hr))
        return hr;
    if (hr == CACHE_S_FORMATETC_NOTSUPPORTED)
        TRACE("creating unsupported format %d\n", formatetc->cfFormat);

    *cache_entry = HeapAlloc(GetProcessHeap(), 0, sizeof(**cache_entry));
    if (!*cache_entry)
        return E_OUTOFMEMORY;

    (*cache_entry)->fmtetc = *formatetc;
    if (formatetc->ptd)
    {
        (*cache_entry)->fmtetc.ptd = HeapAlloc(GetProcessHeap(), 0, formatetc->ptd->tdSize);
        memcpy((*cache_entry)->fmtetc.ptd, formatetc->ptd, formatetc->ptd->tdSize);
    }
    (*cache_entry)->data_cf = 0;
    (*cache_entry)->stgmedium.tymed = TYMED_NULL;
    (*cache_entry)->stgmedium.pUnkForRelease = NULL;
    (*cache_entry)->stream = NULL;
    (*cache_entry)->stream_type = no_stream;
    (*cache_entry)->id = This->last_cache_id++;
    (*cache_entry)->dirty = TRUE;
    (*cache_entry)->stream_number = -1;
    (*cache_entry)->sink_id = 0;
    (*cache_entry)->advise_flags = 0;
    list_add_tail(&This->cache_list, &(*cache_entry)->entry);
    return hr;
}

/************************************************************************
 * DataCache_FireOnViewChange
 *
 * This method will fire an OnViewChange notification to the advise
 * sink registered with the datacache.
 *
 * See IAdviseSink::OnViewChange for more details.
 */
static void DataCache_FireOnViewChange(
  DataCache* this,
  DWORD      aspect,
  LONG       lindex)
{
  TRACE("(%p, %x, %d)\n", this, aspect, lindex);

  /*
   * The sink supplies a filter when it registers
   * we make sure we only send the notifications when that
   * filter matches.
   */
  if ((this->sinkAspects & aspect) != 0)
  {
    if (this->sinkInterface != NULL)
    {
      IAdviseSink_OnViewChange(this->sinkInterface,
			       aspect,
			       lindex);

      /*
       * Some sinks want to be unregistered automatically when
       * the first notification goes out.
       */
      if ( (this->sinkAdviseFlag & ADVF_ONLYONCE) != 0)
      {
	IAdviseSink_Release(this->sinkInterface);

	this->sinkInterface  = NULL;
	this->sinkAspects    = 0;
	this->sinkAdviseFlag = 0;
      }
    }
  }
}

/* Helper for DataCacheEntry_OpenPresStream */
static BOOL DataCache_IsPresentationStream(const STATSTG *elem)
{
    /* The presentation streams have names of the form "\002OlePresXXX",
     * where XXX goes from 000 to 999. */
    static const WCHAR OlePres[] = { 2,'O','l','e','P','r','e','s' };

    LPCWSTR name = elem->pwcsName;

    return (elem->type == STGTY_STREAM)
	&& (strlenW(name) == 11)
	&& (strncmpW(name, OlePres, 8) == 0)
	&& (name[8] >= '0') && (name[8] <= '9')
	&& (name[9] >= '0') && (name[9] <= '9')
	&& (name[10] >= '0') && (name[10] <= '9');
}

static HRESULT read_clipformat(IStream *stream, CLIPFORMAT *clipformat)
{
    DWORD length;
    HRESULT hr;
    ULONG read;

    *clipformat = 0;

    hr = IStream_Read(stream, &length, sizeof(length), &read);
    if (hr != S_OK || read != sizeof(length))
        return DV_E_CLIPFORMAT;
    if (length == -1)
    {
        DWORD cf;
        hr = IStream_Read(stream, &cf, sizeof(cf), &read);
        if (hr != S_OK || read != sizeof(cf))
            return DV_E_CLIPFORMAT;
        *clipformat = cf;
    }
    else
    {
        char *format_name = HeapAlloc(GetProcessHeap(), 0, length);
        if (!format_name)
            return E_OUTOFMEMORY;
        hr = IStream_Read(stream, format_name, length, &read);
        if (hr != S_OK || read != length || format_name[length - 1] != '\0')
        {
            HeapFree(GetProcessHeap(), 0, format_name);
            return DV_E_CLIPFORMAT;
        }
        *clipformat = RegisterClipboardFormatA(format_name);
        HeapFree(GetProcessHeap(), 0, format_name);
    }
    return S_OK;
}

static HRESULT write_clipformat(IStream *stream, CLIPFORMAT clipformat)
{
    DWORD length;
    HRESULT hr;

    if (clipformat < 0xc000)
        length = -1;
    else
        length = GetClipboardFormatNameA(clipformat, NULL, 0);
    hr = IStream_Write(stream, &length, sizeof(length), NULL);
    if (FAILED(hr))
        return hr;
    if (clipformat < 0xc000)
    {
        DWORD cf = clipformat;
        hr = IStream_Write(stream, &cf, sizeof(cf), NULL);
    }
    else
    {
        char *format_name = HeapAlloc(GetProcessHeap(), 0, length);
        if (!format_name)
            return E_OUTOFMEMORY;
        GetClipboardFormatNameA(clipformat, format_name, length);
        hr = IStream_Write(stream, format_name, length, NULL);
        HeapFree(GetProcessHeap(), 0, format_name);
    }
    return hr;
}

/************************************************************************
 * DataCacheEntry_OpenPresStream
 *
 * This method will find the stream for the given presentation. It makes
 * no attempt at fallback.
 *
 * Param:
 *   this       - Pointer to the DataCache object
 *   drawAspect - The aspect of the object that we wish to draw.
 *   pStm       - A returned stream. It points to the beginning of the
 *              - presentation data, including the header.
 *
 * Errors:
 *   S_OK		The requested stream has been opened.
 *   OLE_E_BLANK	The requested stream could not be found.
 *   Quite a few others I'm too lazy to map correctly.
 *
 * Notes:
 *   Algorithm:	Scan the elements of the presentation storage, looking
 *		for presentation streams. For each presentation stream,
 *		load the header and check to see if the aspect matches.
 *
 *   If a fallback is desired, just opening the first presentation stream
 *   is a possibility.
 */
static HRESULT DataCacheEntry_OpenPresStream(DataCacheEntry *cache_entry, IStream **ppStm)
{
    HRESULT hr;
    LARGE_INTEGER offset;

    if (cache_entry->stream)
    {
        /* Rewind the stream before returning it. */
        offset.QuadPart = 0;

        hr = IStream_Seek( cache_entry->stream, offset, STREAM_SEEK_SET, NULL );
        if (SUCCEEDED( hr ))
        {
            *ppStm = cache_entry->stream;
            IStream_AddRef( cache_entry->stream );
        }
    }
    else
        hr = OLE_E_BLANK;

    return hr;
}


static HRESULT load_mf_pict( DataCacheEntry *cache_entry, IStream *stm )
{
    HRESULT hr;
    STATSTG stat;
    ULARGE_INTEGER current_pos;
    void *bits;
    METAFILEPICT *mfpict;
    HGLOBAL hmfpict;
    PresentationDataHeader header;
    CLIPFORMAT clipformat;
    static const LARGE_INTEGER offset_zero;
    ULONG read;

    if (cache_entry->stream_type != pres_stream)
    {
        FIXME( "Unimplemented for stream type %d\n", cache_entry->stream_type );
        return E_FAIL;
    }

    hr = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    hr = read_clipformat( stm, &clipformat );
    if (FAILED( hr )) return hr;

    hr = IStream_Read( stm, &header, sizeof(header), &read );
    if (hr != S_OK || read != sizeof(header)) return E_FAIL;

    hr = IStream_Seek( stm, offset_zero, STREAM_SEEK_CUR, &current_pos );
    if (FAILED( hr )) return hr;

    stat.cbSize.QuadPart -= current_pos.QuadPart;

    hmfpict = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
    if (!hmfpict) return E_OUTOFMEMORY;
    mfpict = GlobalLock( hmfpict );

    bits = HeapAlloc( GetProcessHeap(), 0, stat.cbSize.u.LowPart);
    if (!bits)
    {
        GlobalFree( hmfpict );
        return E_OUTOFMEMORY;
    }

    hr = IStream_Read( stm, bits, stat.cbSize.u.LowPart, &read );
    if (hr != S_OK || read != stat.cbSize.u.LowPart) hr = E_FAIL;

    if (SUCCEEDED( hr ))
    {
        /* FIXME: get this from the stream */
        mfpict->mm = MM_ANISOTROPIC;
        mfpict->xExt = header.dwObjectExtentX;
        mfpict->yExt = header.dwObjectExtentY;
        mfpict->hMF = SetMetaFileBitsEx( stat.cbSize.u.LowPart, bits );
        if (!mfpict->hMF)
            hr = E_FAIL;
    }

    GlobalUnlock( hmfpict );
    if (SUCCEEDED( hr ))
    {
        cache_entry->data_cf = cache_entry->fmtetc.cfFormat;
        cache_entry->stgmedium.tymed = TYMED_MFPICT;
        cache_entry->stgmedium.u.hMetaFilePict = hmfpict;
    }
    else
        GlobalFree( hmfpict );

    HeapFree( GetProcessHeap(), 0, bits );

    return hr;
}

static HRESULT load_dib( DataCacheEntry *cache_entry, IStream *stm )
{
    HRESULT hr;
    STATSTG stat;
    void *dib;
    HGLOBAL hglobal;
    ULONG read;

    if (cache_entry->stream_type != contents_stream)
    {
        FIXME( "Unimplemented for stream type %d\n", cache_entry->stream_type );
        return E_FAIL;
    }

    hr = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    hglobal = GlobalAlloc( GMEM_MOVEABLE, stat.cbSize.u.LowPart );
    if (!hglobal) return E_OUTOFMEMORY;
    dib = GlobalLock( hglobal );

    hr = IStream_Read( stm, dib, stat.cbSize.u.LowPart, &read );
    GlobalUnlock( hglobal );

    if (hr != S_OK || read != stat.cbSize.u.LowPart)
    {
        GlobalFree( hglobal );
        return E_FAIL;
    }

    cache_entry->data_cf = cache_entry->fmtetc.cfFormat;
    cache_entry->stgmedium.tymed = TYMED_HGLOBAL;
    cache_entry->stgmedium.u.hGlobal = hglobal;

    return S_OK;
}

/************************************************************************
 * DataCacheEntry_LoadData
 *
 * This method will read information for the requested presentation
 * into the given structure.
 *
 * Param:
 *   This - The entry to load the data from.
 *
 * Returns:
 *   This method returns a metafile handle if it is successful.
 *   it will return 0 if not.
 */
static HRESULT DataCacheEntry_LoadData(DataCacheEntry *cache_entry)
{
    HRESULT hr;
    IStream *stm;

    hr = DataCacheEntry_OpenPresStream( cache_entry, &stm );
    if (FAILED(hr)) return hr;

    switch (cache_entry->fmtetc.cfFormat)
    {
    case CF_METAFILEPICT:
        hr = load_mf_pict( cache_entry, stm );
        break;

    case CF_DIB:
        hr = load_dib( cache_entry, stm );
        break;

    default:
        FIXME( "Unimplemented clip format %x\n", cache_entry->fmtetc.cfFormat );
        hr = E_NOTIMPL;
    }

    IStream_Release( stm );
    return hr;
}

static HRESULT DataCacheEntry_CreateStream(DataCacheEntry *cache_entry,
                                           IStorage *storage, IStream **stream)
{
    WCHAR wszName[] = {2,'O','l','e','P','r','e','s',
        '0' + (cache_entry->stream_number / 100) % 10,
        '0' + (cache_entry->stream_number / 10) % 10,
        '0' + cache_entry->stream_number % 10, 0};

    /* FIXME: cache the created stream in This? */
    return IStorage_CreateStream(storage, wszName,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                                 0, 0, stream);
}

static HRESULT DataCacheEntry_Save(DataCacheEntry *cache_entry, IStorage *storage,
                                   BOOL same_as_load)
{
    PresentationDataHeader header;
    HRESULT hr;
    IStream *pres_stream;
    void *data = NULL;

    TRACE("stream_number = %d, fmtetc = %s\n", cache_entry->stream_number, debugstr_formatetc(&cache_entry->fmtetc));

    hr = DataCacheEntry_CreateStream(cache_entry, storage, &pres_stream);
    if (FAILED(hr))
        return hr;

    hr = write_clipformat(pres_stream, cache_entry->data_cf);
    if (FAILED(hr))
        return hr;

    if (cache_entry->fmtetc.ptd)
        FIXME("ptd not serialized\n");
    header.unknown3 = 4;
    header.dvAspect = cache_entry->fmtetc.dwAspect;
    header.lindex = cache_entry->fmtetc.lindex;
    header.tymed = cache_entry->stgmedium.tymed;
    header.unknown7 = 0;
    header.dwObjectExtentX = 0;
    header.dwObjectExtentY = 0;
    header.dwSize = 0;

    /* size the data */
    switch (cache_entry->data_cf)
    {
        case CF_METAFILEPICT:
        {
            if (cache_entry->stgmedium.tymed != TYMED_NULL)
            {
                const METAFILEPICT *mfpict = GlobalLock(cache_entry->stgmedium.u.hMetaFilePict);
                if (!mfpict)
                {
                    IStream_Release(pres_stream);
                    return DV_E_STGMEDIUM;
                }
                header.dwObjectExtentX = mfpict->xExt;
                header.dwObjectExtentY = mfpict->yExt;
                header.dwSize = GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
                GlobalUnlock(cache_entry->stgmedium.u.hMetaFilePict);
            }
            break;
        }
        default:
            break;
    }

    /*
     * Write the header.
     */
    hr = IStream_Write(pres_stream, &header, sizeof(PresentationDataHeader),
                       NULL);
    if (FAILED(hr))
    {
        IStream_Release(pres_stream);
        return hr;
    }

    /* get the data */
    switch (cache_entry->data_cf)
    {
        case CF_METAFILEPICT:
        {
            if (cache_entry->stgmedium.tymed != TYMED_NULL)
            {
                const METAFILEPICT *mfpict = GlobalLock(cache_entry->stgmedium.u.hMetaFilePict);
                if (!mfpict)
                {
                    IStream_Release(pres_stream);
                    return DV_E_STGMEDIUM;
                }
                data = HeapAlloc(GetProcessHeap(), 0, header.dwSize);
                GetMetaFileBitsEx(mfpict->hMF, header.dwSize, data);
                GlobalUnlock(cache_entry->stgmedium.u.hMetaFilePict);
            }
            break;
        }
        default:
            break;
    }

    if (data)
        hr = IStream_Write(pres_stream, data, header.dwSize, NULL);
    HeapFree(GetProcessHeap(), 0, data);

    IStream_Release(pres_stream);
    return hr;
}

/* helper for copying STGMEDIUM of type bitmap, MF, EMF or HGLOBAL.
* does no checking of whether src_stgm has a supported tymed, so this should be
* done in the caller */
static HRESULT copy_stg_medium(CLIPFORMAT cf, STGMEDIUM *dest_stgm,
                               const STGMEDIUM *src_stgm)
{
    if (src_stgm->tymed == TYMED_MFPICT)
    {
        const METAFILEPICT *src_mfpict = GlobalLock(src_stgm->u.hMetaFilePict);
        METAFILEPICT *dest_mfpict;

        if (!src_mfpict)
            return DV_E_STGMEDIUM;
        dest_stgm->u.hMetaFilePict = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
        dest_mfpict = GlobalLock(dest_stgm->u.hMetaFilePict);
        if (!dest_mfpict)
        {
            GlobalUnlock(src_stgm->u.hMetaFilePict);
            return E_OUTOFMEMORY;
        }
        *dest_mfpict = *src_mfpict;
        dest_mfpict->hMF = CopyMetaFileW(src_mfpict->hMF, NULL);
        GlobalUnlock(src_stgm->u.hMetaFilePict);
        GlobalUnlock(dest_stgm->u.hMetaFilePict);
    }
    else if (src_stgm->tymed != TYMED_NULL)
    {
        dest_stgm->u.hGlobal = OleDuplicateData(src_stgm->u.hGlobal, cf,
                                                GMEM_MOVEABLE);
        if (!dest_stgm->u.hGlobal)
            return E_OUTOFMEMORY;
    }
    dest_stgm->tymed = src_stgm->tymed;
    dest_stgm->pUnkForRelease = src_stgm->pUnkForRelease;
    if (dest_stgm->pUnkForRelease)
        IUnknown_AddRef(dest_stgm->pUnkForRelease);
    return S_OK;
}

static HRESULT DataCacheEntry_SetData(DataCacheEntry *cache_entry,
                                      const FORMATETC *formatetc,
                                      const STGMEDIUM *stgmedium,
                                      BOOL fRelease)
{
    if ((!cache_entry->fmtetc.cfFormat && !formatetc->cfFormat) ||
        (cache_entry->fmtetc.tymed == TYMED_NULL && formatetc->tymed == TYMED_NULL) ||
        stgmedium->tymed == TYMED_NULL)
    {
        WARN("invalid formatetc\n");
        return DV_E_FORMATETC;
    }

    cache_entry->dirty = TRUE;
    ReleaseStgMedium(&cache_entry->stgmedium);
    cache_entry->data_cf = cache_entry->fmtetc.cfFormat ? cache_entry->fmtetc.cfFormat : formatetc->cfFormat;
    if (fRelease)
    {
        cache_entry->stgmedium = *stgmedium;
        return S_OK;
    }
    else
        return copy_stg_medium(cache_entry->data_cf,
                               &cache_entry->stgmedium, stgmedium);
}

static HRESULT DataCacheEntry_GetData(DataCacheEntry *cache_entry, STGMEDIUM *stgmedium)
{
    if (stgmedium->tymed == TYMED_NULL && cache_entry->stream)
    {
        HRESULT hr = DataCacheEntry_LoadData(cache_entry);
        if (FAILED(hr))
            return hr;
    }
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
        return OLE_E_BLANK;
    return copy_stg_medium(cache_entry->data_cf, stgmedium, &cache_entry->stgmedium);
}

static inline HRESULT DataCacheEntry_DiscardData(DataCacheEntry *cache_entry)
{
    ReleaseStgMedium(&cache_entry->stgmedium);
    cache_entry->data_cf = cache_entry->fmtetc.cfFormat;
    return S_OK;
}

static inline void DataCacheEntry_HandsOffStorage(DataCacheEntry *cache_entry)
{
    if (cache_entry->stream)
    {
        IStream_Release(cache_entry->stream);
        cache_entry->stream = NULL;
    }
}

/*********************************************************
 * Method implementation for the  non delegating IUnknown
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_NDIUnknown_QueryInterface (IUnknown)
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject)
{
  DataCache *this = impl_from_IUnknown(iface);

  if ( ppvObject==0 )
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid))
  {
    *ppvObject = iface;
  }
  else if (IsEqualIID(&IID_IDataObject, riid))
  {
    *ppvObject = &this->IDataObject_iface;
  }
  else if ( IsEqualIID(&IID_IPersistStorage, riid)  ||
            IsEqualIID(&IID_IPersist, riid) )
  {
    *ppvObject = &this->IPersistStorage_iface;
  }
  else if ( IsEqualIID(&IID_IViewObject, riid) ||
            IsEqualIID(&IID_IViewObject2, riid) )
  {
    *ppvObject = &this->IViewObject2_iface;
  }
  else if ( IsEqualIID(&IID_IOleCache, riid) ||
            IsEqualIID(&IID_IOleCache2, riid) )
  {
    *ppvObject = &this->IOleCache2_iface;
  }
  else if ( IsEqualIID(&IID_IOleCacheControl, riid) )
  {
    *ppvObject = &this->IOleCacheControl_iface;
  }

  if ((*ppvObject)==0)
  {
    WARN( "() : asking for unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;
}

/************************************************************************
 * DataCache_NDIUnknown_AddRef (IUnknown)
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_AddRef(
            IUnknown*      iface)
{
  DataCache *this = impl_from_IUnknown(iface);
  return InterlockedIncrement(&this->ref);
}

/************************************************************************
 * DataCache_NDIUnknown_Release (IUnknown)
 *
 * This version of QueryInterface will not delegate its implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_Release(
            IUnknown*      iface)
{
  DataCache *this = impl_from_IUnknown(iface);
  ULONG ref;

  ref = InterlockedDecrement(&this->ref);

  if (ref == 0) DataCache_Destroy(this);

  return ref;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IDataObject_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataCache_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DataCache_IDataObject_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IDataObject_AddRef(
            IDataObject*     iface)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IDataObject_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IDataObject_Release(
            IDataObject*     iface)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_Release(this->outerUnknown);
}

/************************************************************************
 * DataCache_GetData
 *
 * Get Data from a source dataobject using format pformatetcIn->cfFormat
 */
static HRESULT WINAPI DataCache_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn,
	    STGMEDIUM*       pmedium)
{
    DataCache *This = impl_from_IDataObject(iface);
    DataCacheEntry *cache_entry;

    memset(pmedium, 0, sizeof(*pmedium));

    cache_entry = DataCache_GetEntryForFormatEtc(This, pformatetcIn);
    if (!cache_entry)
        return OLE_E_BLANK;

    return DataCacheEntry_GetData(cache_entry, pmedium);
}

static HRESULT WINAPI DataCache_GetDataHere(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_QueryGetData( IDataObject *iface, FORMATETC *fmt )
{
    DataCache *This = impl_from_IDataObject( iface );
    DataCacheEntry *cache_entry;

    TRACE( "(%p)->(%s)\n", iface, debugstr_formatetc( fmt ) );
    cache_entry = DataCache_GetEntryForFormatEtc( This, fmt );

    return cache_entry ? S_OK : S_FALSE;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 */
static HRESULT WINAPI DataCache_GetCanonicalFormatEtc(
	    IDataObject*     iface,
	    LPFORMATETC      pformatectIn,
	    LPFORMATETC      pformatetcOut)
{
  TRACE("()\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_IDataObject_SetData (IDataObject)
 *
 * This method is delegated to the IOleCache2 implementation.
 */
static HRESULT WINAPI DataCache_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium,
	    BOOL             fRelease)
{
  IOleCache2* oleCache = NULL;
  HRESULT     hres;

  TRACE("(%p, %p, %p, %d)\n", iface, pformatetc, pmedium, fRelease);

  hres = IDataObject_QueryInterface(iface, &IID_IOleCache2, (void**)&oleCache);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IOleCache2_SetData(oleCache, pformatetc, pmedium, fRelease);

  IOleCache2_Release(oleCache);

  return hres;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 */
static HRESULT WINAPI DataCache_EnumFormatEtc(
	    IDataObject*     iface,
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc)
{
  TRACE("()\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_DAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 */
static HRESULT WINAPI DataCache_DAdvise(
	    IDataObject*     iface,
	    FORMATETC*       pformatetc,
	    DWORD            advf,
	    IAdviseSink*     pAdvSink,
	    DWORD*           pdwConnection)
{
  TRACE("()\n");
  return OLE_E_ADVISENOTSUPPORTED;
}

/************************************************************************
 * DataCache_DUnadvise (IDataObject)
 *
 * The data cache doesn't support connections.
 */
static HRESULT WINAPI DataCache_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection)
{
  TRACE("()\n");
  return OLE_E_NOCONNECTION;
}

/************************************************************************
 * DataCache_EnumDAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 */
static HRESULT WINAPI DataCache_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise)
{
  TRACE("()\n");
  return OLE_E_ADVISENOTSUPPORTED;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IPersistStorage_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataCache_IPersistStorage_QueryInterface(
            IPersistStorage* iface,
            REFIID           riid,
            void**           ppvObject)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DataCache_IPersistStorage_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IPersistStorage_AddRef(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IPersistStorage_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IPersistStorage_Release(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_Release(this->outerUnknown);
}

/************************************************************************
 * DataCache_GetClassID (IPersistStorage)
 *
 */
static HRESULT WINAPI DataCache_GetClassID(IPersistStorage *iface, CLSID *clsid)
{
    DataCache *This = impl_from_IPersistStorage( iface );
    HRESULT hr;
    STATSTG statstg;

    TRACE( "(%p, %p)\n", iface, clsid );

    if (This->presentationStorage)
    {
        hr = IStorage_Stat( This->presentationStorage, &statstg, STATFLAG_NONAME );
        if (SUCCEEDED(hr))
        {
            *clsid = statstg.clsid;
            return S_OK;
        }
    }

    *clsid = CLSID_NULL;

    return S_OK;
}

/************************************************************************
 * DataCache_IsDirty (IPersistStorage)
 */
static HRESULT WINAPI DataCache_IsDirty(
            IPersistStorage* iface)
{
    DataCache *This = impl_from_IPersistStorage(iface);
    DataCacheEntry *cache_entry;

    TRACE("(%p)\n", iface);

    if (This->dirty)
        return S_OK;

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
        if (cache_entry->dirty)
            return S_OK;

    return S_FALSE;
}

/************************************************************************
 * DataCache_InitNew (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_InitNew simply stores
 * the storage pointer.
 */
static HRESULT WINAPI DataCache_InitNew(
            IPersistStorage* iface,
	    IStorage*        pStg)
{
    DataCache *This = impl_from_IPersistStorage(iface);

    TRACE("(%p, %p)\n", iface, pStg);

    if (This->presentationStorage != NULL)
        IStorage_Release(This->presentationStorage);

    This->presentationStorage = pStg;

    IStorage_AddRef(This->presentationStorage);
    This->dirty = TRUE;

    return S_OK;
}


static HRESULT add_cache_entry( DataCache *This, const FORMATETC *fmt, IStream *stm,
                                enum stream_type type )
{
    DataCacheEntry *cache_entry;
    HRESULT hr = S_OK;

    TRACE( "loading entry with formatetc: %s\n", debugstr_formatetc( fmt ) );

    cache_entry = DataCache_GetEntryForFormatEtc( This, fmt );
    if (!cache_entry)
        hr = DataCache_CreateEntry( This, fmt, &cache_entry, TRUE );
    if (SUCCEEDED( hr ))
    {
        DataCacheEntry_DiscardData( cache_entry );
        if (cache_entry->stream) IStream_Release( cache_entry->stream );
        cache_entry->stream = stm;
        IStream_AddRef( stm );
        cache_entry->stream_type = type;
        cache_entry->dirty = FALSE;
    }
    return hr;
}

static HRESULT parse_pres_streams( DataCache *This, IStorage *stg )
{
    HRESULT hr;
    IEnumSTATSTG *stat_enum;
    STATSTG stat;
    IStream *stm;
    PresentationDataHeader header;
    ULONG actual_read;
    CLIPFORMAT clipformat;
    FORMATETC fmtetc;

    hr = IStorage_EnumElements( stg, 0, NULL, 0, &stat_enum );
    if (FAILED( hr )) return hr;

    while ((hr = IEnumSTATSTG_Next( stat_enum, 1, &stat, NULL )) == S_OK)
    {
        if (DataCache_IsPresentationStream( &stat ))
        {
            hr = IStorage_OpenStream( stg, stat.pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                      0, &stm );
            if (SUCCEEDED( hr ))
            {
                hr = read_clipformat( stm, &clipformat );

                if (hr == S_OK)
                    hr = IStream_Read( stm, &header, sizeof(header), &actual_read );

                if (hr == S_OK && actual_read == sizeof(header))
                {
                    fmtetc.cfFormat = clipformat;
                    fmtetc.ptd = NULL; /* FIXME */
                    fmtetc.dwAspect = header.dvAspect;
                    fmtetc.lindex = header.lindex;
                    fmtetc.tymed = header.tymed;

                    add_cache_entry( This, &fmtetc, stm, pres_stream );
                }
                IStream_Release( stm );
            }
        }
        CoTaskMemFree( stat.pwcsName );
    }
    IEnumSTATSTG_Release( stat_enum );

    return S_OK;
}

static const FORMATETC static_dib_fmt = { CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

static HRESULT parse_contents_stream( DataCache *This, IStorage *stg, IStream *stm )
{
    HRESULT hr;
    STATSTG stat;
    const FORMATETC *fmt;

    hr = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    if (IsEqualCLSID( &stat.clsid, &CLSID_Picture_Dib ))
        fmt = &static_dib_fmt;
    else
    {
        FIXME("unsupported format %s\n", debugstr_guid( &stat.clsid ));
        return E_FAIL;
    }

    return add_cache_entry( This, fmt, stm, contents_stream );
}

static const WCHAR CONTENTS[] = {'C','O','N','T','E','N','T','S',0};

/************************************************************************
 * DataCache_Load (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_Load doesn't
 * actually load anything. Instead, it holds on to the storage pointer
 * and it will load the presentation information when the
 * IDataObject_GetData or IViewObject2_Draw methods are called.
 */
static HRESULT WINAPI DataCache_Load( IPersistStorage *iface, IStorage *pStg )
{
    DataCache *This = impl_from_IPersistStorage(iface);
    HRESULT hr;
    IStream *stm;

    TRACE("(%p, %p)\n", iface, pStg);

    IPersistStorage_HandsOffStorage( iface );

    hr = IStorage_OpenStream( pStg, CONTENTS, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                              0, &stm );
    if (SUCCEEDED( hr ))
    {
        hr = parse_contents_stream( This, pStg, stm );
        IStream_Release( stm );
    }

    if (FAILED(hr))
        hr = parse_pres_streams( This, pStg );

    if (SUCCEEDED( hr ))
    {
        This->dirty = FALSE;
        This->presentationStorage = pStg;
        IStorage_AddRef( This->presentationStorage );
    }

    return hr;
}

/************************************************************************
 * DataCache_Save (IPersistStorage)
 *
 * Until we actually connect to a running object and retrieve new
 * information to it, we never have to save anything. However, it is
 * our responsibility to copy the information when saving to a new
 * storage.
 */
static HRESULT WINAPI DataCache_Save(
            IPersistStorage* iface,
	    IStorage*        pStg,
	    BOOL             fSameAsLoad)
{
    DataCache *This = impl_from_IPersistStorage(iface);
    DataCacheEntry *cache_entry;
    BOOL dirty = FALSE;
    HRESULT hr = S_OK;
    unsigned short stream_number = 0;

    TRACE("(%p, %p, %d)\n", iface, pStg, fSameAsLoad);

    dirty = This->dirty;
    if (!dirty)
    {
        LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
        {
            dirty = cache_entry->dirty;
            if (dirty)
                break;
        }
    }

    /* this is a shortcut if nothing changed */
    if (!dirty && !fSameAsLoad && This->presentationStorage)
    {
        return IStorage_CopyTo(This->presentationStorage, 0, NULL, NULL, pStg);
    }

    /* assign stream numbers to the cache entries */
    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        if (cache_entry->stream_number != stream_number)
        {
            cache_entry->dirty = TRUE; /* needs to be written out again */
            cache_entry->stream_number = stream_number;
        }
        stream_number++;
    }

    /* write out the cache entries */
    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        if (!fSameAsLoad || cache_entry->dirty)
        {
            hr = DataCacheEntry_Save(cache_entry, pStg, fSameAsLoad);
            if (FAILED(hr))
                break;

            cache_entry->dirty = FALSE;
        }
    }

    This->dirty = FALSE;
    return hr;
}

/************************************************************************
 * DataCache_SaveCompleted (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currently holding.
 */
static HRESULT WINAPI DataCache_SaveCompleted(
            IPersistStorage* iface,
	    IStorage*        pStgNew)
{
  TRACE("(%p, %p)\n", iface, pStgNew);

  if (pStgNew)
  {
    IPersistStorage_HandsOffStorage(iface);

    DataCache_Load(iface, pStgNew);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_HandsOffStorage (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currently holding.
 */
static HRESULT WINAPI DataCache_HandsOffStorage(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);
  DataCacheEntry *cache_entry;

  TRACE("(%p)\n", iface);

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
    this->presentationStorage = NULL;
  }

  LIST_FOR_EACH_ENTRY(cache_entry, &this->cache_list, DataCacheEntry, entry)
    DataCacheEntry_HandsOffStorage(cache_entry);

  return S_OK;
}

/*********************************************************
 * Method implementation for the IViewObject2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IViewObject2_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataCache_IViewObject2_QueryInterface(
            IViewObject2* iface,
            REFIID           riid,
            void**           ppvObject)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DataCache_IViewObject2_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IViewObject2_AddRef(
            IViewObject2* iface)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IViewObject2_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IViewObject2_Release(
            IViewObject2* iface)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_Release(this->outerUnknown);
}

/************************************************************************
 * DataCache_Draw (IViewObject2)
 *
 * This method will draw the cached representation of the object
 * to the given device context.
 */
static HRESULT WINAPI DataCache_Draw(
            IViewObject2*    iface,
	    DWORD            dwDrawAspect,
	    LONG             lindex,
	    void*            pvAspect,
	    DVTARGETDEVICE*  ptd,
	    HDC              hdcTargetDev,
	    HDC              hdcDraw,
	    LPCRECTL         lprcBounds,
	    LPCRECTL         lprcWBounds,
	    BOOL  (CALLBACK *pfnContinue)(ULONG_PTR dwContinue),
	    ULONG_PTR        dwContinue)
{
  DataCache *This = impl_from_IViewObject2(iface);
  HRESULT                hres;
  DataCacheEntry        *cache_entry;

  TRACE("(%p, %x, %d, %p, %p, %p, %p, %p, %p, %lx)\n",
	iface,
	dwDrawAspect,
	lindex,
	pvAspect,
	hdcTargetDev,
	hdcDraw,
	lprcBounds,
	lprcWBounds,
	pfnContinue,
	dwContinue);

  if (lprcBounds==NULL)
    return E_INVALIDARG;

  LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
  {
    /* FIXME: compare ptd too */
    if ((cache_entry->fmtetc.dwAspect != dwDrawAspect) ||
        (cache_entry->fmtetc.lindex != lindex))
      continue;

    /* if the data hasn't been loaded yet, do it now */
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && cache_entry->stream)
    {
      hres = DataCacheEntry_LoadData(cache_entry);
      if (FAILED(hres))
        continue;
    }

    /* no data */
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
      continue;

    if (pfnContinue && !pfnContinue(dwContinue)) return E_ABORT;

    switch (cache_entry->data_cf)
    {
      case CF_METAFILEPICT:
      {
        /*
         * We have to be careful not to modify the state of the
         * DC.
         */
        INT   prevMapMode;
        SIZE  oldWindowExt;
        SIZE  oldViewportExt;
        POINT oldViewportOrg;
        METAFILEPICT *mfpict;

        if ((cache_entry->stgmedium.tymed != TYMED_MFPICT) ||
            !((mfpict = GlobalLock(cache_entry->stgmedium.u.hMetaFilePict))))
          continue;

        prevMapMode = SetMapMode(hdcDraw, mfpict->mm);

        SetWindowExtEx(hdcDraw,
		       mfpict->xExt,
		       mfpict->yExt,
		       &oldWindowExt);

        SetViewportExtEx(hdcDraw,
		         lprcBounds->right - lprcBounds->left,
		         lprcBounds->bottom - lprcBounds->top,
		         &oldViewportExt);

        SetViewportOrgEx(hdcDraw,
		         lprcBounds->left,
		         lprcBounds->top,
		         &oldViewportOrg);

        PlayMetaFile(hdcDraw, mfpict->hMF);

        SetWindowExtEx(hdcDraw,
		       oldWindowExt.cx,
		       oldWindowExt.cy,
		       NULL);

        SetViewportExtEx(hdcDraw,
		         oldViewportExt.cx,
		         oldViewportExt.cy,
		         NULL);

        SetViewportOrgEx(hdcDraw,
		         oldViewportOrg.x,
		         oldViewportOrg.y,
		         NULL);

        SetMapMode(hdcDraw, prevMapMode);

        GlobalUnlock(cache_entry->stgmedium.u.hMetaFilePict);

        return S_OK;
      }
      case CF_DIB:
      {
          BITMAPFILEHEADER *file_head;
          BITMAPINFO *info;
          BYTE *bits;

          if ((cache_entry->stgmedium.tymed != TYMED_HGLOBAL) ||
              !((file_head = GlobalLock( cache_entry->stgmedium.u.hGlobal ))))
              continue;

          info = (BITMAPINFO *)(file_head + 1);
          bits = (BYTE *) file_head + file_head->bfOffBits;
          StretchDIBits( hdcDraw, lprcBounds->left, lprcBounds->top,
                         lprcBounds->right - lprcBounds->left, lprcBounds->bottom - lprcBounds->top,
                         0, 0, info->bmiHeader.biWidth, info->bmiHeader.biHeight,
                         bits, info, DIB_RGB_COLORS, SRCCOPY );

          GlobalUnlock( cache_entry->stgmedium.u.hGlobal );
          return S_OK;
      }
    }
  }

  WARN("no data could be found to be drawn\n");

  return OLE_E_BLANK;
}

static HRESULT WINAPI DataCache_GetColorSet(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect,
	    DVTARGETDEVICE* ptd,
	    HDC             hicTargetDevice,
	    LOGPALETTE**    ppColorSet)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Freeze(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect,
	    DWORD*          pdwFreeze)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Unfreeze(
            IViewObject2*   iface,
	    DWORD           dwFreeze)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_SetAdvise (IViewObject2)
 *
 * This sets-up an advisory sink with the data cache. When the object's
 * view changes, this sink is called.
 */
static HRESULT WINAPI DataCache_SetAdvise(
            IViewObject2*   iface,
	    DWORD           aspects,
	    DWORD           advf,
	    IAdviseSink*    pAdvSink)
{
  DataCache *this = impl_from_IViewObject2(iface);

  TRACE("(%p, %x, %x, %p)\n", iface, aspects, advf, pAdvSink);

  /*
   * A call to this function removes the previous sink
   */
  if (this->sinkInterface != NULL)
  {
    IAdviseSink_Release(this->sinkInterface);
    this->sinkInterface  = NULL;
    this->sinkAspects    = 0;
    this->sinkAdviseFlag = 0;
  }

  /*
   * Now, setup the new one.
   */
  if (pAdvSink!=NULL)
  {
    this->sinkInterface  = pAdvSink;
    this->sinkAspects    = aspects;
    this->sinkAdviseFlag = advf;

    IAdviseSink_AddRef(this->sinkInterface);
  }

  /*
   * When the ADVF_PRIMEFIRST flag is set, we have to advise the
   * sink immediately.
   */
  if (advf & ADVF_PRIMEFIRST)
  {
    DataCache_FireOnViewChange(this, aspects, -1);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_GetAdvise (IViewObject2)
 *
 * This method queries the current state of the advise sink
 * installed on the data cache.
 */
static HRESULT WINAPI DataCache_GetAdvise(
            IViewObject2*   iface,
	    DWORD*          pAspects,
	    DWORD*          pAdvf,
	    IAdviseSink**   ppAdvSink)
{
  DataCache *this = impl_from_IViewObject2(iface);

  TRACE("(%p, %p, %p, %p)\n", iface, pAspects, pAdvf, ppAdvSink);

  /*
   * Just copy all the requested values.
   */
  if (pAspects!=NULL)
    *pAspects = this->sinkAspects;

  if (pAdvf!=NULL)
    *pAdvf = this->sinkAdviseFlag;

  if (ppAdvSink!=NULL)
  {
    if (this->sinkInterface != NULL)
        IAdviseSink_QueryInterface(this->sinkInterface,
			       &IID_IAdviseSink,
			       (void**)ppAdvSink);
    else *ppAdvSink = NULL;
  }

  return S_OK;
}

/************************************************************************
 * DataCache_GetExtent (IViewObject2)
 *
 * This method retrieves the "natural" size of this cached object.
 */
static HRESULT WINAPI DataCache_GetExtent(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    DVTARGETDEVICE* ptd,
	    LPSIZEL         lpsizel)
{
  DataCache *This = impl_from_IViewObject2(iface);
  HRESULT                hres = E_FAIL;
  DataCacheEntry        *cache_entry;

  TRACE("(%p, %x, %d, %p, %p)\n",
	iface, dwDrawAspect, lindex, ptd, lpsizel);

  if (lpsizel==NULL)
    return E_POINTER;

  lpsizel->cx = 0;
  lpsizel->cy = 0;

  if (lindex!=-1)
    FIXME("Unimplemented flag lindex = %d\n", lindex);

  /*
   * Right now, we support only the callback from
   * the default handler.
   */
  if (ptd!=NULL)
    FIXME("Unimplemented ptd = %p\n", ptd);

  LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
  {
    /* FIXME: compare ptd too */
    if ((cache_entry->fmtetc.dwAspect != dwDrawAspect) ||
        (cache_entry->fmtetc.lindex != lindex))
      continue;

    /* if the data hasn't been loaded yet, do it now */
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && cache_entry->stream)
    {
      hres = DataCacheEntry_LoadData(cache_entry);
      if (FAILED(hres))
        continue;
    }

    /* no data */
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
      continue;


    switch (cache_entry->data_cf)
    {
      case CF_METAFILEPICT:
      {
          METAFILEPICT *mfpict;

          if ((cache_entry->stgmedium.tymed != TYMED_MFPICT) ||
              !((mfpict = GlobalLock(cache_entry->stgmedium.u.hMetaFilePict))))
            continue;

        lpsizel->cx = mfpict->xExt;
        lpsizel->cy = mfpict->yExt;

        GlobalUnlock(cache_entry->stgmedium.u.hMetaFilePict);

        return S_OK;
      }
      case CF_DIB:
      {
          BITMAPFILEHEADER *file_head;
          BITMAPINFOHEADER *info;
          LONG x_pels_m, y_pels_m;


          if ((cache_entry->stgmedium.tymed != TYMED_HGLOBAL) ||
              !((file_head = GlobalLock( cache_entry->stgmedium.u.hGlobal ))))
              continue;

          info = (BITMAPINFOHEADER *)(file_head + 1);

          x_pels_m = info->biXPelsPerMeter;
          y_pels_m = info->biYPelsPerMeter;

          /* Size in units of 0.01mm (ie. MM_HIMETRIC) */
          if (x_pels_m != 0 && y_pels_m != 0)
          {
              lpsizel->cx = info->biWidth  * 100000 / x_pels_m;
              lpsizel->cy = info->biHeight * 100000 / y_pels_m;
          }
          else
          {
              HDC hdc = GetDC( 0 );
              lpsizel->cx = info->biWidth  * 2540 / GetDeviceCaps( hdc, LOGPIXELSX );
              lpsizel->cy = info->biHeight * 2540 / GetDeviceCaps( hdc, LOGPIXELSY );

              ReleaseDC( 0, hdc );
          }

          GlobalUnlock( cache_entry->stgmedium.u.hGlobal );

          return S_OK;
      }
    }
  }

  WARN("no data could be found to get the extents from\n");

  /*
   * This method returns OLE_E_BLANK when it fails.
   */
  return OLE_E_BLANK;
}


/*********************************************************
 * Method implementation for the IOleCache2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCache2_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataCache_IOleCache2_QueryInterface(
            IOleCache2*     iface,
            REFIID          riid,
            void**          ppvObject)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DataCache_IOleCache2_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCache2_AddRef(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IOleCache2_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCache2_Release(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_Release(this->outerUnknown);
}

/*****************************************************************************
 * setup_sink
 *
 * Set up the sink connection to the running object.
 */
static HRESULT setup_sink(DataCache *This, DataCacheEntry *cache_entry)
{
    HRESULT hr = S_FALSE;
    DWORD flags;

    /* Clear the ADVFCACHE_* bits.  Native also sets the two highest bits for some reason. */
    flags = cache_entry->advise_flags & ~(ADVFCACHE_NOHANDLER | ADVFCACHE_FORCEBUILTIN | ADVFCACHE_ONSAVE);

    if(This->running_object)
        if(!(flags & ADVF_NODATA))
            hr = IDataObject_DAdvise(This->running_object, &cache_entry->fmtetc, flags,
                                     &This->IAdviseSink_iface, &cache_entry->sink_id);
    return hr;
}

static HRESULT WINAPI DataCache_Cache(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    DWORD           advf,
	    DWORD*          pdwConnection)
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;
    HRESULT hr;

    TRACE("(%p, 0x%x, %p)\n", pformatetc, advf, pdwConnection);

    if (!pformatetc || !pdwConnection)
        return E_INVALIDARG;

    TRACE("pformatetc = %s\n", debugstr_formatetc(pformatetc));

    *pdwConnection = 0;

    cache_entry = DataCache_GetEntryForFormatEtc(This, pformatetc);
    if (cache_entry)
    {
        TRACE("found an existing cache entry\n");
        *pdwConnection = cache_entry->id;
        return CACHE_S_SAMECACHE;
    }

    hr = DataCache_CreateEntry(This, pformatetc, &cache_entry, FALSE);

    if (SUCCEEDED(hr))
    {
        *pdwConnection = cache_entry->id;
        cache_entry->advise_flags = advf;
        setup_sink(This, cache_entry);
    }

    return hr;
}

static HRESULT WINAPI DataCache_Uncache(
	    IOleCache2*     iface,
	    DWORD           dwConnection)
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;

    TRACE("(%d)\n", dwConnection);

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
        if (cache_entry->id == dwConnection)
        {
            DataCacheEntry_Destroy(This, cache_entry);
            return S_OK;
        }

    WARN("no connection found for %d\n", dwConnection);

    return OLE_E_NOCONNECTION;
}

static HRESULT WINAPI DataCache_EnumCache(
            IOleCache2*     iface,
	    IEnumSTATDATA** ppenumSTATDATA)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_InitCache(
	    IOleCache2*     iface,
	    IDataObject*    pDataObject)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_IOleCache2_SetData(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    STGMEDIUM*      pmedium,
	    BOOL            fRelease)
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;
    HRESULT hr;

    TRACE("(%p, %p, %s)\n", pformatetc, pmedium, fRelease ? "TRUE" : "FALSE");
    TRACE("formatetc = %s\n", debugstr_formatetc(pformatetc));

    cache_entry = DataCache_GetEntryForFormatEtc(This, pformatetc);
    if (cache_entry)
    {
        hr = DataCacheEntry_SetData(cache_entry, pformatetc, pmedium, fRelease);

        if (SUCCEEDED(hr))
            DataCache_FireOnViewChange(This, cache_entry->fmtetc.dwAspect,
                                       cache_entry->fmtetc.lindex);

        return hr;
    }
    WARN("cache entry not found\n");

    return OLE_E_BLANK;
}

static HRESULT WINAPI DataCache_UpdateCache(
            IOleCache2*     iface,
	    LPDATAOBJECT    pDataObject,
	    DWORD           grfUpdf,
	    LPVOID          pReserved)
{
  FIXME("(%p, 0x%x, %p): stub\n", pDataObject, grfUpdf, pReserved);
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions)
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;
    HRESULT hr = S_OK;

    TRACE("(%d)\n", dwDiscardOptions);

    if (dwDiscardOptions == DISCARDCACHE_SAVEIFDIRTY)
        hr = DataCache_Save(&This->IPersistStorage_iface, This->presentationStorage, TRUE);

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        hr = DataCacheEntry_DiscardData(cache_entry);
        if (FAILED(hr))
            break;
    }

    return hr;
}


/*********************************************************
 * Method implementation for the IOleCacheControl
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCacheControl_QueryInterface (IUnknown)
 */
static HRESULT WINAPI DataCache_IOleCacheControl_QueryInterface(
            IOleCacheControl* iface,
            REFIID            riid,
            void**            ppvObject)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);
}

/************************************************************************
 * DataCache_IOleCacheControl_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCacheControl_AddRef(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IOleCacheControl_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCacheControl_Release(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_Release(this->outerUnknown);
}

/************************************************************************
 * DataCache_OnRun (IOleCacheControl)
 */
static HRESULT WINAPI DataCache_OnRun(IOleCacheControl* iface, IDataObject *data_obj)
{
    DataCache *This = impl_from_IOleCacheControl(iface);
    DataCacheEntry *cache_entry;

    TRACE("(%p)->(%p)\n", iface, data_obj);

    if(This->running_object) return S_OK;

    /* No reference is taken on the data object */
    This->running_object = data_obj;

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        setup_sink(This, cache_entry);
    }

    return S_OK;
}

/************************************************************************
 * DataCache_OnStop (IOleCacheControl)
 */
static HRESULT WINAPI DataCache_OnStop(IOleCacheControl* iface)
{
    DataCache *This = impl_from_IOleCacheControl(iface);
    DataCacheEntry *cache_entry;

    TRACE("(%p)\n", iface);

    if(!This->running_object) return S_OK;

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        if(cache_entry->sink_id)
        {
            IDataObject_DUnadvise(This->running_object, cache_entry->sink_id);
            cache_entry->sink_id = 0;
        }
    }

    /* No ref taken in OnRun, so no Release call here */
    This->running_object = NULL;
    return S_OK;
}

/************************************************************************
 *              IAdviseSink methods.
 * This behaves as an internal object to the data cache.  QI'ing its ptr doesn't
 * give access to the cache's other interfaces.  We don't maintain a ref count,
 * the object exists as long as the cache is around.
 */
static HRESULT WINAPI DataCache_IAdviseSink_QueryInterface(IAdviseSink *iface, REFIID iid, void **obj)
{
    *obj = NULL;
    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IAdviseSink, iid))
    {
        *obj = iface;
    }

    if(*obj)
    {
        IAdviseSink_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI DataCache_IAdviseSink_AddRef(IAdviseSink *iface)
{
    return 2;
}

static ULONG WINAPI DataCache_IAdviseSink_Release(IAdviseSink *iface)
{
    return 1;
}

static void WINAPI DataCache_OnDataChange(IAdviseSink *iface, FORMATETC *fmt, STGMEDIUM *med)
{
    DataCache *This = impl_from_IAdviseSink(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_formatetc(fmt), med);
    IOleCache2_SetData(&This->IOleCache2_iface, fmt, med, FALSE);
}

static void WINAPI DataCache_OnViewChange(IAdviseSink *iface, DWORD aspect, LONG index)
{
    FIXME("stub\n");
}

static void WINAPI DataCache_OnRename(IAdviseSink *iface, IMoniker *mk)
{
    FIXME("stub\n");
}

static void WINAPI DataCache_OnSave(IAdviseSink *iface)
{
    FIXME("stub\n");
}

static void WINAPI DataCache_OnClose(IAdviseSink *iface)
{
    FIXME("stub\n");
}

/*
 * Virtual function tables for the DataCache class.
 */
static const IUnknownVtbl DataCache_NDIUnknown_VTable =
{
  DataCache_NDIUnknown_QueryInterface,
  DataCache_NDIUnknown_AddRef,
  DataCache_NDIUnknown_Release
};

static const IDataObjectVtbl DataCache_IDataObject_VTable =
{
  DataCache_IDataObject_QueryInterface,
  DataCache_IDataObject_AddRef,
  DataCache_IDataObject_Release,
  DataCache_GetData,
  DataCache_GetDataHere,
  DataCache_QueryGetData,
  DataCache_GetCanonicalFormatEtc,
  DataCache_IDataObject_SetData,
  DataCache_EnumFormatEtc,
  DataCache_DAdvise,
  DataCache_DUnadvise,
  DataCache_EnumDAdvise
};

static const IPersistStorageVtbl DataCache_IPersistStorage_VTable =
{
  DataCache_IPersistStorage_QueryInterface,
  DataCache_IPersistStorage_AddRef,
  DataCache_IPersistStorage_Release,
  DataCache_GetClassID,
  DataCache_IsDirty,
  DataCache_InitNew,
  DataCache_Load,
  DataCache_Save,
  DataCache_SaveCompleted,
  DataCache_HandsOffStorage
};

static const IViewObject2Vtbl DataCache_IViewObject2_VTable =
{
  DataCache_IViewObject2_QueryInterface,
  DataCache_IViewObject2_AddRef,
  DataCache_IViewObject2_Release,
  DataCache_Draw,
  DataCache_GetColorSet,
  DataCache_Freeze,
  DataCache_Unfreeze,
  DataCache_SetAdvise,
  DataCache_GetAdvise,
  DataCache_GetExtent
};

static const IOleCache2Vtbl DataCache_IOleCache2_VTable =
{
  DataCache_IOleCache2_QueryInterface,
  DataCache_IOleCache2_AddRef,
  DataCache_IOleCache2_Release,
  DataCache_Cache,
  DataCache_Uncache,
  DataCache_EnumCache,
  DataCache_InitCache,
  DataCache_IOleCache2_SetData,
  DataCache_UpdateCache,
  DataCache_DiscardCache
};

static const IOleCacheControlVtbl DataCache_IOleCacheControl_VTable =
{
  DataCache_IOleCacheControl_QueryInterface,
  DataCache_IOleCacheControl_AddRef,
  DataCache_IOleCacheControl_Release,
  DataCache_OnRun,
  DataCache_OnStop
};

static const IAdviseSinkVtbl DataCache_IAdviseSink_VTable =
{
    DataCache_IAdviseSink_QueryInterface,
    DataCache_IAdviseSink_AddRef,
    DataCache_IAdviseSink_Release,
    DataCache_OnDataChange,
    DataCache_OnViewChange,
    DataCache_OnRename,
    DataCache_OnSave,
    DataCache_OnClose
};

/*********************************************************
 * Method implementation for DataCache class.
 */
static DataCache* DataCache_Construct(
  REFCLSID  clsid,
  LPUNKNOWN pUnkOuter)
{
  DataCache* newObject = 0;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(DataCache));

  if (newObject==0)
    return newObject;

  /*
   * Initialize the virtual function table.
   */
  newObject->IDataObject_iface.lpVtbl = &DataCache_IDataObject_VTable;
  newObject->IUnknown_iface.lpVtbl = &DataCache_NDIUnknown_VTable;
  newObject->IPersistStorage_iface.lpVtbl = &DataCache_IPersistStorage_VTable;
  newObject->IViewObject2_iface.lpVtbl = &DataCache_IViewObject2_VTable;
  newObject->IOleCache2_iface.lpVtbl = &DataCache_IOleCache2_VTable;
  newObject->IOleCacheControl_iface.lpVtbl = &DataCache_IOleCacheControl_VTable;
  newObject->IAdviseSink_iface.lpVtbl = &DataCache_IAdviseSink_VTable;

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  /*
   * Initialize the outer unknown
   * We don't keep a reference on the outer unknown since, the way
   * aggregation works, our lifetime is at least as large as its
   * lifetime.
   */
  if (pUnkOuter==NULL)
    pUnkOuter = &newObject->IUnknown_iface;

  newObject->outerUnknown = pUnkOuter;

  /*
   * Initialize the other members of the structure.
   */
  newObject->sinkAspects = 0;
  newObject->sinkAdviseFlag = 0;
  newObject->sinkInterface = 0;
  newObject->presentationStorage = NULL;
  list_init(&newObject->cache_list);
  newObject->last_cache_id = 1;
  newObject->dirty = FALSE;
  newObject->running_object = NULL;

  return newObject;
}

/******************************************************************************
 *              CreateDataCache        [OLE32.@]
 *
 * Creates a data cache to allow an object to render one or more of its views,
 * whether running or not.
 *
 * PARAMS
 *  pUnkOuter [I] Outer unknown for the object.
 *  rclsid    [I]
 *  riid      [I] IID of interface to return.
 *  ppvObj    [O] Address where the data cache object will be stored on return.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  The following interfaces are supported by the returned data cache object:
 *  IOleCache, IOleCache2, IOleCacheControl, IPersistStorage, IDataObject,
 *  IViewObject and IViewObject2.
 */
HRESULT WINAPI CreateDataCache(
  LPUNKNOWN pUnkOuter,
  REFCLSID  rclsid,
  REFIID    riid,
  LPVOID*   ppvObj)
{
  DataCache* newCache = NULL;
  HRESULT    hr       = S_OK;

  TRACE("(%s, %p, %s, %p)\n", debugstr_guid(rclsid), pUnkOuter, debugstr_guid(riid), ppvObj);

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = 0;

  /*
   * If this cache is constructed for aggregation, make sure
   * the caller is requesting the IUnknown interface.
   * This is necessary because it's the only time the non-delegating
   * IUnknown pointer can be returned to the outside.
   */
  if ( pUnkOuter && !IsEqualIID(&IID_IUnknown, riid) )
    return CLASS_E_NOAGGREGATION;

  /*
   * Try to construct a new instance of the class.
   */
  newCache = DataCache_Construct(rclsid,
				 pUnkOuter);

  if (newCache == 0)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IUnknown_QueryInterface(&newCache->IUnknown_iface, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IUnknown_Release(&newCache->IUnknown_iface);

  return hr;
}
