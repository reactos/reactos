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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "ole2.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/****************************************************************************
 * PresentationDataHeader
 *
 * This structure represents the header of the \002OlePresXXX stream in
 * the OLE object strorage.
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
   * This storage pointer is set through a call to
   * IPersistStorage_Load. This is where the visual
   * representation of the object is stored.
   */
  IStorage *storage;
  /* connection ID */
  DWORD id;
  /* dirty flag */
  BOOL dirty;
  /* stream number (-1 if not set ) */
  unsigned short stream_number;
} DataCacheEntry;

/****************************************************************************
 * DataCache
 */
struct DataCache
{
  /*
   * List all interface VTables here
   */
  const IDataObjectVtbl*      lpVtbl;
  const IUnknownVtbl*         lpvtblNDIUnknown;
  const IPersistStorageVtbl*  lpvtblIPersistStorage;
  const IViewObject2Vtbl*     lpvtblIViewObject;
  const IOleCache2Vtbl*       lpvtblIOleCache2;
  const IOleCacheControlVtbl* lpvtblIOleCacheControl;

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
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpVtbl));
}

static inline DataCache *impl_from_NDIUnknown( IUnknown *iface )
{
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpvtblNDIUnknown));
}

static inline DataCache *impl_from_IPersistStorage( IPersistStorage *iface )
{
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpvtblIPersistStorage));
}

static inline DataCache *impl_from_IViewObject2( IViewObject2 *iface )
{
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpvtblIViewObject));
}

static inline DataCache *impl_from_IOleCache2( IOleCache2 *iface )
{
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpvtblIOleCache2));
}

static inline DataCache *impl_from_IOleCacheControl( IOleCacheControl *iface )
{
    return (DataCache *)((char*)iface - FIELD_OFFSET(DataCache, lpvtblIOleCacheControl));
}

static const char * debugstr_formatetc(const FORMATETC *formatetc)
{
    return wine_dbg_sprintf("{ cfFormat = 0x%x, ptd = %p, dwAspect = %d, lindex = %d, tymed = %d }",
        formatetc->cfFormat, formatetc->ptd, formatetc->dwAspect,
        formatetc->lindex, formatetc->tymed);
}

/*
 * Prototypes for the methods of the DataCache class.
 */
static DataCache* DataCache_Construct(REFCLSID  clsid,
				      LPUNKNOWN pUnkOuter);
static HRESULT    DataCacheEntry_OpenPresStream(DataCacheEntry *This,
					   IStream  **pStm);

static void DataCacheEntry_Destroy(DataCacheEntry *This)
{
    list_remove(&This->entry);
    if (This->storage)
        IStorage_Release(This->storage);
    HeapFree(GetProcessHeap(), 0, This->fmtetc.ptd);
    ReleaseStgMedium(&This->stgmedium);
    HeapFree(GetProcessHeap(), 0, This);
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
    DataCacheEntry_Destroy(cache_entry);

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
static HRESULT check_valid_clipformat_and_tymed(CLIPFORMAT cfFormat, DWORD tymed)
{
    if (!cfFormat || !tymed ||
        (cfFormat == CF_METAFILEPICT && tymed == TYMED_MFPICT) ||
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

static HRESULT DataCache_CreateEntry(DataCache *This, const FORMATETC *formatetc, DataCacheEntry **cache_entry)
{
    HRESULT hr;

    hr = check_valid_clipformat_and_tymed(formatetc->cfFormat, formatetc->tymed);
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
    (*cache_entry)->stgmedium.tymed = TYMED_NULL;
    (*cache_entry)->stgmedium.pUnkForRelease = NULL;
    (*cache_entry)->storage = NULL;
    (*cache_entry)->id = This->last_cache_id++;
    (*cache_entry)->dirty = TRUE;
    (*cache_entry)->stream_number = -1;
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
        hr = IStream_Read(stream, &cf, sizeof(cf), 0);
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
static HRESULT DataCacheEntry_OpenPresStream(
  DataCacheEntry *This,
  IStream  **ppStm)
{
    STATSTG elem;
    IEnumSTATSTG *pEnum;
    HRESULT hr;

    if (!ppStm) return E_POINTER;

    hr = IStorage_EnumElements(This->storage, 0, NULL, 0, &pEnum);
    if (FAILED(hr)) return hr;

    while ((hr = IEnumSTATSTG_Next(pEnum, 1, &elem, NULL)) == S_OK)
    {
	if (DataCache_IsPresentationStream(&elem))
	{
	    IStream *pStm;

	    hr = IStorage_OpenStream(This->storage, elem.pwcsName,
				     NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0,
				     &pStm);
	    if (SUCCEEDED(hr))
	    {
		PresentationDataHeader header;
		ULONG actual_read;
                CLIPFORMAT clipformat;

                hr = read_clipformat(pStm, &clipformat);

                if (hr == S_OK)
                    hr = IStream_Read(pStm, &header, sizeof(header), &actual_read);

		/* can't use SUCCEEDED(hr): S_FALSE counts as an error */
		if (hr == S_OK && actual_read == sizeof(header)
		    && header.dvAspect == This->fmtetc.dwAspect)
		{
		    /* Rewind the stream before returning it. */
		    LARGE_INTEGER offset;
		    offset.u.LowPart = 0;
		    offset.u.HighPart = 0;
		    IStream_Seek(pStm, offset, STREAM_SEEK_SET, NULL);

		    *ppStm = pStm;

		    CoTaskMemFree(elem.pwcsName);
		    IEnumSTATSTG_Release(pEnum);

		    return S_OK;
		}

		IStream_Release(pStm);
	    }
	}

	CoTaskMemFree(elem.pwcsName);
    }

    IEnumSTATSTG_Release(pEnum);

    return (hr == S_FALSE ? OLE_E_BLANK : hr);
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
static HRESULT DataCacheEntry_LoadData(DataCacheEntry *This)
{
  IStream*      presStream = NULL;
  HRESULT       hres;
  ULARGE_INTEGER current_pos;
  STATSTG       streamInfo;
  void*         metafileBits;
  METAFILEPICT *mfpict;
  HGLOBAL       hmfpict;
  PresentationDataHeader header;
  CLIPFORMAT    clipformat;
  static const LARGE_INTEGER offset_zero;

  /*
   * Open the presentation stream.
   */
  hres = DataCacheEntry_OpenPresStream(
           This,
	   &presStream);

  if (FAILED(hres))
    return hres;

  /*
   * Get the size of the stream.
   */
  hres = IStream_Stat(presStream,
		      &streamInfo,
		      STATFLAG_NONAME);

  /*
   * Read the header.
   */

  hres = read_clipformat(presStream, &clipformat);
  if (FAILED(hres))
  {
      IStream_Release(presStream);
      return hres;
  }

  hres = IStream_Read(
                      presStream,
                      &header,
                      sizeof(PresentationDataHeader),
                      NULL);
  if (hres != S_OK)
  {
      IStream_Release(presStream);
      return E_FAIL;
  }

  hres = IStream_Seek(presStream, offset_zero, STREAM_SEEK_CUR, &current_pos);

  streamInfo.cbSize.QuadPart -= current_pos.QuadPart;

  hmfpict = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
  if (!hmfpict)
  {
      IStream_Release(presStream);
      return E_OUTOFMEMORY;
  }
  mfpict = GlobalLock(hmfpict);

  /*
   * Allocate a buffer for the metafile bits.
   */
  metafileBits = HeapAlloc(GetProcessHeap(),
			   0,
			   streamInfo.cbSize.u.LowPart);

  /*
   * Read the metafile bits.
   */
  hres = IStream_Read(
	   presStream,
	   metafileBits,
	   streamInfo.cbSize.u.LowPart,
	   NULL);

  /*
   * Create a metafile with those bits.
   */
  if (SUCCEEDED(hres))
  {
    /* FIXME: get this from the stream */
    mfpict->mm = MM_ANISOTROPIC;
    mfpict->xExt = header.dwObjectExtentX;
    mfpict->yExt = header.dwObjectExtentY;
    mfpict->hMF = SetMetaFileBitsEx(streamInfo.cbSize.u.LowPart, metafileBits);
    if (!mfpict->hMF)
      hres = E_FAIL;
  }

  GlobalUnlock(hmfpict);
  if (SUCCEEDED(hres))
  {
    This->data_cf = This->fmtetc.cfFormat;
    This->stgmedium.tymed = TYMED_MFPICT;
    This->stgmedium.u.hMetaFilePict = hmfpict;
  }
  else
    GlobalFree(hmfpict);

  /*
   * Cleanup.
   */
  HeapFree(GetProcessHeap(), 0, metafileBits);
  IStream_Release(presStream);

  return hres;
}

static HRESULT DataCacheEntry_CreateStream(DataCacheEntry *This,
                                           IStorage *storage, IStream **stream)
{
    HRESULT hr;
    WCHAR wszName[] = {2,'O','l','e','P','r','e','s',
        '0' + (This->stream_number / 100) % 10,
        '0' + (This->stream_number / 10) % 10,
        '0' + This->stream_number % 10, 0};

    /* FIXME: cache the created stream in This? */
    hr = IStorage_CreateStream(storage, wszName,
                               STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                               0, 0, stream);
    return hr;
}

static HRESULT DataCacheEntry_Save(DataCacheEntry *This, IStorage *storage,
                                   BOOL same_as_load)
{
    PresentationDataHeader header;
    HRESULT hr;
    IStream *pres_stream;
    void *data = NULL;

    TRACE("stream_number = %d, fmtetc = %s\n", This->stream_number, debugstr_formatetc(&This->fmtetc));

    hr = DataCacheEntry_CreateStream(This, storage, &pres_stream);
    if (FAILED(hr))
        return hr;

    hr = write_clipformat(pres_stream, This->data_cf);
    if (FAILED(hr))
        return hr;

    if (This->fmtetc.ptd)
        FIXME("ptd not serialized\n");
    header.unknown3 = 4;
    header.dvAspect = This->fmtetc.dwAspect;
    header.lindex = This->fmtetc.lindex;
    header.tymed = This->stgmedium.tymed;
    header.unknown7 = 0;
    header.dwObjectExtentX = 0;
    header.dwObjectExtentY = 0;
    header.dwSize = 0;

    /* size the data */
    switch (This->data_cf)
    {
        case CF_METAFILEPICT:
        {
            if (This->stgmedium.tymed != TYMED_NULL)
            {
                const METAFILEPICT *mfpict = GlobalLock(This->stgmedium.u.hMetaFilePict);
                if (!mfpict)
                {
                    IStream_Release(pres_stream);
                    return DV_E_STGMEDIUM;
                }
                header.dwObjectExtentX = mfpict->xExt;
                header.dwObjectExtentY = mfpict->yExt;
                header.dwSize = GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
                GlobalUnlock(This->stgmedium.u.hMetaFilePict);
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
    switch (This->data_cf)
    {
        case CF_METAFILEPICT:
        {
            if (This->stgmedium.tymed != TYMED_NULL)
            {
                const METAFILEPICT *mfpict = GlobalLock(This->stgmedium.u.hMetaFilePict);
                if (!mfpict)
                {
                    IStream_Release(pres_stream);
                    return DV_E_STGMEDIUM;
                }
                data = HeapAlloc(GetProcessHeap(), 0, header.dwSize);
                GetMetaFileBitsEx(mfpict->hMF, header.dwSize, data);
                GlobalUnlock(This->stgmedium.u.hMetaFilePict);
            }
            break;
        }
        default:
            break;
    }

    if (data)
        hr = IStream_Write(pres_stream, data, header.dwSize, NULL);

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

static HRESULT DataCacheEntry_SetData(DataCacheEntry *This,
                                      const FORMATETC *formatetc,
                                      const STGMEDIUM *stgmedium,
                                      BOOL fRelease)
{
    if ((!This->fmtetc.cfFormat && !formatetc->cfFormat) ||
        (This->fmtetc.tymed == TYMED_NULL && formatetc->tymed == TYMED_NULL) ||
        stgmedium->tymed == TYMED_NULL)
    {
        WARN("invalid formatetc\n");
        return DV_E_FORMATETC;
    }

    This->dirty = TRUE;
    ReleaseStgMedium(&This->stgmedium);
    This->data_cf = This->fmtetc.cfFormat ? This->fmtetc.cfFormat : formatetc->cfFormat;
    if (fRelease)
    {
        This->stgmedium = *stgmedium;
        return S_OK;
    }
    else
        return copy_stg_medium(This->data_cf,
                               &This->stgmedium, stgmedium);
}

static HRESULT DataCacheEntry_GetData(DataCacheEntry *This,
                                      STGMEDIUM *stgmedium)
{
    if (stgmedium->tymed == TYMED_NULL && This->storage)
    {
        HRESULT hr = DataCacheEntry_LoadData(This);
        if (FAILED(hr))
            return hr;
    }
    if (stgmedium->tymed == TYMED_NULL)
        return OLE_E_BLANK;
    return copy_stg_medium(This->data_cf, stgmedium, &This->stgmedium);
}

static inline HRESULT DataCacheEntry_DiscardData(DataCacheEntry *This)
{
    ReleaseStgMedium(&This->stgmedium);
    This->data_cf = This->fmtetc.cfFormat;
    return S_OK;
}

static inline void DataCacheEntry_HandsOffStorage(DataCacheEntry *This)
{
    if (This->storage)
    {
        IStorage_Release(This->storage);
        This->storage = NULL;
    }
}

/*********************************************************
 * Method implementation for the  non delegating IUnknown
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_NDIUnknown_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject)
{
  DataCache *this = impl_from_NDIUnknown(iface);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (this==0) || (ppvObject==0) )
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = iface;
  }
  else if (memcmp(&IID_IDataObject, riid, sizeof(IID_IDataObject)) == 0)
  {
    *ppvObject = (IDataObject*)&(this->lpVtbl);
  }
  else if ( (memcmp(&IID_IPersistStorage, riid, sizeof(IID_IPersistStorage)) == 0)  ||
	    (memcmp(&IID_IPersist, riid, sizeof(IID_IPersist)) == 0) )
  {
    *ppvObject = (IPersistStorage*)&(this->lpvtblIPersistStorage);
  }
  else if ( (memcmp(&IID_IViewObject, riid, sizeof(IID_IViewObject)) == 0) ||
	    (memcmp(&IID_IViewObject2, riid, sizeof(IID_IViewObject2)) == 0) )
  {
    *ppvObject = (IViewObject2*)&(this->lpvtblIViewObject);
  }
  else if ( (memcmp(&IID_IOleCache, riid, sizeof(IID_IOleCache)) == 0) ||
	    (memcmp(&IID_IOleCache2, riid, sizeof(IID_IOleCache2)) == 0) )
  {
    *ppvObject = (IOleCache2*)&(this->lpvtblIOleCache2);
  }
  else if (memcmp(&IID_IOleCacheControl, riid, sizeof(IID_IOleCacheControl)) == 0)
  {
    *ppvObject = (IOleCacheControl*)&(this->lpvtblIOleCacheControl);
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    WARN( "() : asking for unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful.
   */
  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;
}

/************************************************************************
 * DataCache_NDIUnknown_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_AddRef(
            IUnknown*      iface)
{
  DataCache *this = impl_from_NDIUnknown(iface);
  return InterlockedIncrement(&this->ref);
}

/************************************************************************
 * DataCache_NDIUnknown_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_Release(
            IUnknown*      iface)
{
  DataCache *this = impl_from_NDIUnknown(iface);
  ULONG ref;

  /*
   * Decrease the reference count on this object.
   */
  ref = InterlockedDecrement(&this->ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (ref == 0) DataCache_Destroy(this);

  return ref;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IDataObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IDataObject_AddRef(
            IDataObject*     iface)
{
  DataCache *this = impl_from_IDataObject(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IDataObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
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
 * See Windows documentation for more details on GetData.
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

static HRESULT WINAPI DataCache_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IDataObject methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IPersistStorage_AddRef(
            IPersistStorage* iface)
{
  DataCache *this = impl_from_IPersistStorage(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IPersistStorage_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
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
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_GetClassID(
            IPersistStorage* iface,
	    CLSID*           pClassID)
{
  DataCache *This = impl_from_IPersistStorage(iface);
  DataCacheEntry *cache_entry;

  TRACE("(%p, %p)\n", iface, pClassID);

  LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
  {
    if (cache_entry->storage != NULL)
    {
      STATSTG statstg;
      HRESULT hr = IStorage_Stat(cache_entry->storage, &statstg, STATFLAG_NONAME);
      if (SUCCEEDED(hr))
      {
        memcpy(pClassID, &statstg.clsid, sizeof(*pClassID));
        return S_OK;
      }
    }
  }

  memcpy(pClassID, &CLSID_NULL, sizeof(*pClassID));

  return S_OK;
}

/************************************************************************
 * DataCache_IsDirty (IPersistStorage)
 *
 * See Windows documentation for more details on IPersistStorage methods.
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
 *
 * See Windows documentation for more details on IPersistStorage methods.
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

/************************************************************************
 * DataCache_Load (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_Load doesn't
 * actually load anything. Instead, it holds on to the storage pointer
 * and it will load the presentation information when the
 * IDataObject_GetData or IViewObject2_Draw methods are called.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_Load(
            IPersistStorage* iface,
	    IStorage*        pStg)
{
    DataCache *This = impl_from_IPersistStorage(iface);
    STATSTG elem;
    IEnumSTATSTG *pEnum;
    HRESULT hr;

    TRACE("(%p, %p)\n", iface, pStg);

    if (This->presentationStorage != NULL)
      IStorage_Release(This->presentationStorage);

    This->presentationStorage = pStg;

    hr = IStorage_EnumElements(pStg, 0, NULL, 0, &pEnum);
    if (FAILED(hr)) return hr;

    while ((hr = IEnumSTATSTG_Next(pEnum, 1, &elem, NULL)) == S_OK)
    {
	if (DataCache_IsPresentationStream(&elem))
	{
	    IStream *pStm;

	    hr = IStorage_OpenStream(This->presentationStorage, elem.pwcsName,
				     NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0,
				     &pStm);
	    if (SUCCEEDED(hr))
	    {
                PresentationDataHeader header;
                ULONG actual_read;
                CLIPFORMAT clipformat;

                hr = read_clipformat(pStm, &clipformat);

                if (hr == S_OK)
                    hr = IStream_Read(pStm, &header, sizeof(header),
                                      &actual_read);

		/* can't use SUCCEEDED(hr): S_FALSE counts as an error */
		if (hr == S_OK && actual_read == sizeof(header))
		{
		    DataCacheEntry *cache_entry;
		    FORMATETC fmtetc;

		    fmtetc.cfFormat = clipformat;
		    fmtetc.ptd = NULL; /* FIXME */
		    fmtetc.dwAspect = header.dvAspect;
		    fmtetc.lindex = header.lindex;
		    fmtetc.tymed = header.tymed;

                    TRACE("loading entry with formatetc: %s\n", debugstr_formatetc(&fmtetc));

                    cache_entry = DataCache_GetEntryForFormatEtc(This, &fmtetc);
                    if (!cache_entry)
                        hr = DataCache_CreateEntry(This, &fmtetc, &cache_entry);
                    if (SUCCEEDED(hr))
                    {
                        DataCacheEntry_DiscardData(cache_entry);
                        if (cache_entry->storage) IStorage_Release(cache_entry->storage);
                        cache_entry->storage = pStg;
                        IStorage_AddRef(pStg);
                        cache_entry->dirty = FALSE;
                    }
		}

		IStream_Release(pStm);
	    }
	}

	CoTaskMemFree(elem.pwcsName);
    }

    This->dirty = FALSE;

    IEnumSTATSTG_Release(pEnum);

    IStorage_AddRef(This->presentationStorage);
    return S_OK;
}

/************************************************************************
 * DataCache_Save (IPersistStorage)
 *
 * Until we actually connect to a running object and retrieve new
 * information to it, we never have to save anything. However, it is
 * our responsibility to copy the information when saving to a new
 * storage.
 *
 * See Windows documentation for more details on IPersistStorage methods.
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
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_SaveCompleted(
            IPersistStorage* iface,
	    IStorage*        pStgNew)
{
  TRACE("(%p, %p)\n", iface, pStgNew);

  if (pStgNew)
  {
  /*
   * First, make sure we get our hands off any storage we have.
   */

  IPersistStorage_HandsOffStorage(iface);

  /*
   * Then, attach to the new storage.
   */

  DataCache_Load(iface, pStgNew);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_HandsOffStorage (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currently holding.
 *
 * See Windows documentation for more details on IPersistStorage methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IViewObject2_AddRef(
            IViewObject2* iface)
{
  DataCache *this = impl_from_IViewObject2(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IViewObject2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IViewObject2 methods.
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

  /*
   * Sanity check
   */
  if (lprcBounds==NULL)
    return E_INVALIDARG;

  LIST_FOR_EACH_ENTRY(cache_entry, &This->cache_list, DataCacheEntry, entry)
  {
    /* FIXME: compare ptd too */
    if ((cache_entry->fmtetc.dwAspect != dwDrawAspect) ||
        (cache_entry->fmtetc.lindex != lindex))
      continue;

    /* if the data hasn't been loaded yet, do it now */
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && cache_entry->storage)
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
 *
 * See Windows documentation for more details on IViewObject2 methods.
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
 *
 * See Windows documentation for more details on IViewObject2 methods.
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
 *
 * See Windows documentation for more details on IViewObject2 methods.
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

  /*
   * Sanity check
   */
  if (lpsizel==NULL)
    return E_POINTER;

  /*
   * Initialize the out parameter.
   */
  lpsizel->cx = 0;
  lpsizel->cy = 0;

  /*
   * This flag should be set to -1.
   */
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
    if ((cache_entry->stgmedium.tymed == TYMED_NULL) && cache_entry->storage)
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
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_AddRef(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IOleCache2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_Release(
            IOleCache2*     iface)
{
  DataCache *this = impl_from_IOleCache2(iface);

  return IUnknown_Release(this->outerUnknown);
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
    TRACE("pformatetc = %s\n", debugstr_formatetc(pformatetc));

    *pdwConnection = 0;

    cache_entry = DataCache_GetEntryForFormatEtc(This, pformatetc);
    if (cache_entry)
    {
        TRACE("found an existing cache entry\n");
        *pdwConnection = cache_entry->id;
        return CACHE_S_SAMECACHE;
    }

    hr = DataCache_CreateEntry(This, pformatetc, &cache_entry);

    if (SUCCEEDED(hr))
        *pdwConnection = cache_entry->id;

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
            DataCacheEntry_Destroy(cache_entry);
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
        hr = DataCache_Save((IPersistStorage *)&This->lpvtblIPersistStorage,
                            This->presentationStorage, TRUE);

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
 *
 * See Windows documentation for more details on IUnknown methods.
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
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_AddRef(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_AddRef(this->outerUnknown);
}

/************************************************************************
 * DataCache_IOleCacheControl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_Release(
            IOleCacheControl* iface)
{
  DataCache *this = impl_from_IOleCacheControl(iface);

  return IUnknown_Release(this->outerUnknown);
}

static HRESULT WINAPI DataCache_OnRun(
	    IOleCacheControl* iface,
	    LPDATAOBJECT      pDataObject)
{
  FIXME("stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_OnStop(
	    IOleCacheControl* iface)
{
  FIXME("stub\n");
  return E_NOTIMPL;
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
 *  IOleCache, IOleCache2, IOleCacheControl, IPersistStorae, IDataObject,
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
  if ( (pUnkOuter!=NULL) &&
       (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) != 0) )
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
  hr = IUnknown_QueryInterface((IUnknown*)&(newCache->lpvtblNDIUnknown), riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IUnknown_Release((IUnknown*)&(newCache->lpvtblNDIUnknown));

  return hr;
}

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
  newObject->lpVtbl = &DataCache_IDataObject_VTable;
  newObject->lpvtblNDIUnknown = &DataCache_NDIUnknown_VTable;
  newObject->lpvtblIPersistStorage = &DataCache_IPersistStorage_VTable;
  newObject->lpvtblIViewObject = &DataCache_IViewObject2_VTable;
  newObject->lpvtblIOleCache2 = &DataCache_IOleCache2_VTable;
  newObject->lpvtblIOleCacheControl = &DataCache_IOleCacheControl_VTable;

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
    pUnkOuter = (IUnknown*)&(newObject->lpvtblNDIUnknown);

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

  return newObject;
}
