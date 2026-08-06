/*
 * Copyright 2003 Michael Günnewig
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winerror.h"
#include "mmsystem.h"
#include "vfw.h"

#include "avifile_private.h"
#include "extrachunk.h"

#include "wine/debug.h"
#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************/

/* internal interface to get access to table of stream in an editable stream */

DEFINE_AVIGUID(IID_IEditStreamInternal, 0x0002000A,0,0);

typedef struct _EditStreamTable {
  PAVISTREAM pStream;  /* stream which contains the data */
  DWORD      dwStart;  /* where starts the part which is also our */
  DWORD      dwLength; /* how many is also in this stream */
} EditStreamTable;

#define EditStreamEnd(This,streamNr) ((This)->pStreams[streamNr].dwStart + \
                                      (This)->pStreams[streamNr].dwLength)

typedef struct _IAVIEditStreamImpl IAVIEditStreamImpl;

struct _IAVIEditStreamImpl {
  IAVIEditStream       IAVIEditStream_iface;
  IAVIStream           IAVIStream_iface;

  LONG                 ref;

  AVISTREAMINFOW       sInfo;

  EditStreamTable     *pStreams;
  DWORD                nStreams;   /* current fill level of pStreams table */
  DWORD                nTableSize; /* size of pStreams table */

  BOOL                 bDecompress;
  PAVISTREAM           pCurStream;
  PGETFRAME            pg;         /* IGetFrame for pCurStream */
  LPBITMAPINFOHEADER   lpFrame;    /* frame of pCurStream */
};

static IAVIEditStreamImpl *AVIFILE_CreateEditStream(IAVIStream *stream);

static inline IAVIEditStreamImpl *impl_from_IAVIEditStream(IAVIEditStream *iface)
{
    return CONTAINING_RECORD(iface, IAVIEditStreamImpl, IAVIEditStream_iface);
}

static inline IAVIEditStreamImpl *impl_from_IAVIStream(IAVIStream *iface)
{
    return CONTAINING_RECORD(iface, IAVIEditStreamImpl, IAVIStream_iface);
}

/***********************************************************************/

static HRESULT AVIFILE_FindStreamInTable(IAVIEditStreamImpl* const This,
					 DWORD pos,PAVISTREAM *ppStream,
					 DWORD* streamPos,
					 DWORD* streamNr,BOOL bFindSample)
{
  DWORD n;

  TRACE("(%p,%lu,%p,%p,%p,%d)\n",This,pos,ppStream,streamPos,
        streamNr,bFindSample);

  if (pos < This->sInfo.dwStart)
    return AVIERR_BADPARAM;

  pos -= This->sInfo.dwStart;
  for (n = 0; n < This->nStreams; n++) {
    if (pos < This->pStreams[n].dwLength) {
      *ppStream  = This->pStreams[n].pStream;
      *streamPos = This->pStreams[n].dwStart + pos;
      if (streamNr != NULL)
        *streamNr = n;

      return AVIERR_OK;
    }
    pos -= This->pStreams[n].dwLength;
  }
  if (pos == 0 && bFindSample) {
    *ppStream  = This->pStreams[--n].pStream;
    *streamPos = EditStreamEnd(This, n);
    if (streamNr != NULL)
      *streamNr = n;

    TRACE(" -- pos=0 && b=1 -> (%p,%lu,%lu)\n",*ppStream, *streamPos, n);
    return AVIERR_OK;
  } else {
    *ppStream = NULL;
    *streamPos = 0;
    if (streamNr != NULL)
      *streamNr = 0;

    TRACE(" -> ERROR (NULL,0,0)\n");
    return AVIERR_BADPARAM;
  }
}

static LPVOID AVIFILE_ReadFrame(IAVIEditStreamImpl* const This,
                                PAVISTREAM pstream, LONG pos)
{
  PGETFRAME pg;

  TRACE("(%p,%p,%ld)\n",This,pstream,pos);

  if (pstream == NULL)
    return NULL;

  /* if stream changes make sure that only palette changes */
  if (This->pCurStream != pstream) {
    pg = AVIStreamGetFrameOpen(pstream, NULL);
    if (pg == NULL)
      return NULL;
    if (This->pg != NULL) {
      if (IGetFrame_SetFormat(pg, This->lpFrame, NULL, 0, 0, -1, -1) != S_OK) {
        AVIStreamGetFrameClose(pg);
        ERR(": IGetFrame_SetFormat failed\n");
        return NULL;
      }
      AVIStreamGetFrameClose(This->pg);
    }
    This->pg         = pg;
    This->pCurStream = pstream;
  }

  /* now get the decompressed frame */
  This->lpFrame = AVIStreamGetFrame(This->pg, pos);
  if (This->lpFrame != NULL)
    This->sInfo.dwSuggestedBufferSize = This->lpFrame->biSizeImage;

  return This->lpFrame;
}

static HRESULT AVIFILE_RemoveStream(IAVIEditStreamImpl* const This, DWORD nr)
{
  assert(This != NULL);
  assert(nr < This->nStreams);

  /* remove part nr */
  IAVIStream_Release(This->pStreams[nr].pStream);
  This->nStreams--;
  if (nr < This->nStreams)
    memmove(&This->pStreams[nr], &This->pStreams[nr + 1],
            (This->nStreams - nr) * sizeof(This->pStreams[0]));
  This->pStreams[This->nStreams].pStream  = NULL;
  This->pStreams[This->nStreams].dwStart  = 0;
  This->pStreams[This->nStreams].dwLength = 0;

  /* try to merge the part before the deleted one and the one after it */
  if (0 < nr && 0 < This->nStreams &&
      This->pStreams[nr - 1].pStream == This->pStreams[nr].pStream) {
    if (EditStreamEnd(This, nr - 1) == This->pStreams[nr].dwStart) {
      This->pStreams[nr - 1].dwLength += This->pStreams[nr].dwLength;
      return AVIFILE_RemoveStream(This, nr);
    }
  }

  return AVIERR_OK;
}

static BOOL AVIFILE_FormatsEqual(PAVISTREAM avi1, PAVISTREAM avi2)
{
  LPVOID fmt1 = NULL, fmt2 = NULL;
  LONG size1, size2, start1, start2;
  BOOL status = FALSE;

  assert(avi1 != NULL && avi2 != NULL);

  /* get stream starts and check format sizes */
  start1 = AVIStreamStart(avi1);
  start2 = AVIStreamStart(avi2);
  if (FAILED(AVIStreamFormatSize(avi1, start1, &size1)))
    return FALSE;
  if (FAILED(AVIStreamFormatSize(avi2, start2, &size2)))
    return FALSE;
  if (size1 != size2)
    return FALSE;

  /* sizes match, now get formats and compare them */
  fmt1 = malloc(size1);
  if (fmt1 == NULL)
    return FALSE;
  if (SUCCEEDED(AVIStreamReadFormat(avi1, start1, fmt1, &size1))) {
    fmt2 = malloc(size1);
    if (fmt2 != NULL) {
      if (SUCCEEDED(AVIStreamReadFormat(avi2, start2, fmt2, &size1)))
        status = (memcmp(fmt1, fmt2, size1) == 0);
    }
  }

  free(fmt2);
  free(fmt1);

  return status;
}

/***********************************************************************/

static HRESULT WINAPI IAVIEditStream_fnQueryInterface(IAVIEditStream*iface,REFIID refiid,LPVOID *obj)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(refiid), obj);

  if (IsEqualGUID(&IID_IUnknown, refiid) ||
      IsEqualGUID(&IID_IAVIEditStream, refiid) ||
      IsEqualGUID(&IID_IEditStreamInternal, refiid)) {
    *obj = iface;
    IAVIEditStream_AddRef(iface);

    return S_OK;
  } else if (IsEqualGUID(&IID_IAVIStream, refiid)) {
    *obj = &This->IAVIStream_iface;
    IAVIEditStream_AddRef(iface);

    return S_OK;
  }

  return E_NOINTERFACE;
}

static ULONG   WINAPI IAVIEditStream_fnAddRef(IAVIEditStream*iface)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  return ref;
}

static ULONG   WINAPI IAVIEditStream_fnRelease(IAVIEditStream*iface)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  DWORD i;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  if (!ref) {
    /* release memory */
    if (This->pg != NULL)
      AVIStreamGetFrameClose(This->pg);
    if (This->pStreams != NULL) {
      for (i = 0; i < This->nStreams; i++) {
        if (This->pStreams[i].pStream != NULL)
          IAVIStream_Release(This->pStreams[i].pStream);
      }
      free(This->pStreams);
    }

    free(This);
  }
  return ref;
}

static HRESULT WINAPI IAVIEditStream_fnCut(IAVIEditStream*iface,LONG*plStart,
                                           LONG*plLength,PAVISTREAM*ppResult)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  PAVISTREAM stream;
  DWORD      start, len, streamPos, streamNr;
  HRESULT    hr;

  TRACE("(%p,%p,%p,%p)\n",iface,plStart,plLength,ppResult);

  if (ppResult != NULL)
    *ppResult = NULL;
  if (plStart == NULL || plLength == NULL || *plStart < 0)
    return AVIERR_BADPARAM;

  /* if asked for cut part copy it before deleting */
  if (ppResult != NULL) {
    hr = IAVIEditStream_Copy(iface, plStart, plLength, ppResult);
    if (FAILED(hr))
      return hr;
  }

  start = *plStart;
  len   = *plLength;

  /* now delete the requested part */
  while (len > 0) {
    hr = AVIFILE_FindStreamInTable(This, start, &stream,
                                   &streamPos, &streamNr, FALSE);
    if (FAILED(hr))
      return hr;
    if (This->pStreams[streamNr].dwStart == streamPos) {
      /* deleting from start of part */
      if (len < This->pStreams[streamNr].dwLength) {
        start += len;
        This->pStreams[streamNr].dwStart  += len;
        This->pStreams[streamNr].dwLength -= len;
        This->sInfo.dwLength -= len;
        len = 0;

        /* we must return decompressed data now */
        This->bDecompress = TRUE;
      } else {
        /* deleting hole part */
        len -= This->pStreams[streamNr].dwLength;
        AVIFILE_RemoveStream(This,streamNr);
      }
    } else if (EditStreamEnd(This, streamNr) <= streamPos + len) {
      /* deleting at end of a part */
      DWORD count = EditStreamEnd(This, streamNr) - streamPos;
      This->sInfo.dwLength -= count;
      len                  -= count;
      This->pStreams[streamNr].dwLength =
        streamPos - This->pStreams[streamNr].dwStart;
    } else {
      /* splitting */
      if (This->nStreams + 1 >= This->nTableSize) {
        This->pStreams = _recalloc(This->pStreams, This->nTableSize + 32, sizeof(EditStreamTable));
        if (This->pStreams == NULL)
          return AVIERR_MEMORY;
        This->nTableSize += 32;
      }
      memmove(This->pStreams + streamNr + 1, This->pStreams + streamNr,
              (This->nStreams - streamNr) * sizeof(EditStreamTable));
      This->nStreams++;

      IAVIStream_AddRef(This->pStreams[streamNr + 1].pStream);
      This->pStreams[streamNr + 1].dwStart  = streamPos + len;
      This->pStreams[streamNr + 1].dwLength =
        EditStreamEnd(This, streamNr) - This->pStreams[streamNr + 1].dwStart;

      This->pStreams[streamNr].dwLength =
        streamPos - This->pStreams[streamNr].dwStart;
      This->sInfo.dwLength -= len;
      len = 0;
    }
  }

  This->sInfo.dwEditCount++;

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIEditStream_fnCopy(IAVIEditStream*iface,LONG*plStart,
                                            LONG*plLength,PAVISTREAM*ppResult)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  IAVIEditStreamImpl* pEdit;
  HRESULT hr;
  LONG start = 0;

  TRACE("(%p,%p,%p,%p)\n",iface,plStart,plLength,ppResult);

  if (ppResult == NULL)
    return AVIERR_BADPARAM;
  *ppResult = NULL;
  if (plStart == NULL || plLength == NULL || *plStart < 0 || *plLength < 0)
    return AVIERR_BADPARAM;

  /* check bounds */
  if (*(LPDWORD)plLength > This->sInfo.dwLength)
    *(LPDWORD)plLength = This->sInfo.dwLength;
  if (*(LPDWORD)plStart < This->sInfo.dwStart) {
    *(LPDWORD)plLength -= This->sInfo.dwStart - *(LPDWORD)plStart;
    *(LPDWORD)plStart   = This->sInfo.dwStart;
    if (*plLength < 0)
      return AVIERR_BADPARAM;
  }
  if (*(LPDWORD)plStart + *(LPDWORD)plLength > This->sInfo.dwStart + This->sInfo.dwLength)
    *(LPDWORD)plLength = This->sInfo.dwStart + This->sInfo.dwLength -
      *(LPDWORD)plStart;

  pEdit = AVIFILE_CreateEditStream(NULL);
  if (pEdit == NULL)
    return AVIERR_MEMORY;

  hr = IAVIEditStream_Paste(&pEdit->IAVIEditStream_iface, &start, plLength, &This->IAVIStream_iface,
                            *plStart, *plStart + *plLength);
  *plStart = start;
  if (FAILED(hr))
    IAVIEditStream_Release(&pEdit->IAVIEditStream_iface);
  else
    *ppResult = &This->IAVIStream_iface;

  return hr;
}

static HRESULT WINAPI IAVIEditStream_fnPaste(IAVIEditStream*iface,LONG*plStart,
                                             LONG*plLength,PAVISTREAM pSource,
                                             LONG lStart,LONG lLength)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  AVISTREAMINFOW      srcInfo;
  IAVIEditStreamImpl *pEdit = NULL;
  PAVISTREAM          pStream;
  DWORD               startPos, endPos, streamNr, nStreams;
  ULONG               n;

  TRACE("(%p,%p,%p,%p,%ld,%ld)\n",iface,plStart,plLength,
	pSource,lStart,lLength);

  if (pSource == NULL)
    return AVIERR_BADHANDLE;
  if (plStart == NULL || *plStart < 0)
    return AVIERR_BADPARAM;
  if (This->sInfo.dwStart + This->sInfo.dwLength < *plStart)
    return AVIERR_BADPARAM; /* Can't paste with holes */
  if (FAILED(IAVIStream_Info(pSource, &srcInfo, sizeof(srcInfo))))
    return AVIERR_ERROR;
  if (lStart < srcInfo.dwStart || lStart >= srcInfo.dwStart + srcInfo.dwLength)
    return AVIERR_BADPARAM;
  if (This->sInfo.fccType == 0) {
    /* This stream is empty */
    IAVIStream_Info(pSource, &This->sInfo, sizeof(This->sInfo));
    This->sInfo.dwStart  = *plStart;
    This->sInfo.dwLength = 0;
  }
  if (This->sInfo.fccType != srcInfo.fccType)
    return AVIERR_UNSUPPORTED; /* different stream types */
  if (lLength == -1) /* Copy the hole stream */
    lLength = srcInfo.dwLength;
  if (lStart + lLength > srcInfo.dwStart + srcInfo.dwLength)
    lLength = srcInfo.dwStart + srcInfo.dwLength - lStart;
  if (lLength + *plStart >= 0x80000000)
    return AVIERR_MEMORY;

  /* streamtype specific tests */
  if (srcInfo.fccType == streamtypeVIDEO) {
    LONG size;

    size = srcInfo.rcFrame.right - srcInfo.rcFrame.left;
    if (size != This->sInfo.rcFrame.right - This->sInfo.rcFrame.left)
      return AVIERR_UNSUPPORTED; /* FIXME: Can't GetFrame convert it? */
    size = srcInfo.rcFrame.bottom - srcInfo.rcFrame.top;
    if (size != This->sInfo.rcFrame.bottom - This->sInfo.rcFrame.top)
      return AVIERR_UNSUPPORTED; /* FIXME: Can't GetFrame convert it? */
  } else if (srcInfo.fccType == streamtypeAUDIO) {
    if (!AVIFILE_FormatsEqual(&This->IAVIStream_iface, pSource))
      return AVIERR_UNSUPPORTED;
  } else {
    /* FIXME: streamtypeMIDI and streamtypeTEXT */
    return AVIERR_UNSUPPORTED;
  }

  /* try to get an IEditStreamInternal interface */
  if (SUCCEEDED(IAVIStream_QueryInterface(pSource, &IID_IEditStreamInternal, (LPVOID*)&pEdit)))
      IAVIEditStream_Release(&pEdit->IAVIEditStream_iface);  /* pSource holds a reference */

  /* for video must check for change of format */
  if (This->sInfo.fccType == streamtypeVIDEO) {
    if (! This->bDecompress) {
      /* Need to decompress if any of the following conditions matches:
       *  - pSource is an editable stream which decompresses
       *  - the nearest keyframe of pSource isn't lStart
       *  - the nearest keyframe of this stream isn't *plStart
       *  - the format of pSource doesn't match this one
       */
      if ((pEdit != NULL && pEdit->bDecompress) ||
	  AVIStreamNearestKeyFrame(pSource, lStart) != lStart ||
          AVIStreamNearestKeyFrame(&This->IAVIStream_iface, *plStart) != *plStart ||
          (This->nStreams > 0 && !AVIFILE_FormatsEqual(&This->IAVIStream_iface, pSource))) {
	/* Use first stream part to get format to convert everything to */
	AVIFILE_ReadFrame(This, This->pStreams[0].pStream,
			  This->pStreams[0].dwStart);

	/* Check if we could convert the source streams to the desired format... */
	if (pEdit != NULL) {
	  if (FAILED(AVIFILE_FindStreamInTable(pEdit, lStart, &pStream,
					       &startPos, &streamNr, TRUE)))
	    return AVIERR_INTERNAL;
	  for (n = lStart; n < lStart + lLength; streamNr++) {
	    if (AVIFILE_ReadFrame(This, pEdit->pStreams[streamNr].pStream, startPos) == NULL)
	      return AVIERR_BADFORMAT;
	    startPos = pEdit->pStreams[streamNr].dwStart;
	    n += pEdit->pStreams[streamNr].dwLength;
	  }
	} else if (AVIFILE_ReadFrame(This, pSource, lStart) == NULL)
	  return AVIERR_BADFORMAT;

	This->bDecompress      = TRUE;
	This->sInfo.fccHandler = 0;
      }
    } else if (AVIFILE_ReadFrame(This, pSource, lStart) == NULL)
      return AVIERR_BADFORMAT; /* Can't convert source to own format */
  } /* FIXME: something special for the other formats? */

  /* Make sure we have enough memory for parts */
  if (pEdit != NULL) {
    DWORD nLastStream;

    AVIFILE_FindStreamInTable(pEdit, lStart + lLength, &pStream,
			      &endPos, &nLastStream, TRUE);
    AVIFILE_FindStreamInTable(pEdit, lStart, &pStream,
			      &startPos, &streamNr, FALSE);
    if (nLastStream == streamNr)
      nLastStream++;

    nStreams = nLastStream - streamNr;
  } else 
    nStreams = 1;
  if (This->nStreams + nStreams + 1 > This->nTableSize) {
    n = This->nStreams + nStreams + 33;

    This->pStreams = _recalloc(This->pStreams, n, sizeof(EditStreamTable));
    if (This->pStreams == NULL)
      return AVIERR_MEMORY;
    This->nTableSize = n;
  }

  if (plLength != NULL)
    *plLength = lLength;

  /* now do the real work */
  if (This->sInfo.dwStart + This->sInfo.dwLength > *plStart) {
    AVIFILE_FindStreamInTable(This, *plStart, &pStream,
			      &startPos, &streamNr, FALSE);
    if (startPos != This->pStreams[streamNr].dwStart) {
      /* split stream streamNr at startPos */
      memmove(This->pStreams + streamNr + nStreams + 1,
	      This->pStreams + streamNr,
	      (This->nStreams + nStreams - streamNr + 1) * sizeof(EditStreamTable));

      This->pStreams[streamNr + 2].dwLength =
	EditStreamEnd(This, streamNr + 2) - startPos;
      This->pStreams[streamNr + 2].dwStart = startPos;
      This->pStreams[streamNr].dwLength =
	startPos - This->pStreams[streamNr].dwStart;
      IAVIStream_AddRef(This->pStreams[streamNr].pStream);
      streamNr++;
    } else {
      /* insert before stream at streamNr */
      memmove(This->pStreams + streamNr + nStreams, This->pStreams + streamNr,
	      (This->nStreams + nStreams - streamNr) * sizeof(EditStreamTable));
    }
  } else /* append the streams */
    streamNr = This->nStreams;

  if (pEdit != NULL) {
    /* insert the parts of the editable stream instead of itself */
    AVIFILE_FindStreamInTable(pEdit, lStart + lLength, &pStream,
			      &endPos, NULL, FALSE);
    AVIFILE_FindStreamInTable(pEdit, lStart, &pStream, &startPos, &n, FALSE);

    memcpy(This->pStreams + streamNr, pEdit->pStreams + n,
	   nStreams * sizeof(EditStreamTable));
    if (This->pStreams[streamNr].dwStart < startPos) {
      This->pStreams[streamNr].dwLength =
	EditStreamEnd(This, streamNr) - startPos;
      This->pStreams[streamNr].dwStart  = startPos;
    }
    if (endPos < EditStreamEnd(This, streamNr + nStreams))
      This->pStreams[streamNr + nStreams].dwLength =
	endPos - This->pStreams[streamNr + nStreams].dwStart;
  } else {
    /* a simple stream */
    This->pStreams[streamNr].pStream  = pSource;
    This->pStreams[streamNr].dwStart  = lStart;
    This->pStreams[streamNr].dwLength = lLength;
  }

  for (n = 0; n < nStreams; n++) {
    IAVIStream_AddRef(This->pStreams[streamNr + n].pStream);
    if (0 < streamNr + n &&
	This->pStreams[streamNr + n - 1].pStream != This->pStreams[streamNr + n].pStream) {
      This->sInfo.dwFlags |= AVISTREAMINFO_FORMATCHANGES;
      This->sInfo.dwFormatChangeCount++;
    }
  }
  This->sInfo.dwEditCount++;
  This->sInfo.dwLength += lLength;
  This->nStreams += nStreams;

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIEditStream_fnClone(IAVIEditStream*iface,
                                             PAVISTREAM*ppResult)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);
  IAVIEditStreamImpl* pEdit;
  DWORD i;

  TRACE("(%p,%p)\n",iface,ppResult);

  if (ppResult == NULL)
    return AVIERR_BADPARAM;
  *ppResult = NULL;

  pEdit = AVIFILE_CreateEditStream(NULL);
  if (pEdit == NULL)
    return AVIERR_MEMORY;
  if (This->nStreams > pEdit->nTableSize) {
    pEdit->pStreams = _recalloc(pEdit->pStreams, This->nStreams, sizeof(EditStreamTable));
    if (pEdit->pStreams == NULL)
      return AVIERR_MEMORY;
    pEdit->nTableSize = This->nStreams;
  }
  pEdit->nStreams = This->nStreams;
  memcpy(pEdit->pStreams, This->pStreams,
         This->nStreams * sizeof(EditStreamTable));
  memcpy(&pEdit->sInfo,&This->sInfo,sizeof(This->sInfo));
  for (i = 0; i < This->nStreams; i++) {
    if (pEdit->pStreams[i].pStream != NULL)
      IAVIStream_AddRef(pEdit->pStreams[i].pStream);
  }

  *ppResult = &This->IAVIStream_iface;

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIEditStream_fnSetInfo(IAVIEditStream*iface,
                                               LPAVISTREAMINFOW asi,LONG size)
{
  IAVIEditStreamImpl *This = impl_from_IAVIEditStream(iface);

  TRACE("(%p,%p,%ld)\n",iface,asi,size);

  /* check parameters */
  if (size >= 0 && size < sizeof(AVISTREAMINFOW))
    return AVIERR_BADSIZE;

  This->sInfo.wLanguage = asi->wLanguage;
  This->sInfo.wPriority = asi->wPriority;
  This->sInfo.dwStart   = asi->dwStart;
  This->sInfo.dwRate    = asi->dwRate;
  This->sInfo.dwScale   = asi->dwScale;
  This->sInfo.dwQuality = asi->dwQuality;
  This->sInfo.rcFrame   = asi->rcFrame;
  memcpy(This->sInfo.szName, asi->szName, sizeof(asi->szName));
  This->sInfo.dwEditCount++;

  return AVIERR_OK;
}

static const struct IAVIEditStreamVtbl ieditstream = {
  IAVIEditStream_fnQueryInterface,
  IAVIEditStream_fnAddRef,
  IAVIEditStream_fnRelease,
  IAVIEditStream_fnCut,
  IAVIEditStream_fnCopy,
  IAVIEditStream_fnPaste,
  IAVIEditStream_fnClone,
  IAVIEditStream_fnSetInfo
};

static HRESULT WINAPI IEditAVIStream_fnQueryInterface(IAVIStream*iface,
                                                      REFIID refiid,LPVOID*obj)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  return IAVIEditStream_QueryInterface(&This->IAVIEditStream_iface,refiid,obj);
}

static ULONG   WINAPI IEditAVIStream_fnAddRef(IAVIStream*iface)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  return IAVIEditStream_AddRef(&This->IAVIEditStream_iface);
}

static ULONG   WINAPI IEditAVIStream_fnRelease(IAVIStream*iface)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  return IAVIEditStream_Release(&This->IAVIEditStream_iface);
}

static HRESULT WINAPI IEditAVIStream_fnCreate(IAVIStream*iface,
                                              LPARAM lParam1,LPARAM lParam2)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );

  if (lParam2 != 0)
    return AVIERR_ERROR;

  if (This->pStreams == NULL) {
    This->pStreams = calloc(256, sizeof(EditStreamTable));
    if (This->pStreams == NULL)
      return AVIERR_MEMORY;
    This->nTableSize = 256;
  }

  if (lParam1 != 0) {
    IAVIStream_Info((PAVISTREAM)lParam1, &This->sInfo, sizeof(This->sInfo));
    IAVIStream_AddRef((PAVISTREAM)lParam1);
    This->pStreams[0].pStream  = (PAVISTREAM)lParam1;
    This->pStreams[0].dwStart  = This->sInfo.dwStart;
    This->pStreams[0].dwLength = This->sInfo.dwLength;
    This->nStreams = 1;
  }
  return AVIERR_OK;
}

static HRESULT WINAPI IEditAVIStream_fnInfo(IAVIStream*iface,
                                            AVISTREAMINFOW *psi,LONG size)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );

  TRACE("(%p,%p,%ld)\n",iface,psi,size);

  if (psi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  if (This->bDecompress)
    This->sInfo.fccHandler = 0;

  memcpy(psi, &This->sInfo, min((DWORD)size, sizeof(This->sInfo)));

  if ((DWORD)size < sizeof(This->sInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static LONG    WINAPI IEditAVIStream_fnFindSample(IAVIStream*iface,LONG pos,
                                                  LONG flags)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  PAVISTREAM stream;
  DWORD      streamPos, streamNr;

  TRACE("(%p,%ld,0x%08lX)\n",iface,pos,flags);

  if (flags & FIND_FROM_START)
    pos = (LONG)This->sInfo.dwStart;

  /* outside of stream? */
  if (pos < (LONG)This->sInfo.dwStart ||
      (LONG)This->sInfo.dwStart + (LONG)This->sInfo.dwLength <= pos)
    return -1;

  /* map our position to a stream and position in it */
  if (AVIFILE_FindStreamInTable(This, pos, &stream, &streamPos,
                                &streamNr, TRUE) != S_OK)
    return -1; /* doesn't exist */

  if (This->bDecompress) {
    /* only one stream -- format changes only at start */
    if (flags & FIND_FORMAT)
      return (flags & FIND_NEXT ? -1 : 0);

    /* FIXME: map positions back to us */
    return IAVIStream_FindSample(stream, streamPos, flags);
  } else {
    /* assume change of format every frame */
    return pos;
  }
}

static HRESULT WINAPI IEditAVIStream_fnReadFormat(IAVIStream*iface,LONG pos,
                                                  LPVOID format,LONG*fmtsize)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  LPBITMAPINFOHEADER  lp;
  PAVISTREAM          stream;
  DWORD               n;
  HRESULT             hr;

  TRACE("(%p,%ld,%p,%p)\n",iface,pos,format,fmtsize);

  if (fmtsize == NULL || pos < This->sInfo.dwStart ||
      This->sInfo.dwStart + This->sInfo.dwLength <= pos)
    return AVIERR_BADPARAM;

  /* find stream corresponding to position */
  hr = AVIFILE_FindStreamInTable(This, pos, &stream, &n, NULL, FALSE);
  if (FAILED(hr))
    return hr;

  if (! This->bDecompress)
    return IAVIStream_ReadFormat(stream, n, format, fmtsize);

  lp = AVIFILE_ReadFrame(This, stream, n);
  if (lp == NULL)
    return AVIERR_ERROR;
  if (lp->biBitCount <= 8) {
    n  = (lp->biClrUsed > 0 ? lp->biClrUsed : 1 << lp->biBitCount);
    n *= sizeof(RGBQUAD);
  } else
    n = 0;
  n += lp->biSize;
  
  memcpy(format, lp, min((LONG)n, *fmtsize));
  hr = ((LONG)n > *fmtsize ? AVIERR_BUFFERTOOSMALL : AVIERR_OK);
  *fmtsize = n;

  return hr;
}

static HRESULT WINAPI IEditAVIStream_fnSetFormat(IAVIStream*iface,LONG pos,
                                                 LPVOID format,LONG formatsize)
{
  TRACE("(%p,%ld,%p,%ld)\n",iface,pos,format,formatsize);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IEditAVIStream_fnRead(IAVIStream*iface,LONG start,
                                            LONG samples,LPVOID buffer,
                                            LONG buffersize,LONG*bytesread,
                                            LONG*samplesread)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  PAVISTREAM stream;
  DWORD   streamPos, streamNr;
  LONG    readBytes, readSamples, count;
  HRESULT hr;

  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p) -- 0x%08lX\n",iface,start,samples,
        buffer,buffersize,bytesread,samplesread,This->sInfo.fccType);

  /* check parameters */
  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;
  if (buffersize < 0)
    return AVIERR_BADSIZE;
  if ((DWORD)start < This->sInfo.dwStart ||
      This->sInfo.dwStart + This->sInfo.dwLength < (DWORD)start)
    return AVIERR_BADPARAM;

  if (! This->bDecompress) {
    /* audio like data -- sample-based */
    do {
      if (samples == 0)
        return AVIERR_OK; /* nothing at all or already done */

      if (FAILED(AVIFILE_FindStreamInTable(This, start, &stream,
                                           &streamPos, &streamNr, FALSE)))
        return AVIERR_ERROR;

      /* limit to end of the stream */
      count = samples;
      if (streamPos + count > EditStreamEnd(This, streamNr))
        count = EditStreamEnd(This, streamNr) - streamPos;

      hr = IAVIStream_Read(stream, streamPos, count, buffer, buffersize,
                           &readBytes, &readSamples);
      if (FAILED(hr))
        return hr;
      if (readBytes == 0 && readSamples == 0 && count != 0)
        return AVIERR_FILEREAD; /* for bad stream implementations */

      if (samplesread != NULL)
        *samplesread += readSamples;
      if (bytesread != NULL)
        *bytesread += readBytes;
      if (buffer != NULL) {
        buffer = ((LPBYTE)buffer)+readBytes;
        buffersize     -= readBytes;
      }
      start   += count;
      samples -= count;
    } while (This->sInfo.dwStart + This->sInfo.dwLength > start);
  } else {
    /* video like data -- frame-based */
    LPBITMAPINFOHEADER lp;

    if (samples == 0)
      return AVIERR_OK;

    if (FAILED(AVIFILE_FindStreamInTable(This, start, &stream,
                                         &streamPos, &streamNr, FALSE)))
      return AVIERR_ERROR;

    lp = AVIFILE_ReadFrame(This, stream, streamPos);
    if (lp == NULL)
      return AVIERR_ERROR;

    if (buffer != NULL) {
      /* need size of format to skip */
      if (lp->biBitCount <= 8) {
        count  = lp->biClrUsed > 0 ? lp->biClrUsed : 1 << lp->biBitCount;
        count *= sizeof(RGBQUAD);
      } else
        count = 0;
      count += lp->biSize;

      if (buffersize < lp->biSizeImage)
        return AVIERR_BUFFERTOOSMALL;
      memcpy(buffer, (LPBYTE)lp + count, lp->biSizeImage);
    }

    if (bytesread != NULL)
      *bytesread = lp->biSizeImage;
    if (samplesread != NULL)
      *samplesread = 1;
  }

  return AVIERR_OK;
}

static HRESULT WINAPI IEditAVIStream_fnWrite(IAVIStream*iface,LONG start,
                                             LONG samples,LPVOID buffer,
                                             LONG buffersize,DWORD flags,
                                             LONG*sampwritten,LONG*byteswritten)
{
  TRACE("(%p,%ld,%ld,%p,%ld,0x%08lX,%p,%p)\n",iface,start,samples,buffer,
        buffersize,flags,sampwritten,byteswritten);

  /* be sure return parameters have correct values */
  if (sampwritten != NULL)
    *sampwritten = 0;
  if (byteswritten != NULL)
    *byteswritten = 0;

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IEditAVIStream_fnDelete(IAVIStream*iface,LONG start,
                                              LONG samples)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );

  TRACE("(%p,%ld,%ld)\n",iface,start,samples);

  return IAVIEditStream_Cut(&This->IAVIEditStream_iface,&start,&samples,NULL);
}

static HRESULT WINAPI IEditAVIStream_fnReadData(IAVIStream*iface,DWORD fcc,
                                                LPVOID lp,LONG *lpread)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );
  DWORD n;

  TRACE("(%p,0x%08lX,%p,%p)\n",iface,fcc,lp,lpread);

  /* check parameters */
  if (lp == NULL || lpread == NULL)
    return AVIERR_BADPARAM;

  /* simply ask every stream and return the first block found */
  for (n = 0; n < This->nStreams; n++) {
    HRESULT hr = IAVIStream_ReadData(This->pStreams[n].pStream,fcc,lp,lpread);

    if (SUCCEEDED(hr))
      return hr;
  }

  *lpread = 0;
  return AVIERR_NODATA;
}

static HRESULT WINAPI IEditAVIStream_fnWriteData(IAVIStream*iface,DWORD fcc,
                                                 LPVOID lp,LONG size)
{
  TRACE("(%p,0x%08lX,%p,%ld)\n",iface,fcc,lp,size);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IEditAVIStream_fnSetInfo(IAVIStream*iface,
                                               AVISTREAMINFOW*info,LONG len)
{
  IAVIEditStreamImpl *This = impl_from_IAVIStream( iface );

  TRACE("(%p,%p,%ld)\n",iface,info,len);

  return IAVIEditStream_SetInfo(&This->IAVIEditStream_iface,info,len);
}

static const struct IAVIStreamVtbl ieditstast = {
  IEditAVIStream_fnQueryInterface,
  IEditAVIStream_fnAddRef,
  IEditAVIStream_fnRelease,
  IEditAVIStream_fnCreate,
  IEditAVIStream_fnInfo,
  IEditAVIStream_fnFindSample,
  IEditAVIStream_fnReadFormat,
  IEditAVIStream_fnSetFormat,
  IEditAVIStream_fnRead,
  IEditAVIStream_fnWrite,
  IEditAVIStream_fnDelete,
  IEditAVIStream_fnReadData,
  IEditAVIStream_fnWriteData,
  IEditAVIStream_fnSetInfo
};

static IAVIEditStreamImpl *AVIFILE_CreateEditStream(IAVIStream *pstream)
{
  IAVIEditStreamImpl *pedit = NULL;

  pedit = calloc(1, sizeof(IAVIEditStreamImpl));
  if (pedit == NULL)
    return NULL;

  pedit->IAVIEditStream_iface.lpVtbl = &ieditstream;
  pedit->IAVIStream_iface.lpVtbl = &ieditstast;
  pedit->ref = 1;

  IAVIStream_Create(&pedit->IAVIStream_iface, (LPARAM)pstream, 0);

  return pedit;
}

/***********************************************************************
 *             CreateEditableStream     (AVIFIL32.@)
 */
HRESULT WINAPI CreateEditableStream(IAVIStream **editable, IAVIStream *src)
{
    IAVIEditStream *edit = NULL;
    IAVIEditStreamImpl *editobj;
    HRESULT hr;

    TRACE("(%p,%p)\n", editable, src);

    if (!editable)
        return AVIERR_BADPARAM;
    *editable = NULL;

    if (src) {
        hr = IAVIStream_QueryInterface(src, &IID_IAVIEditStream, (void**)&edit);
        if (SUCCEEDED(hr) && edit) {
            hr = IAVIEditStream_Clone(edit, editable);
            IAVIEditStream_Release(edit);

            return hr;
        }
    }

    /* Need own implementation of IAVIEditStream */
    editobj = AVIFILE_CreateEditStream(src);
    if (!editobj)
        return AVIERR_MEMORY;
    *editable = &editobj->IAVIStream_iface;

    return S_OK;
}
