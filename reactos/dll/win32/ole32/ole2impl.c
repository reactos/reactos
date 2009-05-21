/*
 * Ole 2 Create functions implementation
 *
 * Copyright (C) 1999-2000 Abey George
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
#include "wine/debug.h"
#include "ole2.h"
#include "olestd.h"
#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define MAX_CLIPFORMAT_NAME   80

/******************************************************************************
 *		OleQueryCreateFromData [OLE32.@]
 *
 * Checks whether an object can become an embedded object.
 * the clipboard or OLE drag and drop.
 * Returns  : S_OK - Format that supports Embedded object creation are present.
 *            OLE_E_STATIC - Format that supports static object creation are present.
 *            S_FALSE - No acceptable format is available.
 */

HRESULT WINAPI OleQueryCreateFromData(IDataObject *data)
{
    IEnumFORMATETC *enum_fmt;
    FORMATETC fmt;
    BOOL found_static = FALSE;
    HRESULT hr;

    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);

    if(FAILED(hr)) return hr;

    do
    {
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
        if(hr == S_OK)
        {
            if(fmt.cfFormat == embedded_object_clipboard_format ||
               fmt.cfFormat == embed_source_clipboard_format ||
               fmt.cfFormat == filename_clipboard_format)
            {
                IEnumFORMATETC_Release(enum_fmt);
                return S_OK;
            }

            if(fmt.cfFormat == CF_METAFILEPICT ||
               fmt.cfFormat == CF_BITMAP ||
               fmt.cfFormat == CF_DIB)
                found_static = TRUE;
        }
    } while (hr == S_OK);

    IEnumFORMATETC_Release(enum_fmt);

    return found_static ? OLE_S_STATIC : S_FALSE;
}

/******************************************************************************
 *		OleCreateFromDataEx        [OLE32.@]
 *
 * Creates an embedded object from data transfer object retrieved from
 * the clipboard or OLE drag and drop.
 */
HRESULT WINAPI OleCreateFromDataEx(IDataObject *data, REFIID iid, DWORD flags,
                                   DWORD renderopt, ULONG num_fmts, DWORD *adv_flags, FORMATETC *fmts,
                                   IAdviseSink *sink, DWORD *conns,
                                   IOleClientSite *client_site, IStorage *stg, void **obj)
{
    FIXME("(%p, %s, %08x, %08x, %d, %p, %p, %p, %p, %p, %p, %p): stub\n",
          data, debugstr_guid(iid), flags, renderopt, num_fmts, adv_flags, fmts,
          sink, conns, client_site, stg, obj);

    return E_NOTIMPL;
}

/******************************************************************************
 *		OleCreateFromData        [OLE32.@]
 *
 * Author   : Abey George
 * Creates an embedded object from data transfer object retrieved from
 * the clipboard or OLE drag and drop.
 * Returns  : S_OK - Embedded object was created successfully.
 *            OLE_E_STATIC - OLE can create only a static object
 *            DV_E_FORMATETC - No acceptable format is available (only error return code)
 * TODO : CF_FILENAME, CF_EMBEDEDOBJECT formats. Parameter renderopt is currently ignored.
 */

HRESULT WINAPI OleCreateFromData(LPDATAOBJECT pSrcDataObject, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID* ppvObj)
{
  IEnumFORMATETC *pfmt;
  FORMATETC fmt;
  CHAR szFmtName[MAX_CLIPFORMAT_NAME];
  STGMEDIUM std;
  HRESULT hr;
  HRESULT hr1;

  hr = IDataObject_EnumFormatEtc(pSrcDataObject, DATADIR_GET, &pfmt);

  if (hr == S_OK)
  {
    memset(&std, 0, sizeof(STGMEDIUM));

    hr = IEnumFORMATETC_Next(pfmt, 1, &fmt, NULL);
    while (hr == S_OK)
    {
      GetClipboardFormatNameA(fmt.cfFormat, szFmtName, MAX_CLIPFORMAT_NAME-1);

      /* first, Check for Embedded Object, Embed Source or Filename */
      /* TODO: Currently checks only for Embed Source. */

      if (!strcmp(szFmtName, CF_EMBEDSOURCE))
      {
        std.tymed = TYMED_HGLOBAL;

        if ((hr1 = IDataObject_GetData(pSrcDataObject, &fmt, &std)) == S_OK)
        {
          ILockBytes *ptrILockBytes = 0;
          IStorage *pStorage = 0;
          IOleObject *pOleObject = 0;
          IPersistStorage *pPersistStorage = 0;
          CLSID clsID;

          /* Create ILock bytes */

          hr1 = CreateILockBytesOnHGlobal(std.u.hGlobal, FALSE, &ptrILockBytes);

          /* Open storage on the ILock bytes */

          if (hr1 == S_OK)
            hr1 = StgOpenStorageOnILockBytes(ptrILockBytes, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);

          /* Get Class ID from the opened storage */

          if (hr1 == S_OK)
            hr1 = ReadClassStg(pStorage, &clsID);

          /* Create default handler for Persist storage */

          if (hr1 == S_OK)
            hr1 = OleCreateDefaultHandler(&clsID, NULL, &IID_IPersistStorage, (LPVOID*)&pPersistStorage);

          /* Load the storage to Persist storage */

          if (hr1 == S_OK)
            hr1 = IPersistStorage_Load(pPersistStorage, pStorage);

          /* Query for IOleObject */

          if (hr1 == S_OK)
            hr1 = IPersistStorage_QueryInterface(pPersistStorage, &IID_IOleObject, (LPVOID*)&pOleObject);

          /* Set client site with the IOleObject */

          if (hr1 == S_OK)
            hr1 = IOleObject_SetClientSite(pOleObject, pClientSite);

          IPersistStorage_Release(pPersistStorage);
          /* Query for the requested interface */

          if (hr1 == S_OK)
            hr1 = IPersistStorage_QueryInterface(pPersistStorage, riid, ppvObj);

          IPersistStorage_Release(pPersistStorage);

          IStorage_Release(pStorage);

          if (hr1 == S_OK)
            return S_OK;
        }

        /* Return error */

        return DV_E_FORMATETC;
      }

      hr = IEnumFORMATETC_Next(pfmt, 1, &fmt, NULL);
    }
  }

  return DV_E_FORMATETC;
}


/******************************************************************************
 *              OleDuplicateData        [OLE32.@]
 *
 * Duplicates clipboard data.
 *
 * PARAMS
 *  hSrc     [I] Handle of the source clipboard data.
 *  cfFormat [I] The clipboard format of hSrc.
 *  uiFlags  [I] Flags to pass to GlobalAlloc.
 *
 * RETURNS
 *  Success: handle to the duplicated data.
 *  Failure: NULL.
 */
HANDLE WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat,
	                          UINT uiFlags)
{
    HANDLE hDst = NULL;

    TRACE("(%p,%x,%x)\n", hSrc, cfFormat, uiFlags);

    if (!uiFlags) uiFlags = GMEM_MOVEABLE;

    switch (cfFormat)
    {
    case CF_ENHMETAFILE:
        hDst = CopyEnhMetaFileW(hSrc, NULL);
        break;
    case CF_METAFILEPICT:
        hDst = CopyMetaFileW(hSrc, NULL);
        break;
    case CF_PALETTE:
        {
            LOGPALETTE * logpalette;
            UINT nEntries = GetPaletteEntries(hSrc, 0, 0, NULL);
            if (!nEntries) return NULL;
            logpalette = HeapAlloc(GetProcessHeap(), 0,
                FIELD_OFFSET(LOGPALETTE, palPalEntry[nEntries]));
            if (!logpalette) return NULL;
            if (!GetPaletteEntries(hSrc, 0, nEntries, logpalette->palPalEntry))
            {
                HeapFree(GetProcessHeap(), 0, logpalette);
                return NULL;
            }
            logpalette->palVersion = 0x300;
            logpalette->palNumEntries = (WORD)nEntries;

            hDst = CreatePalette(logpalette);

            HeapFree(GetProcessHeap(), 0, logpalette);
            break;
        }
    case CF_BITMAP:
        {
            LONG size;
            BITMAP bm;
            if (!GetObjectW(hSrc, sizeof(bm), &bm))
                return NULL;
            size = GetBitmapBits(hSrc, 0, NULL);
            if (!size) return NULL;
            bm.bmBits = HeapAlloc(GetProcessHeap(), 0, size);
            if (!bm.bmBits) return NULL;
            if (GetBitmapBits(hSrc, size, bm.bmBits))
                hDst = CreateBitmapIndirect(&bm);
            HeapFree(GetProcessHeap(), 0, bm.bmBits);
            break;
        }
    default:
        {
            SIZE_T size = GlobalSize(hSrc);
            LPVOID pvSrc = NULL;
            LPVOID pvDst = NULL;

            /* allocate space for object */
            if (!size) return NULL;
            hDst = GlobalAlloc(uiFlags, size);
            if (!hDst) return NULL;

            /* lock pointers */
            pvSrc = GlobalLock(hSrc);
            if (!pvSrc)
            {
                GlobalFree(hDst);
                return NULL;
            }
            pvDst = GlobalLock(hDst);
            if (!pvDst)
            {
                GlobalUnlock(hSrc);
                GlobalFree(hDst);
                return NULL;
            }
            /* copy data */
            memcpy(pvDst, pvSrc, size);

            /* cleanup */
            GlobalUnlock(hDst);
            GlobalUnlock(hSrc);
        }
    }

    TRACE("returning %p\n", hDst);
    return hDst;
}
