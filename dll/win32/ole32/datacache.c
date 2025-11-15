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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "ole2.h"
#include "compobj_private.h"
#include "wine/list.h"
#include "wine/debug.h"

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
  DWORD tdSize; /* This is actually a truncated DVTARGETDEVICE, if tdSize > sizeof(DWORD)
                   then there are tdSize - sizeof(DWORD) more bytes before dvAspect */
  DVASPECT dvAspect;
  DWORD lindex;
  DWORD advf;
  DWORD unknown7;	/* 0 */
  DWORD dwObjectExtentX;
  DWORD dwObjectExtentY;
  DWORD dwSize;
} PresentationDataHeader;

#define STREAM_NUMBER_NOT_SET -2
#define STREAM_NUMBER_CONTENTS -1 /* CONTENTS stream */

typedef struct DataCacheEntry
{
  struct list entry;
  /* format of this entry */
  FORMATETC fmtetc;
  /* cached data */
  STGMEDIUM stgmedium;
  /* connection ID */
  DWORD id;
  /* dirty flag */
  BOOL dirty;
  /* stream number that the entry was loaded from.
     This is used to defer loading until the data is actually needed. */
  int load_stream_num;
  /* stream number that the entry will be saved to.
     This may differ from above if cache entries have been Uncache()d for example. */
  int save_stream_num;
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
  IUnknown          IUnknown_inner;
  IDataObject       IDataObject_iface;
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
  IUnknown *outer_unk;

  /*
   * The user of this object can setup ONE advise sink
   * connection with the object. These parameters describe
   * that connection.
   */
  DWORD        sinkAspects;
  DWORD        sinkAdviseFlag;
  IAdviseSink *sinkInterface;

  CLSID clsid;
  /* Is the clsid one of the CLSID_Picture classes */
  BOOL clsid_static;

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
    return CONTAINING_RECORD(iface, DataCache, IUnknown_inner);
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
    return wine_dbg_sprintf("{ cfFormat = 0x%x, ptd = %p, dwAspect = %ld, lindex = %ld, tymed = %ld }",
        formatetc->cfFormat, formatetc->ptd, formatetc->dwAspect,
        formatetc->lindex, formatetc->tymed);
}

/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
            ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (colors > 256) /* buffer overflow otherwise */
            colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

static void DataCacheEntry_Destroy(DataCache *cache, DataCacheEntry *cache_entry)
{
    list_remove(&cache_entry->entry);
    CoTaskMemFree(cache_entry->fmtetc.ptd);
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
    FORMATETC fmt = *formatetc;

    if (fmt.cfFormat == CF_BITMAP)
    {
        fmt.cfFormat = CF_DIB;
        fmt.tymed = TYMED_HGLOBAL;
    }

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        /* FIXME: also compare DVTARGETDEVICEs */
        if ((fmt.cfFormat == cache_entry->fmtetc.cfFormat) &&
            (fmt.dwAspect == cache_entry->fmtetc.dwAspect) &&
            (fmt.lindex == cache_entry->fmtetc.lindex) &&
            ((fmt.tymed == cache_entry->fmtetc.tymed) || !cache_entry->fmtetc.cfFormat)) /* tymed is ignored for view caching */
            return cache_entry;
    }
    return NULL;
}

/* Returns the cache entry associated with a static CLSID.
   This will be first in the list with connection id == 1 */
static HRESULT get_static_entry( DataCache *cache, DataCacheEntry **cache_entry )
{
    DataCacheEntry *entry;
    struct list *head = list_head( &cache->cache_list );
    HRESULT hr = E_FAIL;

    *cache_entry = NULL;

    if (head)
    {
        entry = LIST_ENTRY( head, DataCacheEntry, entry );
        if (entry->id == 1)
        {
            *cache_entry = entry;
            hr = S_OK;
        }
    }

    return hr;
}

/* checks that the clipformat and tymed are valid and returns an error if they
* aren't and CACHE_S_NOTSUPPORTED if they are valid, but can't be rendered by
* DataCache_Draw */
static HRESULT check_valid_formatetc( const FORMATETC *fmt )
{
    /* DVASPECT_ICON must be CF_METAFILEPICT */
    if (fmt->dwAspect == DVASPECT_ICON && fmt->cfFormat != CF_METAFILEPICT)
        return DV_E_FORMATETC;

    if (!fmt->cfFormat ||
        (fmt->cfFormat == CF_METAFILEPICT && fmt->tymed == TYMED_MFPICT) ||
        (fmt->cfFormat == CF_BITMAP && fmt->tymed == TYMED_GDI) ||
        (fmt->cfFormat == CF_DIB && fmt->tymed == TYMED_HGLOBAL) ||
        (fmt->cfFormat == CF_ENHMETAFILE && fmt->tymed == TYMED_ENHMF))
        return S_OK;
    else if (fmt->tymed == TYMED_HGLOBAL)
        return CACHE_S_FORMATETC_NOTSUPPORTED;
    else
    {
        WARN("invalid clipformat/tymed combination: %d/%ld\n", fmt->cfFormat, fmt->tymed);
        return DV_E_TYMED;
    }
}

static BOOL init_cache_entry(DataCacheEntry *entry, const FORMATETC *fmt, DWORD advf,
                             DWORD id)
{
    HRESULT hr;

    hr = copy_formatetc(&entry->fmtetc, fmt);
    if (FAILED(hr)) return FALSE;

    entry->stgmedium.tymed = TYMED_NULL;
    entry->stgmedium.pUnkForRelease = NULL;
    entry->id = id;
    entry->dirty = TRUE;
    entry->load_stream_num = STREAM_NUMBER_NOT_SET;
    entry->save_stream_num = STREAM_NUMBER_NOT_SET;
    entry->sink_id = 0;
    entry->advise_flags = advf;

    return TRUE;
}

static HRESULT DataCache_CreateEntry(DataCache *This, const FORMATETC *formatetc, DWORD advf,
                                     BOOL automatic, DataCacheEntry **cache_entry)
{
    HRESULT hr;
    DWORD id = automatic ? 1 : This->last_cache_id;
    DataCacheEntry *entry;

    hr = check_valid_formatetc( formatetc );
    if (FAILED(hr))
        return hr;
    if (hr == CACHE_S_FORMATETC_NOTSUPPORTED)
        TRACE("creating unsupported format %d\n", formatetc->cfFormat);

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    if (!entry)
        return E_OUTOFMEMORY;

    if (!init_cache_entry(entry, formatetc, advf, id))
        goto fail;

    if (automatic)
        list_add_head(&This->cache_list, &entry->entry);
    else
    {
        list_add_tail(&This->cache_list, &entry->entry);
        This->last_cache_id++;
    }

    if (cache_entry) *cache_entry = entry;
    return hr;

fail:
    HeapFree(GetProcessHeap(), 0, entry);
    return E_OUTOFMEMORY;
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
  TRACE("%p, %lx, %ld.\n", this, aspect, lindex);

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

static HRESULT read_clipformat(IStream *stream, CLIPFORMAT *clipformat)
{
    DWORD length;
    HRESULT hr;
    ULONG read;

    *clipformat = 0;

    hr = IStream_Read(stream, &length, sizeof(length), &read);
    if (hr != S_OK || read != sizeof(length))
        return DV_E_CLIPFORMAT;
    if (!length) {
        /* No clipboard format present */
        return S_OK;
    }
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
    char format_name[256];

    if (clipformat == 0)
        length = 0;
    else if (clipformat < 0xc000)
        length = -1;
    else
    {
        length = GetClipboardFormatNameA(clipformat, format_name, sizeof(format_name));
        /* If there is a clipboard format name, we need to include its terminating \0 */
        if (length) length++;
    }
    hr = IStream_Write(stream, &length, sizeof(length), NULL);
    if (FAILED(hr) || clipformat == 0)
        return hr;

    if (clipformat < 0xc000)
    {
        DWORD cf = clipformat;
        hr = IStream_Write(stream, &cf, sizeof(cf), NULL);
    }
    else
    {
        hr = IStream_Write(stream, format_name, length, NULL);
    }
    return hr;
}

static HRESULT open_pres_stream( IStorage *stg, int stream_number, IStream **stm )
{
    WCHAR pres[] = {2,'O','l','e','P','r','e','s',
                    '0' + (stream_number / 100) % 10,
                    '0' + (stream_number / 10) % 10,
                    '0' + stream_number % 10, 0};
    const WCHAR *name = pres;

    if (stream_number == STREAM_NUMBER_NOT_SET) return E_FAIL;
    if (stream_number == STREAM_NUMBER_CONTENTS) name = L"CONTENTS";

    return IStorage_OpenStream( stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, stm );
}

static HRESULT synthesize_emf( HMETAFILEPICT data, STGMEDIUM *med )
{
    METAFILEPICT *pict;
    HRESULT hr = E_FAIL;
    UINT size;
    void *bits;

    if (!(pict = GlobalLock( data ))) return hr;

    size = GetMetaFileBitsEx( pict->hMF, 0, NULL );
    if ((bits = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        GetMetaFileBitsEx( pict->hMF, size, bits );
        med->hEnhMetaFile = SetWinMetaFileBits( size, bits, NULL, pict );
        HeapFree( GetProcessHeap(), 0, bits );
        med->tymed = TYMED_ENHMF;
        med->pUnkForRelease = NULL;
        hr = S_OK;
    }

    GlobalUnlock( data );
    return hr;
}
#include <pshpack2.h>
struct meta_placeable
{
    DWORD key;
    WORD hwmf;
    WORD bounding_box[4];
    WORD inch;
    DWORD reserved;
    WORD checksum;
};
#include <poppack.h>

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
    struct meta_placeable mf_place;

    hr = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    if (cache_entry->load_stream_num != STREAM_NUMBER_CONTENTS)
    {
        hr = read_clipformat( stm, &clipformat );
        if (hr != S_OK) return hr;
        hr = IStream_Read( stm, &header, sizeof(header), &read );
        if (hr != S_OK) return hr;
    }
    else
    {
        hr = IStream_Read( stm, &mf_place, sizeof(mf_place), &read );
        if (hr != S_OK) return hr;
    }

    hr = IStream_Seek( stm, offset_zero, STREAM_SEEK_CUR, &current_pos );
    if (FAILED( hr )) return hr;
    stat.cbSize.QuadPart -= current_pos.QuadPart;

    hmfpict = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
    if (!hmfpict) return E_OUTOFMEMORY;
    mfpict = GlobalLock( hmfpict );

    bits = HeapAlloc( GetProcessHeap(), 0, stat.cbSize.LowPart);
    if (!bits)
    {
        GlobalFree( hmfpict );
        return E_OUTOFMEMORY;
    }

    hr = IStream_Read( stm, bits, stat.cbSize.LowPart, &read );

    if (SUCCEEDED( hr ))
    {
        mfpict->mm = MM_ANISOTROPIC;
        /* FIXME: get this from the stream */
        if (cache_entry->load_stream_num != STREAM_NUMBER_CONTENTS)
        {
            mfpict->xExt = header.dwObjectExtentX;
            mfpict->yExt = header.dwObjectExtentY;
        }
        else
        {
            mfpict->xExt = ((mf_place.bounding_box[2] - mf_place.bounding_box[0])
                            * 2540) / mf_place.inch;
            mfpict->yExt = ((mf_place.bounding_box[3] - mf_place.bounding_box[1])
                            * 2540) / mf_place.inch;
        }
        mfpict->hMF = SetMetaFileBitsEx( stat.cbSize.LowPart, bits );
        if (!mfpict->hMF)
            hr = E_FAIL;
    }

    GlobalUnlock( hmfpict );
    if (SUCCEEDED( hr ))
    {
        cache_entry->stgmedium.tymed = TYMED_MFPICT;
        cache_entry->stgmedium.hMetaFilePict = hmfpict;
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
    BYTE *dib;
    HGLOBAL hglobal;
    ULONG read, info_size, bi_size;
    BITMAPFILEHEADER file;
    BITMAPINFOHEADER *info;
    CLIPFORMAT cf;
    PresentationDataHeader pres;
    ULARGE_INTEGER current_pos;
    static const LARGE_INTEGER offset_zero;

    hr = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    if (cache_entry->load_stream_num != STREAM_NUMBER_CONTENTS)
    {
        hr = read_clipformat( stm, &cf );
        if (hr != S_OK) return hr;
        hr = IStream_Read( stm, &pres, sizeof(pres), &read );
        if (hr != S_OK) return hr;
    }
    else
    {
        hr = IStream_Read( stm, &file, sizeof(BITMAPFILEHEADER), &read );
        if (hr != S_OK) return hr;
    }

    hr = IStream_Seek( stm, offset_zero, STREAM_SEEK_CUR, &current_pos );
    if (FAILED( hr )) return hr;
    stat.cbSize.QuadPart -= current_pos.QuadPart;

    hglobal = GlobalAlloc( GMEM_MOVEABLE, stat.cbSize.LowPart );
    if (!hglobal) return E_OUTOFMEMORY;
    dib = GlobalLock( hglobal );

    /* read first DWORD of BITMAPINFOHEADER */
    hr = IStream_Read( stm, dib, sizeof(DWORD), &read );
    if (hr != S_OK) goto fail;
    bi_size = *(DWORD *)dib;
    if (stat.cbSize.QuadPart < bi_size) goto fail;

    /* read rest of BITMAPINFOHEADER */
    hr = IStream_Read( stm, dib + sizeof(DWORD), bi_size - sizeof(DWORD), &read );
    if (hr != S_OK) goto fail;

    info_size = bitmap_info_size( (BITMAPINFO *)dib, DIB_RGB_COLORS );
    if (stat.cbSize.QuadPart < info_size) goto fail;
    if (info_size > bi_size)
    {
        hr = IStream_Read( stm, dib + bi_size, info_size - bi_size, &read );
        if (hr != S_OK) goto fail;
    }
    stat.cbSize.QuadPart -= info_size;

    /* set Stream pointer to beginning of bitmap bits */
    if (cache_entry->load_stream_num == STREAM_NUMBER_CONTENTS && file.bfOffBits)
    {
        LARGE_INTEGER skip;

        skip.QuadPart = file.bfOffBits - sizeof(file) - info_size;
        if (stat.cbSize.QuadPart < skip.QuadPart) goto fail;
        hr = IStream_Seek( stm, skip, STREAM_SEEK_CUR, NULL );
        if (hr != S_OK) goto fail;
        stat.cbSize.QuadPart -= skip.QuadPart;
    }

    hr = IStream_Read( stm, dib + info_size, stat.cbSize.LowPart, &read );
    if (hr != S_OK) goto fail;

    if (bi_size >= sizeof(*info))
    {
        info = (BITMAPINFOHEADER *)dib;
        if (info->biXPelsPerMeter == 0 || info->biYPelsPerMeter == 0)
        {
            HDC hdc = GetDC( 0 );
            info->biXPelsPerMeter = MulDiv( GetDeviceCaps( hdc, LOGPIXELSX ), 10000, 254 );
            info->biYPelsPerMeter = MulDiv( GetDeviceCaps( hdc, LOGPIXELSY ), 10000, 254 );
            ReleaseDC( 0, hdc );
        }
    }

    GlobalUnlock( hglobal );

    cache_entry->stgmedium.tymed = TYMED_HGLOBAL;
    cache_entry->stgmedium.hGlobal = hglobal;

    return hr;

fail:
    GlobalUnlock( hglobal );
    GlobalFree( hglobal );
    return hr;

}

static HRESULT load_emf( DataCacheEntry *cache_entry, IStream *stm )
{
    HRESULT hr;

    if (cache_entry->load_stream_num != STREAM_NUMBER_CONTENTS)
    {
        STGMEDIUM stgmed;

        hr = load_mf_pict( cache_entry, stm );
        if (SUCCEEDED( hr ))
        {
            hr = synthesize_emf( cache_entry->stgmedium.hMetaFilePict, &stgmed );
            ReleaseStgMedium( &cache_entry->stgmedium );
        }
        if (SUCCEEDED( hr ))
            cache_entry->stgmedium = stgmed;
    }
    else
    {
        STATSTG stat;
        BYTE *data;
        ULONG read, size_bits;

        hr = IStream_Stat( stm, &stat, STATFLAG_NONAME );

        if (SUCCEEDED( hr ))
        {
            data = HeapAlloc( GetProcessHeap(), 0, stat.cbSize.LowPart );
            if (!data) return E_OUTOFMEMORY;

            hr = IStream_Read( stm, data, stat.cbSize.LowPart, &read );
            if (hr != S_OK)
            {
                HeapFree( GetProcessHeap(), 0, data );
                return hr;
            }

            if (read <= sizeof(DWORD) + sizeof(ENHMETAHEADER))
            {
                HeapFree( GetProcessHeap(), 0, data );
                return E_FAIL;
            }
            size_bits = read - sizeof(DWORD) - sizeof(ENHMETAHEADER);
            cache_entry->stgmedium.hEnhMetaFile = SetEnhMetaFileBits( size_bits, data + (read - size_bits) );
            cache_entry->stgmedium.tymed = TYMED_ENHMF;
            cache_entry->stgmedium.pUnkForRelease = NULL;

            HeapFree( GetProcessHeap(), 0, data );
        }
    }

    return hr;
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
static HRESULT DataCacheEntry_LoadData(DataCacheEntry *cache_entry, IStorage *stg)
{
    HRESULT hr;
    IStream *stm;

    if (!stg) return OLE_E_BLANK;
    hr = open_pres_stream( stg, cache_entry->load_stream_num, &stm );
    if (FAILED(hr)) return hr;

    switch (cache_entry->fmtetc.cfFormat)
    {
    case CF_METAFILEPICT:
        hr = load_mf_pict( cache_entry, stm );
        break;

    case CF_DIB:
        hr = load_dib( cache_entry, stm );
        break;

    case CF_ENHMETAFILE:
        hr = load_emf( cache_entry, stm );
        break;

    default:
        FIXME( "Unimplemented clip format %x\n", cache_entry->fmtetc.cfFormat );
        hr = E_NOTIMPL;
    }

    IStream_Release( stm );
    return hr;
}

static void init_stream_header(DataCacheEntry *entry, PresentationDataHeader *header)
{
    if (entry->fmtetc.ptd)
        FIXME("ptd not serialized\n");
    header->tdSize = sizeof(header->tdSize);
    header->dvAspect = entry->fmtetc.dwAspect;
    header->lindex = entry->fmtetc.lindex;
    header->advf = entry->advise_flags;
    header->unknown7 = 0;
    header->dwObjectExtentX = 0;
    header->dwObjectExtentY = 0;
    header->dwSize = 0;
}

static HRESULT save_dib(DataCacheEntry *entry, BOOL contents, IStream *stream)
{
    HRESULT hr = S_OK;
    int data_size = 0;
    BITMAPINFO *bmi = NULL;

    if (entry->stgmedium.tymed != TYMED_NULL)
    {
        data_size = GlobalSize(entry->stgmedium.hGlobal);
        bmi = GlobalLock(entry->stgmedium.hGlobal);
    }

    if (!contents)
    {
        PresentationDataHeader header;

        init_stream_header(entry, &header);
        hr = write_clipformat(stream, entry->fmtetc.cfFormat);
        if (FAILED(hr)) goto end;
        if (data_size)
        {
            header.dwSize = data_size;
            /* Size in units of 0.01mm (ie. MM_HIMETRIC) */
            if (bmi->bmiHeader.biXPelsPerMeter != 0 && bmi->bmiHeader.biYPelsPerMeter != 0)
            {
                header.dwObjectExtentX = MulDiv(bmi->bmiHeader.biWidth, 100000, bmi->bmiHeader.biXPelsPerMeter);
                header.dwObjectExtentY = MulDiv(bmi->bmiHeader.biHeight, 100000, bmi->bmiHeader.biYPelsPerMeter);
            }
            else
            {
                HDC hdc = GetDC(0);
                header.dwObjectExtentX = MulDiv(bmi->bmiHeader.biWidth, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
                header.dwObjectExtentY = MulDiv(bmi->bmiHeader.biHeight, 2540, GetDeviceCaps(hdc, LOGPIXELSY));
                ReleaseDC(0, hdc);
            }
        }
        hr = IStream_Write(stream, &header, sizeof(PresentationDataHeader), NULL);
        if (hr == S_OK && data_size)
            hr = IStream_Write(stream, bmi, data_size, NULL);
    }
    else if(data_size)
    {
        BITMAPFILEHEADER bmp_fhdr;

        bmp_fhdr.bfType = 0x4d42;
        bmp_fhdr.bfSize = data_size + sizeof(BITMAPFILEHEADER);
        bmp_fhdr.bfReserved1 = bmp_fhdr.bfReserved2 = 0;
        bmp_fhdr.bfOffBits = bitmap_info_size(bmi, DIB_RGB_COLORS) + sizeof(BITMAPFILEHEADER);
        hr = IStream_Write(stream, &bmp_fhdr, sizeof(BITMAPFILEHEADER), NULL);
        if (hr == S_OK)
            hr = IStream_Write(stream, bmi, data_size, NULL);
    }

end:
    if (bmi) GlobalUnlock(entry->stgmedium.hGlobal);
    return hr;
}

static HRESULT save_mfpict(DataCacheEntry *entry, BOOL contents, IStream *stream)
{
    HRESULT hr = S_OK;
    int data_size = 0;
    void *data = NULL;
    METAFILEPICT *mfpict = NULL;

    if (!contents)
    {
        PresentationDataHeader header;

        init_stream_header(entry, &header);
        hr = write_clipformat(stream, entry->fmtetc.cfFormat);
        if (FAILED(hr)) return hr;
        if (entry->stgmedium.tymed != TYMED_NULL)
        {
            mfpict = GlobalLock(entry->stgmedium.hMetaFilePict);
            if (!mfpict)
                return DV_E_STGMEDIUM;
            data_size = GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
            header.dwObjectExtentX = mfpict->xExt;
            header.dwObjectExtentY = mfpict->yExt;
            header.dwSize = data_size;
            data = HeapAlloc(GetProcessHeap(), 0, header.dwSize);
            if (!data)
            {
                GlobalUnlock(entry->stgmedium.hMetaFilePict);
                return E_OUTOFMEMORY;
            }
            GetMetaFileBitsEx(mfpict->hMF, header.dwSize, data);
            GlobalUnlock(entry->stgmedium.hMetaFilePict);
        }
        hr = IStream_Write(stream, &header, sizeof(PresentationDataHeader), NULL);
        if (hr == S_OK && data_size)
            hr = IStream_Write(stream, data, data_size, NULL);
        HeapFree(GetProcessHeap(), 0, data);
    }
    else if (entry->stgmedium.tymed != TYMED_NULL)
    {
        struct meta_placeable meta_place_rec;
        WORD *check;

        mfpict = GlobalLock(entry->stgmedium.hMetaFilePict);
        if (!mfpict)
            return DV_E_STGMEDIUM;
        data_size = GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
        data = HeapAlloc(GetProcessHeap(), 0, data_size);
        if (!data)
        {
            GlobalUnlock(entry->stgmedium.hMetaFilePict);
            return E_OUTOFMEMORY;
        }
        GetMetaFileBitsEx(mfpict->hMF, data_size, data);

        /* units are in 1/8th of a point (1 point is 1/72th of an inch) */
        meta_place_rec.key = 0x9ac6cdd7;
        meta_place_rec.hwmf = 0;
        meta_place_rec.inch = 576;
        meta_place_rec.bounding_box[0] = 0;
        meta_place_rec.bounding_box[1] = 0;
        meta_place_rec.bounding_box[2] = 0;
        meta_place_rec.bounding_box[3] = 0;
        meta_place_rec.checksum = 0;
        meta_place_rec.reserved = 0;

        /* These values are rounded down so MulDiv won't do the right thing */
        meta_place_rec.bounding_box[2] = (LONGLONG)mfpict->xExt * meta_place_rec.inch / 2540;
        meta_place_rec.bounding_box[3] = (LONGLONG)mfpict->yExt * meta_place_rec.inch / 2540;
        GlobalUnlock(entry->stgmedium.hMetaFilePict);

        for (check = (WORD *)&meta_place_rec; check != &meta_place_rec.checksum; check++)
            meta_place_rec.checksum ^= *check;
        hr = IStream_Write(stream, &meta_place_rec, sizeof(struct meta_placeable), NULL);
        if (hr == S_OK && data_size)
            hr = IStream_Write(stream, data, data_size, NULL);
        HeapFree(GetProcessHeap(), 0, data);
    }

    return hr;
}

static HRESULT save_emf(DataCacheEntry *entry, BOOL contents, IStream *stream)
{
    HRESULT hr = S_OK;
    int data_size = 0;
    BYTE *data;

    if (!contents)
    {
        PresentationDataHeader header;
        METAFILEPICT *mfpict;
        HDC hdc = GetDC(0);

        init_stream_header(entry, &header);
        hr = write_clipformat(stream, entry->fmtetc.cfFormat);
        if (FAILED(hr))
        {
            ReleaseDC(0, hdc);
            return hr;
        }
        data_size = GetWinMetaFileBits(entry->stgmedium.hEnhMetaFile, 0, NULL, MM_ANISOTROPIC, hdc);
        header.dwSize = data_size;
        data = HeapAlloc(GetProcessHeap(), 0, header.dwSize);
        if (!data)
        {
            ReleaseDC(0, hdc);
            return E_OUTOFMEMORY;
        }
        GetWinMetaFileBits(entry->stgmedium.hEnhMetaFile, header.dwSize, data, MM_ANISOTROPIC, hdc);
        ReleaseDC(0, hdc);
        mfpict = (METAFILEPICT *)data;
        header.dwObjectExtentX = mfpict->xExt;
        header.dwObjectExtentY = mfpict->yExt;
        hr = IStream_Write(stream, &header, sizeof(PresentationDataHeader), NULL);
        if (hr == S_OK && data_size)
            hr = IStream_Write(stream, data, data_size, NULL);
        HeapFree(GetProcessHeap(), 0, data);
    }
    else if (entry->stgmedium.tymed != TYMED_NULL)
    {
        data_size = GetEnhMetaFileBits(entry->stgmedium.hEnhMetaFile, 0, NULL);
        data = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) + sizeof(ENHMETAHEADER) + data_size);
        if (!data) return E_OUTOFMEMORY;
        *((DWORD *)data) = sizeof(ENHMETAHEADER);
        GetEnhMetaFileBits(entry->stgmedium.hEnhMetaFile, data_size, data + sizeof(DWORD) + sizeof(ENHMETAHEADER));
        memcpy(data + sizeof(DWORD), data + sizeof(DWORD) + sizeof(ENHMETAHEADER), sizeof(ENHMETAHEADER));
        data_size += sizeof(DWORD) + sizeof(ENHMETAHEADER);
        hr = IStream_Write(stream, data, data_size, NULL);
        HeapFree(GetProcessHeap(), 0, data);
    }

    return hr;
}

static HRESULT save_view_cache(DataCacheEntry *entry, IStream *stream)
{
    HRESULT hr;
    PresentationDataHeader header;

    init_stream_header(entry, &header);
    hr = write_clipformat(stream, entry->fmtetc.cfFormat);
    if (SUCCEEDED(hr))
        hr = IStream_Write(stream, &header, FIELD_OFFSET(PresentationDataHeader, unknown7), NULL);

    return hr;
}

static HRESULT create_stream(DataCacheEntry *cache_entry, IStorage *storage,
                             BOOL contents, IStream **stream)
{
    WCHAR pres[] = {2,'O','l','e','P','r','e','s',
        '0' + (cache_entry->save_stream_num / 100) % 10,
        '0' + (cache_entry->save_stream_num / 10) % 10,
        '0' + cache_entry->save_stream_num % 10, 0};
    const WCHAR *name;

    if (contents)
        name = L"CONTENTS";
    else
        name = pres;

    return IStorage_CreateStream(storage, name,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                                 0, 0, stream);
}

static HRESULT DataCacheEntry_Save(DataCacheEntry *cache_entry, IStorage *storage,
                                   BOOL same_as_load)
{
    HRESULT hr;
    IStream *stream;
    BOOL contents = (cache_entry->id == 1);

    TRACE("stream_number = %d, fmtetc = %s\n", cache_entry->save_stream_num, debugstr_formatetc(&cache_entry->fmtetc));

    hr = create_stream(cache_entry, storage, contents, &stream);
    if (FAILED(hr))
        return hr;

    switch (cache_entry->fmtetc.cfFormat)
    {
    case CF_DIB:
        hr = save_dib(cache_entry, contents, stream);
        break;
    case CF_METAFILEPICT:
        hr = save_mfpict(cache_entry, contents, stream);
        break;
    case CF_ENHMETAFILE:
        hr = save_emf(cache_entry, contents, stream);
        break;
    case 0:
        hr = save_view_cache(cache_entry, stream);
        break;
    default:
        FIXME("got unsupported clipboard format %x\n", cache_entry->fmtetc.cfFormat);
    }

    IStream_Release(stream);
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
        const METAFILEPICT *src_mfpict = GlobalLock(src_stgm->hMetaFilePict);
        METAFILEPICT *dest_mfpict;

        if (!src_mfpict)
            return DV_E_STGMEDIUM;
        dest_stgm->hMetaFilePict = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
        dest_mfpict = GlobalLock(dest_stgm->hMetaFilePict);
        if (!dest_mfpict)
        {
            GlobalUnlock(src_stgm->hMetaFilePict);
            return E_OUTOFMEMORY;
        }
        *dest_mfpict = *src_mfpict;
        dest_mfpict->hMF = CopyMetaFileW(src_mfpict->hMF, NULL);
        GlobalUnlock(src_stgm->hMetaFilePict);
        GlobalUnlock(dest_stgm->hMetaFilePict);
    }
    else if (src_stgm->tymed != TYMED_NULL)
    {
        dest_stgm->hGlobal = OleDuplicateData(src_stgm->hGlobal, cf, GMEM_MOVEABLE);
        if (!dest_stgm->hGlobal)
            return E_OUTOFMEMORY;
    }
    dest_stgm->tymed = src_stgm->tymed;
    dest_stgm->pUnkForRelease = src_stgm->pUnkForRelease;
    if (dest_stgm->pUnkForRelease)
        IUnknown_AddRef(dest_stgm->pUnkForRelease);
    return S_OK;
}

static HRESULT synthesize_dib( HBITMAP bm, STGMEDIUM *med )
{
    HDC hdc = GetDC( 0 );
    BITMAPINFOHEADER header;
    BITMAPINFO *bmi;
    HRESULT hr = E_FAIL;
    DWORD header_size;

    memset( &header, 0, sizeof(header) );
    header.biSize = sizeof(header);
    if (!GetDIBits( hdc, bm, 0, 0, NULL, (BITMAPINFO *)&header, DIB_RGB_COLORS )) goto done;

    header_size = bitmap_info_size( (BITMAPINFO *)&header, DIB_RGB_COLORS );
    if (!(med->hGlobal = GlobalAlloc( GMEM_MOVEABLE, header_size + header.biSizeImage ))) goto done;
    bmi = GlobalLock( med->hGlobal );
    memset( bmi, 0, header_size );
    memcpy( bmi, &header, header.biSize );
    GetDIBits( hdc, bm, 0, abs(header.biHeight), (char *)bmi + header_size, bmi, DIB_RGB_COLORS );
    GlobalUnlock( med->hGlobal );
    med->tymed = TYMED_HGLOBAL;
    med->pUnkForRelease = NULL;
    hr = S_OK;

done:
    ReleaseDC( 0, hdc );
    return hr;
}

static HRESULT synthesize_bitmap( HGLOBAL dib, STGMEDIUM *med )
{
    HRESULT hr = E_FAIL;
    BITMAPINFO *bmi;
    HDC hdc = GetDC( 0 );

    if ((bmi = GlobalLock( dib )))
    {
        /* FIXME: validate data size */
        med->hBitmap = CreateDIBitmap( hdc, &bmi->bmiHeader, CBM_INIT,
                                       (char *)bmi + bitmap_info_size( bmi, DIB_RGB_COLORS ),
                                       bmi, DIB_RGB_COLORS );
        GlobalUnlock( dib );
        med->tymed = TYMED_GDI;
        med->pUnkForRelease = NULL;
        hr = S_OK;
    }
    ReleaseDC( 0, hdc );
    return hr;
}

static HRESULT DataCacheEntry_SetData(DataCacheEntry *cache_entry,
                                      const FORMATETC *formatetc,
                                      STGMEDIUM *stgmedium,
                                      BOOL fRelease)
{
    STGMEDIUM copy;
    HRESULT hr;

    if ((!cache_entry->fmtetc.cfFormat && !formatetc->cfFormat) ||
        (cache_entry->fmtetc.tymed == TYMED_NULL && formatetc->tymed == TYMED_NULL) ||
        stgmedium->tymed == TYMED_NULL)
    {
        WARN("invalid formatetc\n");
        return DV_E_FORMATETC;
    }

    cache_entry->dirty = TRUE;
    ReleaseStgMedium(&cache_entry->stgmedium);

    if (formatetc->cfFormat == CF_BITMAP)
    {
        hr = synthesize_dib( stgmedium->hBitmap, &copy );
        if (FAILED(hr)) return hr;
        if (fRelease) ReleaseStgMedium(stgmedium);
        stgmedium = &copy;
        fRelease = TRUE;
    }
    else if (formatetc->cfFormat == CF_METAFILEPICT && cache_entry->fmtetc.cfFormat == CF_ENHMETAFILE)
    {
        hr = synthesize_emf( stgmedium->hMetaFilePict, &copy );
        if (FAILED(hr)) return hr;
        if (fRelease) ReleaseStgMedium(stgmedium);
        stgmedium = &copy;
        fRelease = TRUE;
    }

    if (fRelease)
    {
        cache_entry->stgmedium = *stgmedium;
        return S_OK;
    }
    else
        return copy_stg_medium(cache_entry->fmtetc.cfFormat, &cache_entry->stgmedium, stgmedium);
}

static HRESULT DataCacheEntry_GetData(DataCacheEntry *cache_entry, IStorage *stg, FORMATETC *fmt, STGMEDIUM *stgmedium)
{
    if (cache_entry->stgmedium.tymed == TYMED_NULL && cache_entry->load_stream_num != STREAM_NUMBER_NOT_SET)
    {
        HRESULT hr = DataCacheEntry_LoadData(cache_entry, stg);
        if (FAILED(hr))
            return hr;
    }
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
        return OLE_E_BLANK;

    if (fmt->cfFormat == CF_BITMAP)
        return synthesize_bitmap( cache_entry->stgmedium.hGlobal, stgmedium );

    return copy_stg_medium(cache_entry->fmtetc.cfFormat, stgmedium, &cache_entry->stgmedium);
}

static inline HRESULT DataCacheEntry_DiscardData(DataCacheEntry *cache_entry)
{
    ReleaseStgMedium(&cache_entry->stgmedium);
    return S_OK;
}

static inline DWORD tymed_from_cf( DWORD cf )
{
    switch( cf )
    {
    case CF_BITMAP:       return TYMED_GDI;
    case CF_METAFILEPICT: return TYMED_MFPICT;
    case CF_ENHMETAFILE:  return TYMED_ENHMF;
    case CF_DIB:
    default:              return TYMED_HGLOBAL;
    }
}

/****************************************************************
 *  create_automatic_entry
 *
 * Creates an appropriate cache entry for one of the CLSID_Picture_
 * classes.  The connection id of the entry is one.  Any pre-existing
 * automatic entry is re-assigned a new connection id, and moved to
 * the end of the list.
 */
static HRESULT create_automatic_entry(DataCache *cache, const CLSID *clsid)
{
    static const struct data
    {
        const CLSID *clsid;
        FORMATETC fmt;
    } data[] =
    {
        { &CLSID_Picture_Dib,         { CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL } },
        { &CLSID_Picture_Metafile,    { CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT } },
        { &CLSID_Picture_EnhMetafile, { CF_ENHMETAFILE,  0, DVASPECT_CONTENT, -1, TYMED_ENHMF } },
        { NULL }
    };
    const struct data *ptr = data;
    struct list *head;
    DataCacheEntry *entry;

    if (IsEqualCLSID( &cache->clsid, clsid )) return S_OK;

    /* move and reassign any pre-existing automatic entry */
    if ((head = list_head( &cache->cache_list )))
    {
        entry = LIST_ENTRY( head, DataCacheEntry, entry );
        if (entry->id == 1)
        {
            list_remove( &entry->entry );
            entry->id = cache->last_cache_id++;
            list_add_tail( &cache->cache_list, &entry->entry );
        }
    }

    while (ptr->clsid)
    {
        if (IsEqualCLSID( clsid, ptr->clsid ))
        {
            cache->clsid_static = TRUE;
            return DataCache_CreateEntry( cache, &ptr->fmt, 0, TRUE, NULL );
        }
        ptr++;
    }
    cache->clsid_static = FALSE;
    return S_OK;
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
    if (this->outer_unk == iface) /* non-aggregated, return IUnknown from IOleCache2 */
      *ppvObject = &this->IOleCache2_iface;
    else
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

  return IUnknown_QueryInterface(this->outer_unk, riid, ppvObject);
}

/************************************************************************
 * DataCache_IDataObject_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IDataObject_AddRef(
            IDataObject*     iface)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_AddRef(this->outer_unk);
}

/************************************************************************
 * DataCache_IDataObject_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IDataObject_Release(
            IDataObject*     iface)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_Release(this->outer_unk);
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

    TRACE("(%p, %s, %p)\n", iface, debugstr_formatetc(pformatetcIn), pmedium);

    memset(pmedium, 0, sizeof(*pmedium));

    cache_entry = DataCache_GetEntryForFormatEtc(This, pformatetcIn);
    if (!cache_entry)
        return OLE_E_BLANK;

    return DataCacheEntry_GetData(cache_entry, This->presentationStorage, pformatetcIn, pmedium);
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

  return IUnknown_QueryInterface(this->outer_unk, riid, ppvObject);
}

/************************************************************************
 * DataCache_IPersistStorage_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IPersistStorage_AddRef(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_AddRef(this->outer_unk);
}

/************************************************************************
 * DataCache_IPersistStorage_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IPersistStorage_Release(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_Release(this->outer_unk);
}

/************************************************************************
 * DataCache_GetClassID (IPersistStorage)
 *
 */
static HRESULT WINAPI DataCache_GetClassID(IPersistStorage *iface, CLSID *clsid)
{
    DataCache *This = impl_from_IPersistStorage( iface );

    TRACE( "(%p, %p) returning %s\n", iface, clsid, debugstr_guid(&This->clsid) );
    *clsid = This->clsid;

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
    CLSID clsid;
    HRESULT hr;

    TRACE("(%p, %p)\n", iface, pStg);

    if (This->presentationStorage != NULL)
        return CO_E_ALREADYINITIALIZED;

    This->presentationStorage = pStg;

    IStorage_AddRef(This->presentationStorage);
    This->dirty = TRUE;
    ReadClassStg( pStg, &clsid );
    hr = create_automatic_entry( This, &clsid );
    if (FAILED(hr))
    {
        IStorage_Release( pStg );
        This->presentationStorage = NULL;
        return hr;
    }
    This->clsid = clsid;

    return S_OK;
}


static HRESULT add_cache_entry( DataCache *This, const FORMATETC *fmt, DWORD advf, int stream_number )
{
    DataCacheEntry *cache_entry;
    HRESULT hr = S_OK;

    TRACE( "loading entry with formatetc: %s\n", debugstr_formatetc( fmt ) );

    cache_entry = DataCache_GetEntryForFormatEtc( This, fmt );
    if (!cache_entry)
        hr = DataCache_CreateEntry( This, fmt, advf, FALSE, &cache_entry );
    if (SUCCEEDED( hr ))
    {
        DataCacheEntry_DiscardData( cache_entry );
        cache_entry->load_stream_num = stream_number;
        cache_entry->save_stream_num = stream_number;
        cache_entry->dirty = FALSE;
    }
    return hr;
}

static HRESULT parse_pres_streams( DataCache *cache, IStorage *stg )
{
    HRESULT hr;
    IStream *stm;
    PresentationDataHeader header;
    ULONG actual_read;
    CLIPFORMAT clipformat;
    FORMATETC fmtetc;
    int stream_number = 0;

    do
    {
        hr = open_pres_stream( stg, stream_number, &stm );
        if (FAILED(hr)) break;

        hr = read_clipformat( stm, &clipformat );

        if (hr == S_OK) hr = IStream_Read( stm, &header, sizeof(header), &actual_read );

        if (hr == S_OK && actual_read == sizeof(header))
        {
            fmtetc.cfFormat = clipformat;
            fmtetc.ptd = NULL; /* FIXME */
            fmtetc.dwAspect = header.dvAspect;
            fmtetc.lindex = header.lindex;
            fmtetc.tymed = tymed_from_cf( clipformat );

            add_cache_entry( cache, &fmtetc, header.advf, stream_number );
        }
        IStream_Release( stm );
        stream_number++;
    } while (hr == S_OK);

    return S_OK;
}

static HRESULT parse_contents_stream( DataCache *cache, IStorage *stg )
{
    HRESULT hr;
    IStream *stm;
    DataCacheEntry *cache_entry;

    hr = open_pres_stream( stg, STREAM_NUMBER_CONTENTS, &stm );
    if (FAILED( hr )) return hr;

    hr = get_static_entry( cache, &cache_entry );
    if (hr == S_OK)
    {
        cache_entry->load_stream_num = STREAM_NUMBER_CONTENTS;
        cache_entry->save_stream_num = STREAM_NUMBER_CONTENTS;
        cache_entry->dirty = FALSE;
    }

    IStream_Release( stm );
    return hr;
}

/************************************************************************
 * DataCache_Load (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_Load doesn't
 * actually load anything. Instead, it holds on to the storage pointer
 * and it will load the presentation information when the
 * IDataObject_GetData or IViewObject2_Draw methods are called.
 */
static HRESULT WINAPI DataCache_Load( IPersistStorage *iface, IStorage *stg )
{
    DataCache *This = impl_from_IPersistStorage(iface);
    HRESULT hr;
    CLSID clsid;
    DataCacheEntry *entry, *cursor2;

    TRACE("(%p, %p)\n", iface, stg);

    IPersistStorage_HandsOffStorage( iface );

    LIST_FOR_EACH_ENTRY_SAFE( entry, cursor2, &This->cache_list, DataCacheEntry, entry )
        DataCacheEntry_Destroy( This, entry );
    This->clsid = CLSID_NULL;

    ReadClassStg( stg, &clsid );
    hr = create_automatic_entry( This, &clsid );
    if (FAILED( hr )) return hr;

    This->clsid = clsid;

    if (This->clsid_static)
    {
        hr = parse_contents_stream( This, stg );
        if (FAILED(hr)) hr = parse_pres_streams( This, stg );
    }
    else
        hr = parse_pres_streams( This, stg );

    if (SUCCEEDED( hr ))
    {
        This->dirty = FALSE;
        This->presentationStorage = stg;
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
static HRESULT WINAPI DataCache_Save(IPersistStorage* iface, IStorage *stg, BOOL same_as_load)
{
    DataCache *This = impl_from_IPersistStorage(iface);
    DataCacheEntry *cache_entry;
    HRESULT hr = S_OK;
    int stream_number = 0;

    TRACE("(%p, %p, %d)\n", iface, stg, same_as_load);

    /* assign stream numbers to the cache entries */
    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        if (cache_entry->save_stream_num != stream_number)
        {
            cache_entry->dirty = TRUE; /* needs to be written out again */
            cache_entry->save_stream_num = stream_number;
        }
        stream_number++;
    }

    /* write out the cache entries */
    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
    {
        if (!same_as_load || cache_entry->dirty)
        {
            hr = DataCacheEntry_Save(cache_entry, stg, same_as_load);
            if (FAILED(hr))
                break;

            if (same_as_load) cache_entry->dirty = FALSE;
        }
    }

    if (same_as_load) This->dirty = FALSE;
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

  TRACE("(%p)\n", iface);

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
    this->presentationStorage = NULL;
  }

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

  return IUnknown_QueryInterface(this->outer_unk, riid, ppvObject);
}

/************************************************************************
 * DataCache_IViewObject2_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IViewObject2_AddRef(
            IViewObject2* iface)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_AddRef(this->outer_unk);
}

/************************************************************************
 * DataCache_IViewObject2_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IViewObject2_Release(
            IViewObject2* iface)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_Release(this->outer_unk);
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

  TRACE("%p, %lx, %ld, %p, %p, %p, %p, %p, %p, %Ix.\n",
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
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && (cache_entry->load_stream_num != STREAM_NUMBER_NOT_SET))
    {
      hres = DataCacheEntry_LoadData(cache_entry, This->presentationStorage);
      if (FAILED(hres))
        continue;
    }

    /* no data */
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
      continue;

    if (pfnContinue && !pfnContinue(dwContinue)) return E_ABORT;

    switch (cache_entry->fmtetc.cfFormat)
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
            !((mfpict = GlobalLock(cache_entry->stgmedium.hMetaFilePict))))
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

        GlobalUnlock(cache_entry->stgmedium.hMetaFilePict);

        return S_OK;
      }
      case CF_DIB:
      {
          BITMAPINFO *info;
          BYTE *bits;

          if ((cache_entry->stgmedium.tymed != TYMED_HGLOBAL) ||
              !((info = GlobalLock( cache_entry->stgmedium.hGlobal ))))
              continue;

          bits = (BYTE *) info + bitmap_info_size( info, DIB_RGB_COLORS );
          StretchDIBits( hdcDraw, lprcBounds->left, lprcBounds->top,
                         lprcBounds->right - lprcBounds->left, lprcBounds->bottom - lprcBounds->top,
                         0, 0, info->bmiHeader.biWidth, info->bmiHeader.biHeight,
                         bits, info, DIB_RGB_COLORS, SRCCOPY );

          GlobalUnlock( cache_entry->stgmedium.hGlobal );
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

  TRACE("%p, %lx, %lx, %p.\n", iface, aspects, advf, pAdvSink);

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

  TRACE("%p, %lx, %ld, %p, %p.\n", iface, dwDrawAspect, lindex, ptd, lpsizel);

  if (lpsizel==NULL)
    return E_POINTER;

  lpsizel->cx = 0;
  lpsizel->cy = 0;

  if (lindex!=-1)
    FIXME("Unimplemented flag lindex = %ld\n", lindex);

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
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && (cache_entry->load_stream_num != STREAM_NUMBER_NOT_SET))
    {
      hres = DataCacheEntry_LoadData(cache_entry, This->presentationStorage);
      if (FAILED(hres))
        continue;
    }

    /* no data */
    if (cache_entry->stgmedium.tymed == TYMED_NULL)
      continue;


    switch (cache_entry->fmtetc.cfFormat)
    {
      case CF_METAFILEPICT:
      {
          METAFILEPICT *mfpict;

          if ((cache_entry->stgmedium.tymed != TYMED_MFPICT) ||
              !((mfpict = GlobalLock(cache_entry->stgmedium.hMetaFilePict))))
            continue;

        lpsizel->cx = mfpict->xExt;
        lpsizel->cy = mfpict->yExt;

        GlobalUnlock(cache_entry->stgmedium.hMetaFilePict);

        return S_OK;
      }
      case CF_DIB:
      {
          BITMAPINFOHEADER *info;
          LONG x_pels_m, y_pels_m;


          if ((cache_entry->stgmedium.tymed != TYMED_HGLOBAL) ||
              !((info = GlobalLock( cache_entry->stgmedium.hGlobal ))))
              continue;

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

          GlobalUnlock( cache_entry->stgmedium.hGlobal );

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

  return IUnknown_QueryInterface(this->outer_unk, riid, ppvObject);
}

/************************************************************************
 * DataCache_IOleCache2_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCache2_AddRef(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_AddRef(this->outer_unk);
}

/************************************************************************
 * DataCache_IOleCache2_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCache2_Release(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_Release(this->outer_unk);
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
    FORMATETC fmt_cpy;

    TRACE("%p, %#lx, %p.\n", pformatetc, advf, pdwConnection);

    if (!pformatetc || !pdwConnection)
        return E_INVALIDARG;

    TRACE("pformatetc = %s\n", debugstr_formatetc(pformatetc));

    fmt_cpy = *pformatetc; /* No need for a deep copy */
    if (fmt_cpy.cfFormat == CF_BITMAP && fmt_cpy.tymed == TYMED_GDI)
    {
        fmt_cpy.cfFormat = CF_DIB;
        fmt_cpy.tymed = TYMED_HGLOBAL;
    }

    /* View caching DVASPECT_ICON gets converted to CF_METAFILEPICT */
    if (fmt_cpy.dwAspect == DVASPECT_ICON && fmt_cpy.cfFormat == 0)
    {
        fmt_cpy.cfFormat = CF_METAFILEPICT;
        fmt_cpy.tymed = TYMED_MFPICT;
    }

    *pdwConnection = 0;

    cache_entry = DataCache_GetEntryForFormatEtc(This, &fmt_cpy);
    if (cache_entry)
    {
        TRACE("found an existing cache entry\n");
        *pdwConnection = cache_entry->id;
        return CACHE_S_SAMECACHE;
    }

    if (This->clsid_static && fmt_cpy.dwAspect != DVASPECT_ICON) return DV_E_FORMATETC;

    hr = DataCache_CreateEntry(This, &fmt_cpy, advf, FALSE, &cache_entry);

    if (SUCCEEDED(hr))
    {
        *pdwConnection = cache_entry->id;
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

    TRACE("%ld\n", dwConnection);

    LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
        if (cache_entry->id == dwConnection)
        {
            DataCacheEntry_Destroy(This, cache_entry);
            return S_OK;
        }

    WARN("no connection found for %ld\n", dwConnection);

    return OLE_E_NOCONNECTION;
}

static HRESULT WINAPI DataCache_EnumCache(IOleCache2 *iface,
                                          IEnumSTATDATA **enum_stat)
{
    DataCache *This = impl_from_IOleCache2( iface );
    DataCacheEntry *cache_entry;
    int i = 0, count = 0;
    STATDATA *data;
    HRESULT hr;

    TRACE( "(%p, %p)\n", This, enum_stat );

    LIST_FOR_EACH_ENTRY( cache_entry, &This->cache_list, DataCacheEntry, entry )
    {
        count++;
        if (cache_entry->fmtetc.cfFormat == CF_DIB)
            count++;
    }

    data = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*data) );
    if (!data) return E_OUTOFMEMORY;

    LIST_FOR_EACH_ENTRY( cache_entry, &This->cache_list, DataCacheEntry, entry )
    {
        if (i == count) goto fail;
        hr = copy_formatetc( &data[i].formatetc, &cache_entry->fmtetc );
        if (FAILED(hr)) goto fail;
        data[i].advf = cache_entry->advise_flags;
        data[i].pAdvSink = NULL;
        data[i].dwConnection = cache_entry->id;
        i++;

        if (cache_entry->fmtetc.cfFormat == CF_DIB)
        {
            if (i == count) goto fail;
            hr = copy_formatetc( &data[i].formatetc, &cache_entry->fmtetc );
            if (FAILED(hr)) goto fail;
            data[i].formatetc.cfFormat = CF_BITMAP;
            data[i].formatetc.tymed = TYMED_GDI;
            data[i].advf = cache_entry->advise_flags;
            data[i].pAdvSink = NULL;
            data[i].dwConnection = cache_entry->id;
            i++;
        }
    }

    hr = EnumSTATDATA_Construct( NULL, 0, i, data, FALSE, enum_stat );
    if (SUCCEEDED(hr)) return hr;

fail:
    while (i--) CoTaskMemFree( data[i].formatetc.ptd );
    HeapFree( GetProcessHeap(), 0, data );
    return hr;
}

static HRESULT WINAPI DataCache_InitCache( IOleCache2 *iface, IDataObject *data )
{
    TRACE( "(%p %p)\n", iface, data );
    return IOleCache2_UpdateCache( iface, data, UPDFCACHE_ALLBUTNODATACACHE, NULL );
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

static BOOL entry_updatable( DataCacheEntry *entry, DWORD mode )
{
    BOOL is_blank = entry->stgmedium.tymed == TYMED_NULL;

    if ((mode & UPDFCACHE_ONLYIFBLANK) && !is_blank) return FALSE;

    if ((mode & UPDFCACHE_NODATACACHE) && (entry->advise_flags & ADVF_NODATA)) return TRUE;
    if ((mode & UPDFCACHE_ONSAVECACHE) && (entry->advise_flags & ADVFCACHE_ONSAVE)) return TRUE;
    if ((mode & UPDFCACHE_ONSTOPCACHE) && (entry->advise_flags & ADVF_DATAONSTOP)) return TRUE;
    if ((mode & UPDFCACHE_NORMALCACHE) && (entry->advise_flags == 0)) return TRUE;
    if ((mode & UPDFCACHE_IFBLANK) && (is_blank && !(entry->advise_flags & ADVF_NODATA))) return TRUE;

    return FALSE;
}

static HRESULT WINAPI DataCache_UpdateCache( IOleCache2 *iface, IDataObject *data,
                                             DWORD mode, void *reserved )
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;
    STGMEDIUM med;
    HRESULT hr = S_OK;
    CLIPFORMAT view_list[] = { CF_METAFILEPICT, CF_ENHMETAFILE, CF_DIB, CF_BITMAP };
    FORMATETC fmt;
    int i, slots = 0;
    BOOL done_one = FALSE;

    TRACE("%p, %p, %#lx, %p.\n", iface, data, mode, reserved );

    LIST_FOR_EACH_ENTRY( cache_entry, &This->cache_list, DataCacheEntry, entry )
    {
        slots++;

        if (!entry_updatable( cache_entry, mode ))
        {
            done_one = TRUE;
            continue;
        }

        fmt = cache_entry->fmtetc;

        if (fmt.cfFormat)
        {
            hr = IDataObject_GetData( data, &fmt, &med );
            if (hr != S_OK && fmt.cfFormat == CF_DIB)
            {
                fmt.cfFormat = CF_BITMAP;
                fmt.tymed = TYMED_GDI;
                hr = IDataObject_GetData( data, &fmt, &med );
            }
            if (hr != S_OK && fmt.cfFormat == CF_ENHMETAFILE)
            {
                fmt.cfFormat = CF_METAFILEPICT;
                fmt.tymed = TYMED_MFPICT;
                hr = IDataObject_GetData( data, &fmt, &med );
            }
            if (hr == S_OK)
            {
                hr = DataCacheEntry_SetData( cache_entry, &fmt, &med, TRUE );
                if (hr != S_OK) ReleaseStgMedium( &med );
                else done_one = TRUE;
            }
        }
        else
        {
            for (i = 0; i < ARRAY_SIZE(view_list); i++)
            {
                fmt.cfFormat = view_list[i];
                fmt.tymed = tymed_from_cf( fmt.cfFormat );
                hr = IDataObject_QueryGetData( data, &fmt );
                if (hr == S_OK)
                {
                    hr = IDataObject_GetData( data, &fmt, &med );
                    if (hr == S_OK)
                    {
                        if (fmt.cfFormat == CF_BITMAP)
                        {
                            cache_entry->fmtetc.cfFormat = CF_DIB;
                            cache_entry->fmtetc.tymed = TYMED_HGLOBAL;
                        }
                        else
                        {
                            cache_entry->fmtetc.cfFormat = fmt.cfFormat;
                            cache_entry->fmtetc.tymed = fmt.tymed;
                        }
                        hr = DataCacheEntry_SetData( cache_entry, &fmt, &med, TRUE );
                        if (hr != S_OK) ReleaseStgMedium( &med );
                        else done_one = TRUE;
                        break;
                    }
                }
            }
        }
    }

    return (!slots || done_one) ? S_OK : CACHE_E_NOCACHE_UPDATED;
}

static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions)
{
    DataCache *This = impl_from_IOleCache2(iface);
    DataCacheEntry *cache_entry;
    HRESULT hr = S_OK;

    TRACE("%ld\n", dwDiscardOptions);

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

  return IUnknown_QueryInterface(this->outer_unk, riid, ppvObject);
}

/************************************************************************
 * DataCache_IOleCacheControl_AddRef (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCacheControl_AddRef(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_AddRef(this->outer_unk);
}

/************************************************************************
 * DataCache_IOleCacheControl_Release (IUnknown)
 */
static ULONG WINAPI DataCache_IOleCacheControl_Release(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_Release(this->outer_unk);
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
  newObject->IUnknown_inner.lpVtbl = &DataCache_NDIUnknown_VTable;
  newObject->IPersistStorage_iface.lpVtbl = &DataCache_IPersistStorage_VTable;
  newObject->IViewObject2_iface.lpVtbl = &DataCache_IViewObject2_VTable;
  newObject->IOleCache2_iface.lpVtbl = &DataCache_IOleCache2_VTable;
  newObject->IOleCacheControl_iface.lpVtbl = &DataCache_IOleCacheControl_VTable;
  newObject->IAdviseSink_iface.lpVtbl = &DataCache_IAdviseSink_VTable;
  newObject->outer_unk = pUnkOuter ? pUnkOuter : &newObject->IUnknown_inner;
  newObject->ref = 1;

  /*
   * Initialize the other members of the structure.
   */
  newObject->sinkAspects = 0;
  newObject->sinkAdviseFlag = 0;
  newObject->sinkInterface = 0;
  newObject->clsid = CLSID_NULL;
  newObject->clsid_static = FALSE;
  newObject->presentationStorage = NULL;
  list_init(&newObject->cache_list);
  newObject->last_cache_id = 2;
  newObject->dirty = FALSE;
  newObject->running_object = NULL;

  create_automatic_entry( newObject, clsid );
  newObject->clsid = *clsid;

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
    return E_INVALIDARG;

  /*
   * Try to construct a new instance of the class.
   */
  newCache = DataCache_Construct(rclsid,
				 pUnkOuter);

  if (newCache == 0)
    return E_OUTOFMEMORY;

  hr = IUnknown_QueryInterface(&newCache->IUnknown_inner, riid, ppvObj);
  IUnknown_Release(&newCache->IUnknown_inner);

  return hr;
}
