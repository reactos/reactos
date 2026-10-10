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

#include <assert.h>

#include "extrachunk.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/* reads a chunk out of the extrachunk-structure */
HRESULT ReadExtraChunk(const EXTRACHUNKS *extra,FOURCC ckid,LPVOID lpData,LPLONG size)
{
  LPBYTE lp;
  DWORD  cb;

  /* pre-conditions */
  assert(extra != NULL);
  assert(size != NULL);

  lp = extra->lp;
  cb = extra->cb;

  if (lp != NULL) {
    while (cb > 0) {
      if (((FOURCC*)lp)[0] == ckid) {
	/* found correct chunk */
	if (lpData != NULL && *size > 0)
	  memcpy(lpData, lp + 2 * sizeof(DWORD),
		 min(((LPDWORD)lp)[1], *(LPDWORD)size));

	*(LPDWORD)size = ((LPDWORD)lp)[1];

	return AVIERR_OK;
      } else {
	/* skip to next chunk */
	cb -= ((LPDWORD)lp)[1] + 2 * sizeof(DWORD);
	lp += ((LPDWORD)lp)[1] + 2 * sizeof(DWORD);
      }
    }
  }

  /* wanted chunk doesn't exist */
  *size = 0;

  return AVIERR_NODATA;
}

/* writes a chunk into the extrachunk-structure */
HRESULT WriteExtraChunk(LPEXTRACHUNKS extra,FOURCC ckid,LPCVOID lpData, LONG size)
{
  LPDWORD lp;

  /* pre-conditions */
  assert(extra != NULL);
  assert(lpData != NULL);
  assert(size > 0);

  lp = _recalloc(extra->lp, 1, extra->cb + size + 2 * sizeof(DWORD));
  if (lp == NULL)
    return AVIERR_MEMORY;

  extra->lp  = lp;
  lp = (LPDWORD) ((LPBYTE)lp + extra->cb);
  extra->cb += size + 2 * sizeof(DWORD);

  /* insert chunk-header in block */
  lp[0] = ckid;
  lp[1] = size;

  if (lpData != NULL && size > 0)
    memcpy(lp + 2, lpData, size);

  return AVIERR_OK;
}

/* reads a chunk from the HMMIO into the extrachunk-structure */
HRESULT ReadChunkIntoExtra(LPEXTRACHUNKS extra,HMMIO hmmio,const MMCKINFO *lpck)
{
  LPDWORD lp;
  DWORD   cb;

  /* pre-conditions */
  assert(extra != NULL);
  assert(hmmio != NULL);
  assert(lpck  != NULL);

  cb  = lpck->cksize + 2 * sizeof(DWORD);
  cb += (cb & 1);

  lp = _recalloc(extra->lp, 1, extra->cb + cb);
  if (lp == NULL)
    return AVIERR_MEMORY;

  extra->lp  = lp;
  lp = (LPDWORD) ((LPBYTE)lp + extra->cb);
  extra->cb += cb;

  /* insert chunk-header in block */
  lp[0] = lpck->ckid;
  lp[1] = lpck->cksize;

  if (lpck->cksize > 0) {
    if (mmioSeek(hmmio, lpck->dwDataOffset, SEEK_SET) == -1)
      return AVIERR_FILEREAD;
    if (mmioRead(hmmio, (HPSTR)&lp[2], lpck->cksize) != (LONG)lpck->cksize)
      return AVIERR_FILEREAD;
  }

  return AVIERR_OK;
}

/* reads all non-junk chunks into the extrachunk-structure until it finds
 * the given chunk or the optional parent-chunk is at the end */
HRESULT FindChunkAndKeepExtras(LPEXTRACHUNKS extra,HMMIO hmmio,MMCKINFO *lpck,
			       MMCKINFO *lpckParent,UINT flags)
{
  FOURCC  ckid;
  FOURCC  fccType;
  MMRESULT mmr;

  /* pre-conditions */
  assert(extra != NULL);
  assert(hmmio != NULL);
  assert(lpck  != NULL);

  TRACE("({%p,%lu},%p,%p,%p,0x%X)\n", extra->lp, extra->cb, hmmio, lpck,
	lpckParent, flags);

  /* what chunk id and form/list type should we search? */
  if (flags & MMIO_FINDCHUNK) {
    ckid    = lpck->ckid;
    fccType = 0;
  } else if (flags & MMIO_FINDLIST) {
    ckid    = FOURCC_LIST;
    fccType = lpck->fccType;
  } else if (flags & MMIO_FINDRIFF) {
    ckid    = FOURCC_RIFF;
    fccType = lpck->fccType;
  } else
    ckid = fccType = (FOURCC)-1; /* collect everything into extra! */

  TRACE(": find ckid=0x%08lX fccType=0x%08lX\n", ckid, fccType);

  for (;;) {
    mmr = mmioDescend(hmmio, lpck, lpckParent, 0);
    if (mmr != MMSYSERR_NOERROR) {
      /* No extra chunks in front of desired chunk? */
      if (flags == 0 && mmr == MMIOERR_CHUNKNOTFOUND)
	return AVIERR_OK;
      else
        return AVIERR_FILEREAD;
    }

    /* Have we found what we search for? */
    if ((lpck->ckid == ckid) &&
	(fccType == 0 || lpck->fccType == fccType))
      return AVIERR_OK;

    /* Skip padding chunks, the others put into the extrachunk-structure */
    if (lpck->ckid == ckidAVIPADDING ||
	lpck->ckid == mmioFOURCC('p','a','d','d'))
    {
      mmr = mmioAscend(hmmio, lpck, 0);
      if (mmr != MMSYSERR_NOERROR) return AVIERR_FILEREAD;
    }
    else
    {
      HRESULT hr = ReadChunkIntoExtra(extra, hmmio, lpck);
      if (FAILED(hr))
        return hr;
    }
  }
}
