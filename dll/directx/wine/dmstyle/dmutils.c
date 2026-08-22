/* Debug and Helper Functions
 *
 * Copyright (C) 2004 Rok Mandeljc
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
 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

#include "dmutils.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmfile);

HRESULT IDirectMusicUtils_IPersistStream_ParseDescGeneric (DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, LPDMUS_OBJECTDESC pDesc) {

  switch (pChunk->fccID) {
  case DMUS_FOURCC_GUID_CHUNK: {
    TRACE(": GUID chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_OBJECT;
    IStream_Read (pStm, &pDesc->guidObject, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_DATE_CHUNK: {
    TRACE(": file date chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_DATE;
    IStream_Read (pStm, &pDesc->ftDate, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_NAME_CHUNK: {
    TRACE(": name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_NAME;
    IStream_Read (pStm, pDesc->wszName, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_FILE_CHUNK: {
    TRACE(": file name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_FILENAME;
    IStream_Read (pStm, pDesc->wszFileName, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_VERSION_CHUNK: {
    TRACE(": version chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_VERSION;
    IStream_Read (pStm, &pDesc->vVersion, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_CATEGORY_CHUNK: {
    TRACE(": category chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
    IStream_Read (pStm, pDesc->wszCategory, pChunk->dwSize, NULL);
    break;
  }
  default:
    /* not handled */
    return S_FALSE;
  }

  return S_OK;
}

HRESULT IDirectMusicUtils_IPersistStream_ParseUNFOGeneric (DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, LPDMUS_OBJECTDESC pDesc) {

  LARGE_INTEGER liMove; /* used when skipping chunks */

  /**
   * don't ask me why, but M$ puts INFO elements in UNFO list sometimes
   * (though strings seem to be valid unicode) 
   */
  switch (pChunk->fccID) {

  case mmioFOURCC('I','N','A','M'):
  case DMUS_FOURCC_UNAM_CHUNK: {
    TRACE(": name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_NAME;
    IStream_Read (pStm, pDesc->wszName, pChunk->dwSize, NULL);
    TRACE(" - wszName: %s\n", debugstr_w(pDesc->wszName));
    break;
  }

  case mmioFOURCC('I','A','R','T'):
  case DMUS_FOURCC_UART_CHUNK: {
    TRACE(": artist chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','C','O','P'):
  case DMUS_FOURCC_UCOP_CHUNK: {
    TRACE(": copyright chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','S','B','J'):
  case DMUS_FOURCC_USBJ_CHUNK: {
    TRACE(": subject chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','C','M','T'):
  case DMUS_FOURCC_UCMT_CHUNK: {
    TRACE(": comment chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  default:
    /* not handled */
    return S_FALSE;
  }

  return S_OK;
}
