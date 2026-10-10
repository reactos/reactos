/*
 * Copyright 2002 Michael Günnewig
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

#ifndef __WINE_EXTRACHUNK_H
#define __WINE_EXTRACHUNK_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EXTRACHUNKS {
  LPVOID lp;
  DWORD  cb;
} EXTRACHUNKS, *LPEXTRACHUNKS;

/* reads a chunk outof the extrachunk-structure */
HRESULT ReadExtraChunk(const EXTRACHUNKS *extra,FOURCC ckid,LPVOID lp,LPLONG size);

/* writes a chunk into the extrachunk-structure */
HRESULT WriteExtraChunk(LPEXTRACHUNKS extra,FOURCC ckid,LPCVOID lp,LONG size);

/* reads a chunk from the HMMIO into the extrachunk-structure */
HRESULT ReadChunkIntoExtra(LPEXTRACHUNKS extra,HMMIO hmmio,const MMCKINFO *lpck);

/* reads all non-junk chunks into the extrachunk-structure until it finds
 * the given chunk or the optional parent-chunk is at the end */
HRESULT FindChunkAndKeepExtras(LPEXTRACHUNKS extra,HMMIO hmmio,
			       MMCKINFO *lpck,MMCKINFO *lpckParent,UINT flags);

#ifdef __cplusplus
}
#endif

#endif
