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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "olestd.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define MAX_CLIPFORMAT_NAME   80

/******************************************************************************
 *		OleQueryCreateFromData [OLE32.@]
 *
 * Author   : Abey George
 * Checks whether an object can become an embedded object.
 * the clipboard or OLE drag and drop.
 * Returns  : S_OK - Format that supports Embedded object creation are present.
 *            OLE_E_STATIC - Format that supports static object creation are present.
 *            S_FALSE - No acceptable format is available.
 */

HRESULT WINAPI OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject)
{
  IEnumFORMATETC *pfmt;
  FORMATETC fmt;
  CHAR szFmtName[MAX_CLIPFORMAT_NAME];
  BOOL bFoundStatic = FALSE;

  HRESULT hr = IDataObject_EnumFormatEtc(pSrcDataObject, DATADIR_GET, &pfmt);

  if (hr == S_OK)
    hr = IEnumFORMATETC_Next(pfmt, 1, &fmt, NULL);

  while (hr == S_OK)
  {
    GetClipboardFormatNameA(fmt.cfFormat, szFmtName, MAX_CLIPFORMAT_NAME-1);

    /* first, Check for Embedded Object, Embed Source or Filename */

    if (!strcmp(szFmtName, CF_EMBEDDEDOBJECT) || !strcmp(szFmtName, CF_EMBEDSOURCE) || !strcmp(szFmtName, CF_FILENAME))
      return S_OK;

    /* Check for Metafile, Bitmap or DIB */

    if (fmt.cfFormat == CF_METAFILEPICT || fmt.cfFormat == CF_BITMAP || fmt.cfFormat == CF_DIB)
      bFoundStatic = TRUE;

    hr = IEnumFORMATETC_Next(pfmt, 1, &fmt, NULL);
  }

  /* Found a static format, but no embed format */

  if (bFoundStatic)
    return OLE_S_STATIC;

  return S_FALSE;
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
            hr1 = OleCreateDefaultHandler(&clsID, NULL, &IID_IPersistStorage, (LPVOID*)(char*)&pPersistStorage);

          /* Load the storage to Persist storage */

          if (hr1 == S_OK)
            hr1 = IPersistStorage_Load(pPersistStorage, pStorage);

          /* Query for IOleObject */

          if (hr1 == S_OK)
            hr1 = IPersistStorage_QueryInterface(pPersistStorage, &IID_IOleObject, (LPVOID*)(char*)&pOleObject);

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
