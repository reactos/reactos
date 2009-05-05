/*
 *  OLE 2 clipboard support
 *
 *      Copyright 1999  Noel Borthwick <noel@macadamian.com>
 *      Copyright 2000  Abey George <abey@macadamian.com>
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
 *    This file contains the implementation for the OLE Clipboard and its
 *    internal interfaces. The OLE clipboard interacts with an IDataObject
 *    interface via the OleSetClipboard, OleGetClipboard and
 *    OleIsCurrentClipboard API's. An internal IDataObject delegates
 *    to a client supplied IDataObject or the WIN32 clipboard API depending
 *    on whether OleSetClipboard has been invoked.
 *    Here are some operating scenarios:
 *
 *    1. OleSetClipboard called: In this case the internal IDataObject
 *       delegates to the client supplied IDataObject. Additionally OLE takes
 *       ownership of the Windows clipboard and any HGLOCBAL IDataObject
 *       items are placed on the Windows clipboard. This allows non OLE aware
 *       applications to access these. A local WinProc fields WM_RENDERFORMAT
 *       and WM_RENDERALLFORMATS messages in this case.
 *
 *    2. OleGetClipboard called without previous OleSetClipboard. Here the internal
 *       IDataObject functionality wraps around the WIN32 clipboard API.
 *
 *    3. OleGetClipboard called after previous OleSetClipboard. Here the internal
 *       IDataObject delegates to the source IDataObjects functionality directly,
 *       thereby bypassing the Windows clipboard.
 *
 *    Implementation references : Inside OLE 2'nd  edition by Kraig Brockschmidt
 *
 * TODO:
 *    - Support for pasting between different processes. OLE clipboard support
 *      currently works only for in process copy and paste. Since we internally
 *      store a pointer to the source's IDataObject and delegate to that, this
 *      will fail if the IDataObject client belongs to a different process.
 *    - IDataObject::GetDataHere is not implemented
 *    - OleFlushClipboard needs to additionally handle TYMED_IStorage media
 *      by copying the storage into global memory. Subsequently the default
 *      data object exposed through OleGetClipboard must convert this TYMED_HGLOBAL
 *      back to TYMED_IStorage.
 *    - OLE1 compatibility formats to be synthesized from OLE2 formats and put on
 *      clipboard in OleSetClipboard.
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "ole2.h"
#include "wine/debug.h"
#include "olestd.h"

#include "storage32.h"

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define HANDLE_ERROR(err) do { hr = err; TRACE("(HRESULT=%x)\n", (HRESULT)err); goto CLEANUP; } while (0)

/* Structure of 'Ole Private Data' clipboard format */
typedef struct
{
    FORMATETC fmtetc;
    DWORD first_use;  /* Has this cf been added to the list already */
    DWORD unk[2];
} ole_priv_data_entry;

typedef struct
{
    DWORD unk1;
    DWORD size; /* in bytes of the entire structure */
    DWORD unk2;
    DWORD count; /* no. of format entries */
    DWORD unk3[2];
    ole_priv_data_entry entries[1]; /* array of size count */
    /* then follows any DVTARGETDEVICE structures referenced in the FORMATETCs */
} ole_priv_data;

/*****************************************************************************
 *           td_offs_to_ptr
 *
 * Returns a ptr to a target device at a given offset from the
 * start of the ole_priv_data.
 *
 * Used when unpacking ole private data from the clipboard.
 */
static inline DVTARGETDEVICE *td_offs_to_ptr(ole_priv_data *data, DWORD_PTR off)
{
    if(off == 0) return NULL;
    return (DVTARGETDEVICE*)((char*)data + off);
}

/*****************************************************************************
 *           td_get_offs
 *
 * Get the offset from the start of the ole_priv_data of the idx'th
 * target device.
 *
 * Used when packing ole private data to the clipboard.
 */
static inline DWORD_PTR td_get_offs(ole_priv_data *data, DWORD idx)
{
    if(data->entries[idx].fmtetc.ptd == NULL) return 0;
    return (char*)data->entries[idx].fmtetc.ptd - (char*)data;
}

/****************************************************************************
 * Consumer snapshot.  Represents the state of the ole clipboard
 * returned by OleGetClipboard().
 */
typedef struct snapshot
{
    const IDataObjectVtbl* lpVtbl;
    LONG ref;

    DWORD seq_no;                   /* Clipboard sequence number corresponding to this snapshot */

    IDataObject *data;              /* If we unmarshal a remote data object we hold a ref here */
} snapshot;

/****************************************************************************
 * ole_clipbrd
 */
typedef struct ole_clipbrd
{
    snapshot *latest_snapshot;       /* Latest consumer snapshot */

    HWND window;                     /* Hidden clipboard window */
    IDataObject *src_data;           /* Source object passed to OleSetClipboard */
    ole_priv_data *cached_enum;      /* Cached result from the enumeration of src data object */
    IStream *marshal_data;           /* Stream onto which to marshal src_data */
} ole_clipbrd;

static inline snapshot *impl_from_IDataObject(IDataObject *iface)
{
    return (snapshot*)((char*)iface - FIELD_OFFSET(snapshot, lpVtbl));
}

typedef struct PresentationDataHeader
{
  BYTE unknown1[28];
  DWORD dwObjectExtentX;
  DWORD dwObjectExtentY;
  DWORD dwSize;
} PresentationDataHeader;

/*
 * The one and only ole_clipbrd object which is created by OLEClipbrd_Initialize()
 */
static ole_clipbrd* theOleClipboard;

static inline HRESULT get_ole_clipbrd(ole_clipbrd **clipbrd)
{
    struct oletls *info = COM_CurrentInfo();
    *clipbrd = NULL;

    if(!info->ole_inits)
        return CO_E_NOTINITIALIZED;
    *clipbrd = theOleClipboard;

    return S_OK;
}

/*
 * Name of our registered OLE clipboard window class
 */
static const WCHAR clipbrd_wndclass[] = {'C','L','I','P','B','R','D','W','N','D','C','L','A','S','S',0};

static const WCHAR wine_marshal_dataobject[] = {'W','i','n','e',' ','m','a','r','s','h','a','l',' ','d','a','t','a','o','b','j','e','c','t',0};

UINT ownerlink_clipboard_format = 0;
UINT filename_clipboard_format = 0;
UINT filenameW_clipboard_format = 0;
UINT dataobject_clipboard_format = 0;
UINT embedded_object_clipboard_format = 0;
UINT embed_source_clipboard_format = 0;
UINT custom_link_source_clipboard_format = 0;
UINT link_source_clipboard_format = 0;
UINT object_descriptor_clipboard_format = 0;
UINT link_source_descriptor_clipboard_format = 0;
UINT ole_private_data_clipboard_format = 0;

static UINT wine_marshal_clipboard_format;

static inline char *dump_fmtetc(FORMATETC *fmt)
{
    static char buf[100];

    snprintf(buf, sizeof(buf), "cf %04x ptd %p aspect %x lindex %d tymed %x",
             fmt->cfFormat, fmt->ptd, fmt->dwAspect, fmt->lindex, fmt->tymed);
    return buf;
}

/*---------------------------------------------------------------------*
 *  Implementation of the internal IEnumFORMATETC interface returned by
 *  the OLE clipboard's IDataObject.
 *---------------------------------------------------------------------*/

typedef struct enum_fmtetc
{
    const IEnumFORMATETCVtbl *lpVtbl;
    LONG ref;

    UINT pos;    /* current enumerator position */
    ole_priv_data *data;
} enum_fmtetc;

static inline enum_fmtetc *impl_from_IEnumFORMATETC(IEnumFORMATETC *iface)
{
    return (enum_fmtetc*)((char*)iface - FIELD_OFFSET(enum_fmtetc, lpVtbl));
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_QueryInterface
  (LPENUMFORMATETC iface, REFIID riid, LPVOID* ppvObj)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);

  TRACE("(%p)->(IID: %s, %p)\n", This, debugstr_guid(riid), ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown) ||
     IsEqualIID(riid, &IID_IEnumFORMATETC))
  {
    *ppvObj = iface;
  }

  if(*ppvObj)
  {
    IEnumFORMATETC_AddRef((IEnumFORMATETC*)*ppvObj);
    TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }

  TRACE("-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_AddRef (IUnknown)
 *
 */
static ULONG WINAPI OLEClipbrd_IEnumFORMATETC_AddRef(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->(count=%u)\n",This, This->ref);

  return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEClipbrd_IEnumFORMATETC_Release(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  ULONG ref;

  TRACE("(%p)->(count=%u)\n",This, This->ref);

  ref = InterlockedDecrement(&This->ref);
  if (!ref)
  {
    TRACE("() - destroying IEnumFORMATETC(%p)\n",This);
    HeapFree(GetProcessHeap(), 0, This->data);
    HeapFree(GetProcessHeap(), 0, This);
  }
  return ref;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Next (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Next
  (LPENUMFORMATETC iface, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  UINT cfetch, i;
  HRESULT hres = S_FALSE;

  TRACE("(%p)->(pos=%u)\n", This, This->pos);

  if (This->pos < This->data->count)
  {
    cfetch = This->data->count - This->pos;
    if (cfetch >= celt)
    {
      cfetch = celt;
      hres = S_OK;
    }

    for(i = 0; i < cfetch; i++)
    {
      rgelt[i] = This->data->entries[This->pos++].fmtetc;
      if(rgelt[i].ptd)
      {
        DVTARGETDEVICE *target = rgelt[i].ptd;
        rgelt[i].ptd = CoTaskMemAlloc(target->tdSize);
        if(!rgelt[i].ptd) return E_OUTOFMEMORY;
        memcpy(rgelt[i].ptd, target, target->tdSize);
      }
    }
  }
  else
  {
    cfetch = 0;
  }

  if (pceltFethed)
  {
    *pceltFethed = cfetch;
  }

  return hres;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Skip (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Skip(LPENUMFORMATETC iface, ULONG celt)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->(num=%u)\n", This, celt);

  This->pos += celt;
  if (This->pos > This->data->count)
  {
    This->pos = This->data->count;
    return S_FALSE;
  }
  return S_OK;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Reset (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Reset(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->()\n", This);

  This->pos = 0;
  return S_OK;
}

static HRESULT enum_fmtetc_construct(ole_priv_data *data, UINT pos, IEnumFORMATETC **obj);

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Clone (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Clone
  (LPENUMFORMATETC iface, LPENUMFORMATETC* obj)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  ole_priv_data *new_data;
  DWORD i;

  TRACE("(%p)->(%p)\n", This, obj);

  if ( !obj ) return E_INVALIDARG;
  *obj = NULL;

  new_data = HeapAlloc(GetProcessHeap(), 0, This->data->size);
  if(!new_data) return E_OUTOFMEMORY;
  memcpy(new_data, This->data, This->data->size);

  /* Fixup any target device ptrs */
  for(i = 0; i < This->data->count; i++)
      new_data->entries[i].fmtetc.ptd =
          td_offs_to_ptr(new_data, td_get_offs(This->data, i));

  return enum_fmtetc_construct(new_data, This->pos, obj);
}

static const IEnumFORMATETCVtbl efvt =
{
  OLEClipbrd_IEnumFORMATETC_QueryInterface,
  OLEClipbrd_IEnumFORMATETC_AddRef,
  OLEClipbrd_IEnumFORMATETC_Release,
  OLEClipbrd_IEnumFORMATETC_Next,
  OLEClipbrd_IEnumFORMATETC_Skip,
  OLEClipbrd_IEnumFORMATETC_Reset,
  OLEClipbrd_IEnumFORMATETC_Clone
};

/************************************************************************
 * enum_fmtetc_construct
 *
 * Creates an IEnumFORMATETC enumerator from ole_priv_data which it then owns.
 */
static HRESULT enum_fmtetc_construct(ole_priv_data *data, UINT pos, IEnumFORMATETC **obj)
{
  enum_fmtetc* ef;

  *obj = NULL;
  ef = HeapAlloc(GetProcessHeap(), 0, sizeof(*ef));
  if (!ef) return E_OUTOFMEMORY;

  ef->ref = 1;
  ef->lpVtbl = &efvt;
  ef->data = data;
  ef->pos = pos;

  TRACE("(%p)->()\n", ef);
  *obj = (IEnumFORMATETC *)ef;
  return S_OK;
}

/***********************************************************************
 *                    dup_global_mem
 *
 * Helper method to duplicate an HGLOBAL chunk of memory
 */
static HRESULT dup_global_mem( HGLOBAL src, DWORD flags, HGLOBAL *dst )
{
    void *src_ptr, *dst_ptr;
    DWORD size;

    *dst = NULL;
    if ( !src ) return S_FALSE;

    size = GlobalSize(src);

    *dst = GlobalAlloc( flags, size );
    if ( !*dst ) return E_OUTOFMEMORY;

    src_ptr = GlobalLock(src);
    dst_ptr = GlobalLock(*dst);

    memcpy(dst_ptr, src_ptr, size);

    GlobalUnlock(*dst);
    GlobalUnlock(src);

    return S_OK;
}

/************************************************************
 *              render_embed_source_hack
 *
 * This is clearly a hack and has no place in the clipboard code.
 *
 */
static HRESULT render_embed_source_hack(IDataObject *data, LPFORMATETC fmt)
{
    STGMEDIUM std;
    HGLOBAL hStorage = 0;
    HRESULT hr = S_OK;
    ILockBytes *ptrILockBytes;

    memset(&std, 0, sizeof(STGMEDIUM));
    std.tymed = fmt->tymed = TYMED_ISTORAGE;

    hStorage = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, 0);
    if (hStorage == NULL) return E_OUTOFMEMORY;
    hr = CreateILockBytesOnHGlobal(hStorage, FALSE, &ptrILockBytes);
    hr = StgCreateDocfileOnILockBytes(ptrILockBytes, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &std.u.pstg);
    ILockBytes_Release(ptrILockBytes);

    if (FAILED(hr = IDataObject_GetDataHere(theOleClipboard->src_data, fmt, &std)))
    {
        WARN("() : IDataObject_GetDataHere failed to render clipboard data! (%x)\n", hr);
        GlobalFree(hStorage);
        return hr;
    }

    if (1) /* check whether the presentation data is already -not- present */
    {
        FORMATETC fmt2;
        STGMEDIUM std2;
        METAFILEPICT *mfp = 0;

        fmt2.cfFormat = CF_METAFILEPICT;
        fmt2.ptd = 0;
        fmt2.dwAspect = DVASPECT_CONTENT;
        fmt2.lindex = -1;
        fmt2.tymed = TYMED_MFPICT;

        memset(&std2, 0, sizeof(STGMEDIUM));
        std2.tymed = TYMED_MFPICT;

        /* Get the metafile picture out of it */

        if (SUCCEEDED(hr = IDataObject_GetData(theOleClipboard->src_data, &fmt2, &std2)))
        {
            mfp = GlobalLock(std2.u.hGlobal);
        }

        if (mfp)
        {
            OLECHAR name[]={ 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};
            IStream *pStream = 0;
            void *mfBits;
            PresentationDataHeader pdh;
            INT nSize;
            CLSID clsID;
            LPOLESTR strProgID;
            CHAR strOleTypeName[51];
            BYTE OlePresStreamHeader [] =
            {
                0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
                0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00
            };

            nSize = GetMetaFileBitsEx(mfp->hMF, 0, NULL);

            memset(&pdh, 0, sizeof(PresentationDataHeader));
            memcpy(&pdh, OlePresStreamHeader, sizeof(OlePresStreamHeader));

            pdh.dwObjectExtentX = mfp->xExt;
            pdh.dwObjectExtentY = mfp->yExt;
            pdh.dwSize = nSize;

            hr = IStorage_CreateStream(std.u.pstg, name, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &pStream);

            hr = IStream_Write(pStream, &pdh, sizeof(PresentationDataHeader), NULL);

            mfBits = HeapAlloc(GetProcessHeap(), 0, nSize);
            nSize = GetMetaFileBitsEx(mfp->hMF, nSize, mfBits);

            hr = IStream_Write(pStream, mfBits, nSize, NULL);

            IStream_Release(pStream);

            HeapFree(GetProcessHeap(), 0, mfBits);

            GlobalUnlock(std2.u.hGlobal);
            ReleaseStgMedium(&std2);

            ReadClassStg(std.u.pstg, &clsID);
            ProgIDFromCLSID(&clsID, &strProgID);

            WideCharToMultiByte( CP_ACP, 0, strProgID, -1, strOleTypeName, sizeof(strOleTypeName), NULL, NULL );
            OLECONVERT_CreateOleStream(std.u.pstg);
            OLECONVERT_CreateCompObjStream(std.u.pstg, strOleTypeName);
        }
    }

    if ( !SetClipboardData( fmt->cfFormat, hStorage ) )
    {
        WARN("() : Failed to set rendered clipboard data into clipboard!\n");
        GlobalFree(hStorage);
        hr = CLIPBRD_E_CANT_SET;
    }

    ReleaseStgMedium(&std);
    return hr;
}

/************************************************************************
 *           find_format_in_list
 *
 * Returns the first entry that matches the provided clipboard format.
 */
static inline ole_priv_data_entry *find_format_in_list(ole_priv_data_entry *entries, DWORD num, UINT cf)
{
    DWORD i;
    for(i = 0; i < num; i++)
        if(entries[i].fmtetc.cfFormat == cf)
            return &entries[i];

    return NULL;
}

/***************************************************************************
 *         get_data_from_storage
 *
 * Returns storage data in an HGLOBAL.
 */
static HRESULT get_data_from_storage(IDataObject *data, FORMATETC *fmt, HGLOBAL *mem)
{
    HGLOBAL h;
    IStorage *stg;
    HRESULT hr;
    FORMATETC stg_fmt;
    STGMEDIUM med;
    ILockBytes *lbs;

    *mem = NULL;

    h = GlobalAlloc( GMEM_DDESHARE|GMEM_MOVEABLE, 0 );
    if(!h) return E_OUTOFMEMORY;

    hr = CreateILockBytesOnHGlobal(h, FALSE, &lbs);
    if(SUCCEEDED(hr))
    {
        hr = StgCreateDocfileOnILockBytes(lbs, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stg);
        ILockBytes_Release(lbs);
    }
    if(FAILED(hr))
    {
        GlobalFree(h);
        return hr;
    }

    stg_fmt = *fmt;
    med.tymed = stg_fmt.tymed = TYMED_ISTORAGE;
    med.u.pstg = stg;
    med.pUnkForRelease = NULL;

    hr = IDataObject_GetDataHere(data, &stg_fmt, &med);
    if(FAILED(hr))
    {
        med.u.pstg = NULL;
        hr = IDataObject_GetData(data, &stg_fmt, &med);
        if(FAILED(hr)) goto end;

        hr = IStorage_CopyTo(med.u.pstg, 0, NULL, NULL, stg);
        ReleaseStgMedium(&med);
        if(FAILED(hr)) goto end;
    }
    *mem = h;

end:
    IStorage_Release(stg);
    if(FAILED(hr)) GlobalFree(h);
    return hr;
}

/***************************************************************************
 *         get_data_from_stream
 *
 * Returns stream data in an HGLOBAL.
 */
static HRESULT get_data_from_stream(IDataObject *data, FORMATETC *fmt, HGLOBAL *mem)
{
    HGLOBAL h;
    IStream *stm = NULL;
    HRESULT hr;
    FORMATETC stm_fmt;
    STGMEDIUM med;

    *mem = NULL;

    h = GlobalAlloc( GMEM_DDESHARE|GMEM_MOVEABLE, 0 );
    if(!h) return E_OUTOFMEMORY;

    hr = CreateStreamOnHGlobal(h, FALSE, &stm);
    if(FAILED(hr)) goto error;

    stm_fmt = *fmt;
    med.tymed = stm_fmt.tymed = TYMED_ISTREAM;
    med.u.pstm = stm;
    med.pUnkForRelease = NULL;

    hr = IDataObject_GetDataHere(data, &stm_fmt, &med);
    if(FAILED(hr))
    {
        LARGE_INTEGER offs;
        ULARGE_INTEGER pos;

        med.u.pstm = NULL;
        hr = IDataObject_GetData(data, &stm_fmt, &med);
        if(FAILED(hr)) goto error;

        offs.QuadPart = 0;
        IStream_Seek(med.u.pstm, offs, STREAM_SEEK_CUR, &pos);
        IStream_Seek(med.u.pstm, offs, STREAM_SEEK_SET, NULL);
        hr = IStream_CopyTo(med.u.pstm, stm, pos, NULL, NULL);
        ReleaseStgMedium(&med);
        if(FAILED(hr)) goto error;
    }
    *mem = h;
    IStream_Release(stm);
    return S_OK;

error:
    if(stm) IStream_Release(stm);
    GlobalFree(h);
    return hr;
}

/***************************************************************************
 *         get_data_from_global
 *
 * Returns global data in an HGLOBAL.
 */
static HRESULT get_data_from_global(IDataObject *data, FORMATETC *fmt, HGLOBAL *mem)
{
    HGLOBAL h;
    HRESULT hr;
    FORMATETC mem_fmt;
    STGMEDIUM med;

    *mem = NULL;

    mem_fmt = *fmt;
    mem_fmt.tymed = TYMED_HGLOBAL;

    hr = IDataObject_GetData(data, &mem_fmt, &med);
    if(FAILED(hr)) return hr;

    hr = dup_global_mem(med.u.hGlobal, GMEM_DDESHARE|GMEM_MOVEABLE, &h);

    if(SUCCEEDED(hr)) *mem = h;

    ReleaseStgMedium(&med);

    return hr;
}

/***********************************************************************
 *                render_format
 *
 * Render the clipboard data. Note that this call will delegate to the
 * source data object.
 */
static HRESULT render_format(IDataObject *data, LPFORMATETC fmt)
{
    HGLOBAL clip_data = NULL;
    HRESULT hr;

    /* Embed source hack */
    if(fmt->cfFormat == embed_source_clipboard_format)
    {
        return render_embed_source_hack(data, fmt);
    }

    if(fmt->tymed & TYMED_ISTORAGE)
    {
        hr = get_data_from_storage(data, fmt, &clip_data);
    }
    else if(fmt->tymed & TYMED_ISTREAM)
    {
        hr = get_data_from_stream(data, fmt, &clip_data);
    }
    else if(fmt->tymed & TYMED_HGLOBAL)
    {
        hr = get_data_from_global(data, fmt, &clip_data);
    }
    else
    {
        FIXME("Unhandled tymed %x\n", fmt->tymed);
        hr = DV_E_FORMATETC;
    }

    if(SUCCEEDED(hr))
    {
        if ( !SetClipboardData(fmt->cfFormat, clip_data) )
        {
            WARN("() : Failed to set rendered clipboard data into clipboard!\n");
            GlobalFree(clip_data);
            hr = CLIPBRD_E_CANT_SET;
        }
    }

    return hr;
}

/*---------------------------------------------------------------------*
 *  Implementation of the internal IDataObject interface exposed by
 *  the OLE clipboard.
 *---------------------------------------------------------------------*/


/************************************************************************
 *           snapshot_QueryInterface
 */
static HRESULT WINAPI snapshot_QueryInterface(IDataObject *iface,
                                              REFIID riid, void **ppvObject)
{
  snapshot *This = impl_from_IDataObject(iface);
  TRACE("(%p)->(IID:%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IDataObject, riid))
  {
    *ppvObject = iface;
  }
  else
  {
    WARN( "() : asking for unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;
}

/************************************************************************
 *              snapshot_AddRef
 */
static ULONG WINAPI snapshot_AddRef(IDataObject *iface)
{
    snapshot *This = impl_from_IDataObject(iface);

    TRACE("(%p)->(count=%u)\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

/************************************************************************
 *      snapshot_Release
 */
static ULONG WINAPI snapshot_Release(IDataObject *iface)
{
    snapshot *This = impl_from_IDataObject(iface);
    ULONG ref;

    TRACE("(%p)->(count=%u)\n", This, This->ref);

    ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {
        ole_clipbrd *clipbrd;
        HRESULT hr = get_ole_clipbrd(&clipbrd);

        if(This->data) IDataObject_Release(This->data);

        if(SUCCEEDED(hr) && clipbrd->latest_snapshot == This)
            clipbrd->latest_snapshot = NULL;
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/************************************************************
 *              get_current_ole_clip_window
 *
 * Return the window that owns the ole clipboard.
 *
 * If the clipboard is flushed or not owned by ole this will
 * return NULL.
 */
static HWND get_current_ole_clip_window(void)
{
    HGLOBAL h;
    HWND *ptr, wnd;

    h = GetClipboardData(dataobject_clipboard_format);
    if(!h) return NULL;
    ptr = GlobalLock(h);
    if(!ptr) return NULL;
    wnd = *ptr;
    GlobalUnlock(h);
    return wnd;
}

/************************************************************
 *              get_current_dataobject
 *
 * Return an unmarshalled IDataObject if there is a current
 * (ie non-flushed) object on the ole clipboard.
 */
static HRESULT get_current_dataobject(IDataObject **data)
{
    HRESULT hr = S_FALSE;
    HWND wnd = get_current_ole_clip_window();
    HGLOBAL h;
    void *ptr;
    IStream *stm;
    LARGE_INTEGER pos;

    *data = NULL;
    if(!wnd) return S_FALSE;

    h = GetClipboardData(wine_marshal_clipboard_format);
    if(!h) return S_FALSE;
    if(GlobalSize(h) == 0) return S_FALSE;
    ptr = GlobalLock(h);
    if(!ptr) return S_FALSE;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stm);
    if(FAILED(hr)) goto end;

    hr = IStream_Write(stm, ptr, GlobalSize(h), NULL);
    if(SUCCEEDED(hr))
    {
        pos.QuadPart = 0;
        IStream_Seek(stm, pos, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(stm, &IID_IDataObject, (void**)data);
    }
    IStream_Release(stm);

end:
    GlobalUnlock(h);
    return hr;
}

static DWORD get_tymed_from_nonole_cf(UINT cf)
{
    if(cf >= 0xc000) return TYMED_ISTREAM | TYMED_HGLOBAL;

    switch(cf)
    {
    case CF_TEXT:
    case CF_OEMTEXT:
    case CF_UNICODETEXT:
        return TYMED_ISTREAM | TYMED_HGLOBAL;
    case CF_ENHMETAFILE:
        return TYMED_ENHMF;
    case CF_METAFILEPICT:
        return TYMED_MFPICT;
    default:
        FIXME("returning TYMED_NULL for cf %04x\n", cf);
        return TYMED_NULL;
    }
}

/***********************************************************
 *     get_priv_data
 *
 * Returns a copy of the Ole Private Data
 */
static HRESULT get_priv_data(ole_priv_data **data)
{
    HGLOBAL handle;
    HRESULT hr = S_OK;
    ole_priv_data *ret = NULL;

    *data = NULL;

    handle = GetClipboardData( ole_private_data_clipboard_format );
    if(handle)
    {
        ole_priv_data *src = GlobalLock(handle);
        if(src)
        {
            DWORD i;

            /* FIXME: sanity check on size */
            ret = HeapAlloc(GetProcessHeap(), 0, src->size);
            if(!ret)
            {
                GlobalUnlock(handle);
                return E_OUTOFMEMORY;
            }
            memcpy(ret, src, src->size);
            GlobalUnlock(handle);

            /* Fixup any target device offsets to ptrs */
            for(i = 0; i < ret->count; i++)
                ret->entries[i].fmtetc.ptd =
                    td_offs_to_ptr(ret, (DWORD_PTR) ret->entries[i].fmtetc.ptd);
        }
    }

    if(!ret) /* Non-ole data */
    {
        UINT cf;
        DWORD count = 0, idx, size = FIELD_OFFSET(ole_priv_data, entries);

        for(cf = 0; (cf = EnumClipboardFormats(cf)) != 0; count++)
        {
            char buf[100];
            GetClipboardFormatNameA(cf, buf, sizeof(buf));
            TRACE("\tcf %04x %s\n", cf, buf);
            ;
        }
        TRACE("count %d\n", count);
        size += count * sizeof(ret->entries[0]);

        /* There are holes in fmtetc so zero init */
        ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if(!ret) return E_OUTOFMEMORY;
        ret->size = size;
        ret->count = count;

        for(cf = 0, idx = 0; (cf = EnumClipboardFormats(cf)) != 0; idx++)
        {
            ret->entries[idx].fmtetc.cfFormat = cf;
            ret->entries[idx].fmtetc.ptd = NULL;
            ret->entries[idx].fmtetc.dwAspect = DVASPECT_CONTENT;
            ret->entries[idx].fmtetc.lindex = -1;
            ret->entries[idx].fmtetc.tymed = get_tymed_from_nonole_cf(cf);
            ret->entries[idx].first_use = 1;
        }
    }

    *data = ret;
    return hr;
}

/************************************************************************
 *                    get_stgmed_for_global
 *
 * Returns a stg medium with a copy of the global handle
 */
static HRESULT get_stgmed_for_global(HGLOBAL h, STGMEDIUM *med)
{
    HRESULT hr;

    med->pUnkForRelease = NULL;
    med->tymed = TYMED_NULL;

    hr = dup_global_mem(h, GMEM_MOVEABLE, &med->u.hGlobal);

    if(SUCCEEDED(hr)) med->tymed = TYMED_HGLOBAL;

    return hr;
}

/************************************************************************
 *                    get_stgmed_for_stream
 *
 * Returns a stg medium with a stream based on the handle
 */
static HRESULT get_stgmed_for_stream(HGLOBAL h, STGMEDIUM *med)
{
    HRESULT hr;
    HGLOBAL dst;

    med->pUnkForRelease = NULL;
    med->tymed = TYMED_NULL;

    hr = dup_global_mem(h, GMEM_MOVEABLE, &dst);
    if(FAILED(hr)) return hr;

    hr = CreateStreamOnHGlobal(dst, TRUE, &med->u.pstm);
    if(FAILED(hr))
    {
        GlobalFree(dst);
        return hr;
    }

    med->tymed = TYMED_ISTREAM;
    return hr;
}

/************************************************************************
 *                    get_stgmed_for_storage
 *
 * Returns a stg medium with a storage based on the handle
 */
static HRESULT get_stgmed_for_storage(HGLOBAL h, STGMEDIUM *med)
{
    HRESULT hr;
    HGLOBAL dst;
    ILockBytes *lbs;

    med->pUnkForRelease = NULL;
    med->tymed = TYMED_NULL;

    hr = dup_global_mem(h, GMEM_MOVEABLE, &dst);
    if(FAILED(hr)) return hr;

    hr = CreateILockBytesOnHGlobal(dst, TRUE, &lbs);
    if(FAILED(hr))
    {
        GlobalFree(dst);
        return hr;
    }

    hr = StgOpenStorageOnILockBytes(lbs, NULL,  STGM_SHARE_EXCLUSIVE | STGM_READWRITE, NULL, 0, &med->u.pstg);
    ILockBytes_Release(lbs);
    if(FAILED(hr))
    {
        GlobalFree(dst);
        return hr;
    }

    med->tymed = TYMED_ISTORAGE;
    return hr;
}

static inline BOOL string_off_equal(const DVTARGETDEVICE *t1, WORD off1, const DVTARGETDEVICE *t2, WORD off2)
{
    const WCHAR *str1, *str2;

    if(off1 == 0 && off2 == 0) return TRUE;
    if(off1 == 0 || off2 == 0) return FALSE;

    str1 = (const WCHAR*)((const char*)t1 + off1);
    str2 = (const WCHAR*)((const char*)t2 + off2);

    return !lstrcmpW(str1, str2);
}

static inline BOOL td_equal(const DVTARGETDEVICE *t1, const DVTARGETDEVICE *t2)
{
    if(t1 == NULL && t2 == NULL) return TRUE;
    if(t1 == NULL || t2 == NULL) return FALSE;

    if(!string_off_equal(t1, t1->tdDriverNameOffset, t2, t2->tdDriverNameOffset))
        return FALSE;
    if(!string_off_equal(t1, t1->tdDeviceNameOffset, t2, t2->tdDeviceNameOffset))
        return FALSE;
    if(!string_off_equal(t1, t1->tdPortNameOffset, t2, t2->tdPortNameOffset))
        return FALSE;

    /* FIXME check devmode? */

    return TRUE;
}

/************************************************************************
 *         snapshot_GetData
 */
static HRESULT WINAPI snapshot_GetData(IDataObject *iface, FORMATETC *fmt,
                                       STGMEDIUM *med)
{
    snapshot *This = impl_from_IDataObject(iface);
    HANDLE h;
    HRESULT hr;
    ole_priv_data *enum_data = NULL;
    ole_priv_data_entry *entry;
    DWORD mask;

    TRACE("(%p, %p {%s}, %p)\n", iface, fmt, dump_fmtetc(fmt), med);

    if ( !fmt || !med ) return E_INVALIDARG;

    if ( !OpenClipboard(NULL)) return CLIPBRD_E_CANT_OPEN;

    if(!This->data)
        hr = get_current_dataobject(&This->data);

    if(This->data)
    {
        hr = IDataObject_GetData(This->data, fmt, med);
        CloseClipboard();
        return hr;
    }

    h = GetClipboardData(fmt->cfFormat);
    if(!h)
    {
        hr = DV_E_FORMATETC;
        goto end;
    }

    hr = get_priv_data(&enum_data);
    if(FAILED(hr)) goto end;

    entry = find_format_in_list(enum_data->entries, enum_data->count, fmt->cfFormat);
    if(entry)
    {
        if(!td_equal(fmt->ptd, entry->fmtetc.ptd))
        {
            hr = DV_E_FORMATETC;
            goto end;
        }
        mask = fmt->tymed & entry->fmtetc.tymed;
        if(!mask) mask = fmt->tymed & (TYMED_ISTREAM | TYMED_HGLOBAL);
    }
    else /* non-Ole format */
        mask = fmt->tymed & TYMED_HGLOBAL;

    if(mask & TYMED_ISTORAGE)
        hr = get_stgmed_for_storage(h, med);
    else if(mask & TYMED_HGLOBAL)
        hr = get_stgmed_for_global(h, med);
    else if(mask & TYMED_ISTREAM)
        hr = get_stgmed_for_stream(h, med);
    else
    {
        FIXME("Unhandled tymed - emum tymed %x req tymed %x\n", entry->fmtetc.tymed, fmt->tymed);
        hr = E_FAIL;
        goto end;
    }

end:
    HeapFree(GetProcessHeap(), 0, enum_data);
    if ( !CloseClipboard() ) hr = CLIPBRD_E_CANT_CLOSE;
    return hr;
}

/************************************************************************
 *          snapshot_GetDataHere
 */
static HRESULT WINAPI snapshot_GetDataHere(IDataObject *iface, FORMATETC *fmt,
                                           STGMEDIUM *med)
{
    FIXME("(%p, %p {%s}, %p): stub\n", iface, fmt, dump_fmtetc(fmt), med);
    return E_NOTIMPL;
}

/************************************************************************
 *           snapshot_QueryGetData
 *
 * The OLE Clipboard's implementation of this method delegates to
 * a data source if there is one or wraps around the windows clipboard
 * function IsClipboardFormatAvailable() otherwise.
 *
 */
static HRESULT WINAPI snapshot_QueryGetData(IDataObject *iface, FORMATETC *fmt)
{
    FIXME("(%p, %p {%s})\n", iface, fmt, dump_fmtetc(fmt));

    if (!fmt) return E_INVALIDARG;

    if ( fmt->dwAspect != DVASPECT_CONTENT ) return DV_E_FORMATETC;

    if ( fmt->lindex != -1 ) return DV_E_FORMATETC;

    return (IsClipboardFormatAvailable(fmt->cfFormat)) ? S_OK : DV_E_CLIPFORMAT;
}

/************************************************************************
 *              snapshot_GetCanonicalFormatEtc
 */
static HRESULT WINAPI snapshot_GetCanonicalFormatEtc(IDataObject *iface, FORMATETC *fmt_in,
                                                     FORMATETC *fmt_out)
{
    TRACE("(%p, %p, %p)\n", iface, fmt_in, fmt_out);

    if ( !fmt_in || !fmt_out ) return E_INVALIDARG;

    *fmt_out = *fmt_in;
    return DATA_S_SAMEFORMATETC;
}

/************************************************************************
 *              snapshot_SetData
 *
 * The OLE Clipboard does not implement this method
 */
static HRESULT WINAPI snapshot_SetData(IDataObject *iface, FORMATETC *fmt,
                                       STGMEDIUM *med, BOOL release)
{
    TRACE("(%p, %p, %p, %d): not implemented\n", iface, fmt, med, release);
    return E_NOTIMPL;
}

/************************************************************************
 *             snapshot_EnumFormatEtc
 *
 */
static HRESULT WINAPI snapshot_EnumFormatEtc(IDataObject *iface, DWORD dir,
                                             IEnumFORMATETC **enum_fmt)
{
    HRESULT hr;
    ole_priv_data *data = NULL;

    TRACE("(%p, %x, %p)\n", iface, dir, enum_fmt);

    *enum_fmt = NULL;

    if ( dir != DATADIR_GET ) return E_NOTIMPL;
    if ( !OpenClipboard(NULL) ) return CLIPBRD_E_CANT_OPEN;

    hr = get_priv_data(&data);

    if(FAILED(hr)) goto end;

    hr = enum_fmtetc_construct( data, 0, enum_fmt );

end:
    if ( !CloseClipboard() ) hr = CLIPBRD_E_CANT_CLOSE;
    return hr;
}

/************************************************************************
 *               snapshot_DAdvise
 *
 * The OLE Clipboard does not implement this method
 */
static HRESULT WINAPI snapshot_DAdvise(IDataObject *iface, FORMATETC *fmt,
                                       DWORD flags, IAdviseSink *sink,
                                       DWORD *conn)
{
    TRACE("(%p, %p, %x, %p, %p): not implemented\n", iface, fmt, flags, sink, conn);
    return E_NOTIMPL;
}

/************************************************************************
 *              snapshot_DUnadvise
 *
 * The OLE Clipboard does not implement this method
 */
static HRESULT WINAPI snapshot_DUnadvise(IDataObject* iface, DWORD conn)
{
    TRACE("(%p, %d): not implemented\n", iface, conn);
    return E_NOTIMPL;
}

/************************************************************************
 *             snapshot_EnumDAdvise
 *
 * The OLE Clipboard does not implement this method
 */
static HRESULT WINAPI snapshot_EnumDAdvise(IDataObject* iface,
                                           IEnumSTATDATA** enum_advise)
{
    TRACE("(%p, %p): not implemented\n", iface, enum_advise);
    return E_NOTIMPL;
}

static const IDataObjectVtbl snapshot_vtable =
{
    snapshot_QueryInterface,
    snapshot_AddRef,
    snapshot_Release,
    snapshot_GetData,
    snapshot_GetDataHere,
    snapshot_QueryGetData,
    snapshot_GetCanonicalFormatEtc,
    snapshot_SetData,
    snapshot_EnumFormatEtc,
    snapshot_DAdvise,
    snapshot_DUnadvise,
    snapshot_EnumDAdvise
};

/*---------------------------------------------------------------------*
 *           Internal implementation methods for the OLE clipboard
 *---------------------------------------------------------------------*/

static snapshot *snapshot_construct(DWORD seq_no)
{
    snapshot *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof(*This) );
    if (!This) return NULL;

    This->lpVtbl = &snapshot_vtable;
    This->ref = 0;
    This->seq_no = seq_no;
    This->data = NULL;

    return This;
}

/*********************************************************
 *               register_clipboard_formats
 */
static void register_clipboard_formats(void)
{
    static const WCHAR OwnerLink[] = {'O','w','n','e','r','L','i','n','k',0};
    static const WCHAR FileName[] = {'F','i','l','e','N','a','m','e',0};
    static const WCHAR FileNameW[] = {'F','i','l','e','N','a','m','e','W',0};
    static const WCHAR DataObject[] = {'D','a','t','a','O','b','j','e','c','t',0};
    static const WCHAR EmbeddedObject[] = {'E','m','b','e','d','d','e','d',' ','O','b','j','e','c','t',0};
    static const WCHAR EmbedSource[] = {'E','m','b','e','d',' ','S','o','u','r','c','e',0};
    static const WCHAR CustomLinkSource[] = {'C','u','s','t','o','m',' ','L','i','n','k',' ','S','o','u','r','c','e',0};
    static const WCHAR LinkSource[] = {'L','i','n','k',' ','S','o','u','r','c','e',0};
    static const WCHAR ObjectDescriptor[] = {'O','b','j','e','c','t',' ','D','e','s','c','r','i','p','t','o','r',0};
    static const WCHAR LinkSourceDescriptor[] = {'L','i','n','k',' ','S','o','u','r','c','e',' ',
                                                 'D','e','s','c','r','i','p','t','o','r',0};
    static const WCHAR OlePrivateData[] = {'O','l','e',' ','P','r','i','v','a','t','e',' ','D','a','t','a',0};

    static const WCHAR WineMarshalledDataObject[] = {'W','i','n','e',' ','M','a','r','s','h','a','l','l','e','d',' ',
                                                     'D','a','t','a','O','b','j','e','c','t',0};

    ownerlink_clipboard_format = RegisterClipboardFormatW(OwnerLink);
    filename_clipboard_format = RegisterClipboardFormatW(FileName);
    filenameW_clipboard_format = RegisterClipboardFormatW(FileNameW);
    dataobject_clipboard_format = RegisterClipboardFormatW(DataObject);
    embedded_object_clipboard_format = RegisterClipboardFormatW(EmbeddedObject);
    embed_source_clipboard_format = RegisterClipboardFormatW(EmbedSource);
    custom_link_source_clipboard_format = RegisterClipboardFormatW(CustomLinkSource);
    link_source_clipboard_format = RegisterClipboardFormatW(LinkSource);
    object_descriptor_clipboard_format = RegisterClipboardFormatW(ObjectDescriptor);
    link_source_descriptor_clipboard_format = RegisterClipboardFormatW(LinkSourceDescriptor);
    ole_private_data_clipboard_format = RegisterClipboardFormatW(OlePrivateData);

    wine_marshal_clipboard_format = RegisterClipboardFormatW(WineMarshalledDataObject);
}

/***********************************************************************
 * OLEClipbrd_Initialize()
 * Initializes the OLE clipboard.
 */
void OLEClipbrd_Initialize(void)
{
    register_clipboard_formats();

    if ( !theOleClipboard )
    {
        ole_clipbrd* clipbrd;
        HGLOBAL h;

        TRACE("()\n");

        clipbrd = HeapAlloc( GetProcessHeap(), 0, sizeof(*clipbrd) );
        if (!clipbrd) return;

        clipbrd->latest_snapshot = NULL;
        clipbrd->window = NULL;
        clipbrd->src_data = NULL;
        clipbrd->cached_enum = NULL;

        h = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, 0);
        if(!h)
        {
            HeapFree(GetProcessHeap(), 0, clipbrd);
            return;
        }

        if(FAILED(CreateStreamOnHGlobal(h, TRUE, &clipbrd->marshal_data)))
        {
            GlobalFree(h);
            HeapFree(GetProcessHeap(), 0, clipbrd);
            return;
        }

        theOleClipboard = clipbrd;
    }
}

/***********************************************************************
 * OLEClipbrd_UnInitialize()
 * Un-Initializes the OLE clipboard
 */
void OLEClipbrd_UnInitialize(void)
{
    ole_clipbrd *clipbrd = theOleClipboard;

    TRACE("()\n");

    if ( clipbrd )
    {
        static const WCHAR ole32W[] = {'o','l','e','3','2',0};
        HINSTANCE hinst = GetModuleHandleW(ole32W);

        if ( clipbrd->window )
        {
            DestroyWindow(clipbrd->window);
            UnregisterClassW( clipbrd_wndclass, hinst );
        }

        IStream_Release(clipbrd->marshal_data);
        HeapFree(GetProcessHeap(), 0, clipbrd);
        theOleClipboard = NULL;
    }
}

/*********************************************************************
 *          set_clipboard_formats
 *
 * Enumerate all formats supported by the source and make
 * those formats available using delayed rendering using SetClipboardData.
 * Cache the enumeration list and make that list visibile as the
 * 'Ole Private Data' format on the clipboard.
 *
 */
static HRESULT set_clipboard_formats(ole_clipbrd *clipbrd, IDataObject *data)
{
    HRESULT hr;
    FORMATETC fmt;
    IEnumFORMATETC *enum_fmt;
    HGLOBAL priv_data_handle;
    DWORD_PTR target_offset;
    ole_priv_data *priv_data;
    DWORD count = 0, needed = sizeof(*priv_data), idx;

    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);
    if(FAILED(hr)) return hr;

    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
    {
        count++;
        needed += sizeof(priv_data->entries[0]);
        if(fmt.ptd)
        {
            needed += fmt.ptd->tdSize;
            CoTaskMemFree(fmt.ptd);
        }
    }

    /* Windows pads the list with two empty ole_priv_data_entries, one
     * after the entries array and one after the target device data.
     * Allocating with zero init to zero these pads. */

    needed += sizeof(priv_data->entries[0]); /* initialisation of needed includes one of these. */
    priv_data_handle = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, needed);
    priv_data = GlobalLock(priv_data_handle);

    priv_data->unk1 = 0;
    priv_data->size = needed;
    priv_data->unk2 = 1;
    priv_data->count = count;
    priv_data->unk3[0] = 0;
    priv_data->unk3[1] = 0;

    IEnumFORMATETC_Reset(enum_fmt);

    idx = 0;
    target_offset = FIELD_OFFSET(ole_priv_data, entries[count + 1]); /* count entries + one pad. */

    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
    {
        TRACE("%s\n", dump_fmtetc(&fmt));

        priv_data->entries[idx].fmtetc = fmt;
        if(fmt.ptd)
        {
            memcpy((char*)priv_data + target_offset, fmt.ptd, fmt.ptd->tdSize);
            priv_data->entries[idx].fmtetc.ptd = (DVTARGETDEVICE*)target_offset;
            target_offset += fmt.ptd->tdSize;
            CoTaskMemFree(fmt.ptd);
        }

        priv_data->entries[idx].first_use = !find_format_in_list(priv_data->entries, idx, fmt.cfFormat);
        priv_data->entries[idx].unk[0] = 0;
        priv_data->entries[idx].unk[1] = 0;

        if (priv_data->entries[idx].first_use)
            SetClipboardData(fmt.cfFormat, NULL);

        idx++;
    }

    IEnumFORMATETC_Release(enum_fmt);

    /* Cache the list and fixup any target device offsets to ptrs */
    clipbrd->cached_enum = HeapAlloc(GetProcessHeap(), 0, needed);
    memcpy(clipbrd->cached_enum, priv_data, needed);
    for(idx = 0; idx < clipbrd->cached_enum->count; idx++)
        clipbrd->cached_enum->entries[idx].fmtetc.ptd =
            td_offs_to_ptr(clipbrd->cached_enum, (DWORD_PTR)clipbrd->cached_enum->entries[idx].fmtetc.ptd);

    GlobalUnlock(priv_data_handle);
    SetClipboardData(ole_private_data_clipboard_format, priv_data_handle);

    return S_OK;
}

static HWND create_clipbrd_window(void);

/***********************************************************************
 *                 get_clipbrd_window
 */
static inline HRESULT get_clipbrd_window(ole_clipbrd *clipbrd, HWND *wnd)
{
    if ( !clipbrd->window )
        clipbrd->window = create_clipbrd_window();

    *wnd = clipbrd->window;
    return *wnd ? S_OK : E_FAIL;
}


/**********************************************************************
 *                  release_marshal_data
 *
 * Releases the data and sets the stream back to zero size.
 */
static inline void release_marshal_data(IStream *stm)
{
    LARGE_INTEGER pos;
    ULARGE_INTEGER size;
    pos.QuadPart = size.QuadPart = 0;

    IStream_Seek(stm, pos, STREAM_SEEK_SET, NULL);
    CoReleaseMarshalData(stm);
    IStream_Seek(stm, pos, STREAM_SEEK_SET, NULL);
    IStream_SetSize(stm, size);
}

/***********************************************************************
 *   expose_marshalled_dataobject
 *
 * Sets the marshalled dataobject to the clipboard.  In the flushed case
 * we set a zero sized HGLOBAL to clear the old marshalled data.
 */
static HRESULT expose_marshalled_dataobject(ole_clipbrd *clipbrd, IDataObject *data)
{
    HGLOBAL h;

    if(data)
    {
        HGLOBAL h_stm;
        GetHGlobalFromStream(clipbrd->marshal_data, &h_stm);
        dup_global_mem(h_stm, GMEM_DDESHARE|GMEM_MOVEABLE, &h);
    }
    else /* flushed */
        h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 0);

    if(!h) return E_OUTOFMEMORY;

    SetClipboardData(wine_marshal_clipboard_format, h);
    return S_OK;
}

/***********************************************************************
 *                   set_src_dataobject
 *
 * Clears and sets the clipboard's src IDataObject.
 *
 * To marshal the source dataobject we do something rather different from Windows.
 * We set a clipboard format which contains the marshalled data.
 * Windows sets two window props one of which is an IID, the other is an endpoint number.
 */
static HRESULT set_src_dataobject(ole_clipbrd *clipbrd, IDataObject *data)
{
    HRESULT hr;
    HWND wnd;

    if(FAILED(hr = get_clipbrd_window(clipbrd, &wnd))) return hr;

    if(clipbrd->src_data)
    {
        release_marshal_data(clipbrd->marshal_data);

        IDataObject_Release(clipbrd->src_data);
        clipbrd->src_data = NULL;
        HeapFree(GetProcessHeap(), 0, clipbrd->cached_enum);
        clipbrd->cached_enum = NULL;
    }

    if(data)
    {
        IUnknown *unk;

        IDataObject_AddRef(data);
        clipbrd->src_data = data;

        IDataObject_QueryInterface(data, &IID_IUnknown, (void**)&unk);
        hr = CoMarshalInterface(clipbrd->marshal_data, &IID_IDataObject, unk,
                                MSHCTX_LOCAL, NULL, MSHLFLAGS_TABLESTRONG);
        IUnknown_Release(unk); /* Don't hold a ref on IUnknown, we have one on IDataObject. */
        if(FAILED(hr)) return hr;
        hr = set_clipboard_formats(clipbrd, data);
    }
    return hr;
}

/***********************************************************************
 *                   clipbrd_wndproc
 */
static LRESULT CALLBACK clipbrd_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ole_clipbrd *clipbrd;

    get_ole_clipbrd(&clipbrd);

    switch (message)
    {
    case WM_RENDERFORMAT:
    {
        UINT cf = wparam;
        ole_priv_data_entry *entry;

        TRACE("(): WM_RENDERFORMAT(cfFormat=%x)\n", cf);
        entry = find_format_in_list(clipbrd->cached_enum->entries, clipbrd->cached_enum->count, cf);

        if(entry)
            render_format(clipbrd->src_data, &entry->fmtetc);

        break;
    }

    case WM_RENDERALLFORMATS:
    {
        DWORD i;
        ole_priv_data_entry *entries = clipbrd->cached_enum->entries;

        TRACE("(): WM_RENDERALLFORMATS\n");

        for(i = 0; i < clipbrd->cached_enum->count; i++)
        {
            if(entries[i].first_use)
                render_format(clipbrd->src_data, &entries[i].fmtetc);
        }
        break;
    }

    case WM_DESTROYCLIPBOARD:
    {
        TRACE("(): WM_DESTROYCLIPBOARD\n");

        set_src_dataobject(clipbrd, NULL);
        break;
    }

    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    return 0;
}


/***********************************************************************
 *                 create_clipbrd_window
 */
static HWND create_clipbrd_window(void)
{
    WNDCLASSEXW class;
    static const WCHAR ole32W[] = {'o','l','e','3','2',0};
    static const WCHAR title[] = {'C','l','i','p','b','o','a','r','d','W','i','n','d','o','w',0};
    HINSTANCE hinst = GetModuleHandleW(ole32W);

    class.cbSize         = sizeof(class);
    class.style          = 0;
    class.lpfnWndProc    = clipbrd_wndproc;
    class.cbClsExtra     = 0;
    class.cbWndExtra     = 0;
    class.hInstance      = hinst;
    class.hIcon          = 0;
    class.hCursor        = 0;
    class.hbrBackground  = 0;
    class.lpszMenuName   = NULL;
    class.lpszClassName  = clipbrd_wndclass;
    class.hIconSm        = NULL;

    RegisterClassExW(&class);

    return CreateWindowW(clipbrd_wndclass, title, WS_POPUP | WS_CLIPSIBLINGS | WS_OVERLAPPED,
                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                         NULL, NULL, hinst, 0);
}

/*********************************************************************
 *          set_dataobject_format
 *
 * Windows creates a 'DataObject' clipboard format that contains the
 * clipboard window's HWND or NULL if the Ole clipboard has been flushed.
 */
static HRESULT set_dataobject_format(HWND hwnd)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(hwnd));
    HWND *data;

    if(!h) return E_OUTOFMEMORY;

    data = GlobalLock(h);
    *data = hwnd;
    GlobalUnlock(h);

    if(!SetClipboardData(dataobject_clipboard_format, h))
    {
        GlobalFree(h);
        return CLIPBRD_E_CANT_SET;
    }

    return S_OK;
}

/*---------------------------------------------------------------------*
 *           Win32 OLE clipboard API
 *---------------------------------------------------------------------*/

/***********************************************************************
 *           OleSetClipboard     [OLE32.@]
 *  Places a pointer to the specified data object onto the clipboard,
 *  making the data object accessible to the OleGetClipboard function.
 *
 * RETURNS
 *
 *    S_OK                  IDataObject pointer placed on the clipboard
 *    CLIPBRD_E_CANT_OPEN   OpenClipboard failed
 *    CLIPBRD_E_CANT_EMPTY  EmptyClipboard failed
 *    CLIPBRD_E_CANT_CLOSE  CloseClipboard failed
 *    CLIPBRD_E_CANT_SET    SetClipboard failed
 */

HRESULT WINAPI OleSetClipboard(IDataObject* data)
{
  HRESULT hr;
  ole_clipbrd *clipbrd;
  HWND wnd;

  TRACE("(%p)\n", data);

  if(FAILED(hr = get_ole_clipbrd(&clipbrd))) return hr;

  if(FAILED(hr = get_clipbrd_window(clipbrd, &wnd))) return hr;

  if ( !OpenClipboard(wnd) ) return CLIPBRD_E_CANT_OPEN;

  if ( !EmptyClipboard() )
  {
    hr = CLIPBRD_E_CANT_EMPTY;
    goto end;
  }

  hr = set_src_dataobject(clipbrd, data);
  if(FAILED(hr)) goto end;

  if(data)
  {
    hr = expose_marshalled_dataobject(clipbrd, data);
    if(FAILED(hr)) goto end;
    hr = set_dataobject_format(wnd);
  }

end:

  if ( !CloseClipboard() ) hr = CLIPBRD_E_CANT_CLOSE;

  if ( FAILED(hr) )
  {
    expose_marshalled_dataobject(clipbrd, NULL);
    set_src_dataobject(clipbrd, NULL);
  }

  return hr;
}


/***********************************************************************
 * OleGetClipboard [OLE32.@]
 * Returns a pointer to our internal IDataObject which represents the conceptual
 * state of the Windows clipboard. If the current clipboard already contains
 * an IDataObject, our internal IDataObject will delegate to this object.
 */
HRESULT WINAPI OleGetClipboard(IDataObject **obj)
{
    HRESULT hr;
    ole_clipbrd *clipbrd;
    DWORD seq_no;

    TRACE("(%p)\n", obj);

    if(!obj) return E_INVALIDARG;

    if(FAILED(hr = get_ole_clipbrd(&clipbrd))) return hr;

    seq_no = GetClipboardSequenceNumber();
    if(clipbrd->latest_snapshot && clipbrd->latest_snapshot->seq_no != seq_no)
        clipbrd->latest_snapshot = NULL;

    if(!clipbrd->latest_snapshot)
    {
        clipbrd->latest_snapshot = snapshot_construct(seq_no);
        if(!clipbrd->latest_snapshot) return E_OUTOFMEMORY;
    }

    *obj = (IDataObject*)&clipbrd->latest_snapshot->lpVtbl;
    IDataObject_AddRef(*obj);

    return S_OK;
}

/******************************************************************************
 *              OleFlushClipboard        [OLE32.@]
 *  Renders the data from the source IDataObject into the windows clipboard
 *
 *  TODO: OleFlushClipboard needs to additionally handle TYMED_IStorage media
 *  by copying the storage into global memory. Subsequently the default
 *  data object exposed through OleGetClipboard must convert this TYMED_HGLOBAL
 *  back to TYMED_IStorage.
 */
HRESULT WINAPI OleFlushClipboard(void)
{
  HRESULT hr;
  ole_clipbrd *clipbrd;
  HWND wnd;

  TRACE("()\n");

  if(FAILED(hr = get_ole_clipbrd(&clipbrd))) return hr;

  if(FAILED(hr = get_clipbrd_window(clipbrd, &wnd))) return hr;

  /*
   * Already flushed or no source DataObject? Nothing to do.
   */
  if (!clipbrd->src_data) return S_OK;

  if (!OpenClipboard(wnd)) return CLIPBRD_E_CANT_OPEN;

  SendMessageW(wnd, WM_RENDERALLFORMATS, 0, 0);

  hr = set_dataobject_format(NULL);

  expose_marshalled_dataobject(clipbrd, NULL);
  set_src_dataobject(clipbrd, NULL);

  if ( !CloseClipboard() ) hr = CLIPBRD_E_CANT_CLOSE;

  return hr;
}


/***********************************************************************
 *           OleIsCurrentClipboard [OLE32.@]
 */
HRESULT WINAPI OleIsCurrentClipboard(IDataObject *data)
{
    HRESULT hr;
    ole_clipbrd *clipbrd;
    TRACE("()\n");

    if(FAILED(hr = get_ole_clipbrd(&clipbrd))) return hr;

    if (data == NULL) return S_FALSE;

    return (data == clipbrd->src_data) ? S_OK : S_FALSE;
}
