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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "vfw.h"
#include "msacm.h"

#include "avifile_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************/

typedef struct _IAVIStreamImpl {
  /* IUnknown stuff */
  IAVIStream      IAVIStream_iface;
  LONG		  ref;

  /* IAVIStream stuff */
  PAVISTREAM      pStream;
  AVISTREAMINFOW  sInfo;

  HACMSTREAM      has;

  LPWAVEFORMATEX  lpInFormat;
  LONG            cbInFormat;

  LPWAVEFORMATEX  lpOutFormat;
  LONG            cbOutFormat;

  ACMSTREAMHEADER acmStreamHdr;
} IAVIStreamImpl;

/***********************************************************************/

#define CONVERT_STREAM_to_THIS(a) do { \
           DWORD __bytes; \
           acmStreamSize(This->has,*(a) * This->lpInFormat->nBlockAlign,\
                         &__bytes, ACM_STREAMSIZEF_SOURCE); \
           *(a) = __bytes / This->lpOutFormat->nBlockAlign; } while(0)

#define CONVERT_THIS_to_STREAM(a) do { \
           DWORD __bytes; \
           acmStreamSize(This->has,*(a) * This->lpOutFormat->nBlockAlign,\
                         &__bytes, ACM_STREAMSIZEF_DESTINATION); \
           *(a) = __bytes / This->lpInFormat->nBlockAlign; } while(0)

static HRESULT AVIFILE_OpenCompressor(IAVIStreamImpl *This)
{
  HRESULT hr;

  /* pre-conditions */
  assert(This != NULL);
  assert(This->pStream != NULL);

  if (This->has != NULL)
    return AVIERR_OK;

  if (This->lpInFormat == NULL) {
    /* decode or encode the data from pStream */
    hr = AVIStreamFormatSize(This->pStream, This->sInfo.dwStart, &This->cbInFormat);
    if (FAILED(hr))
      return hr;
    This->lpInFormat = malloc(This->cbInFormat);
    if (This->lpInFormat == NULL)
      return AVIERR_MEMORY;

    hr = IAVIStream_ReadFormat(This->pStream, This->sInfo.dwStart,
                               This->lpInFormat, &This->cbInFormat);
    if (FAILED(hr))
      return hr;

    if (This->lpOutFormat == NULL) {
      /* we must decode to default format */
      This->cbOutFormat = sizeof(WAVEFORMATEX);
      This->lpOutFormat = malloc(This->cbOutFormat);
      if (This->lpOutFormat == NULL)
        return AVIERR_MEMORY;

      This->lpOutFormat->wFormatTag = WAVE_FORMAT_PCM;
      if (acmFormatSuggest(NULL, This->lpInFormat, This->lpOutFormat,
                           This->cbOutFormat, ACM_FORMATSUGGESTF_WFORMATTAG) != S_OK)
        return AVIERR_NOCOMPRESSOR;
    }
  } else if (This->lpOutFormat == NULL)
    return AVIERR_ERROR; /* To what should I encode? */

  if (acmStreamOpen(&This->has, NULL, This->lpInFormat, This->lpOutFormat,
                    NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME) != S_OK)
    return AVIERR_NOCOMPRESSOR;

  /* update AVISTREAMINFO structure */
  This->sInfo.dwSampleSize = This->lpOutFormat->nBlockAlign;
  This->sInfo.dwScale      = This->lpOutFormat->nBlockAlign;
  This->sInfo.dwRate       = This->lpOutFormat->nAvgBytesPerSec;
  This->sInfo.dwQuality    = (DWORD)ICQUALITY_DEFAULT;
  SetRectEmpty(&This->sInfo.rcFrame);

  /* convert positions and sizes to output format */
  CONVERT_STREAM_to_THIS(&This->sInfo.dwStart);
  CONVERT_STREAM_to_THIS(&This->sInfo.dwLength);
  CONVERT_STREAM_to_THIS(&This->sInfo.dwSuggestedBufferSize);

  return AVIERR_OK;
}

static inline IAVIStreamImpl *impl_from_IAVIStream(IAVIStream *iface)
{
  return CONTAINING_RECORD(iface, IAVIStreamImpl, IAVIStream_iface);
}

static HRESULT WINAPI ACMStream_fnQueryInterface(IAVIStream *iface,
						  REFIID refiid, LPVOID *obj)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%s,%p)\n", iface, debugstr_guid(refiid), obj);

  if (IsEqualGUID(&IID_IUnknown, refiid) ||
      IsEqualGUID(&IID_IAVIStream, refiid)) {
    *obj = &This->IAVIStream_iface;
    IAVIStream_AddRef(iface);

    return S_OK;
  }

  return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI ACMStream_fnAddRef(IAVIStream *iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  /* also add reference to the nested stream */
  if (This->pStream != NULL)
    IAVIStream_AddRef(This->pStream);

  return ref;
}

static ULONG WINAPI ACMStream_fnRelease(IAVIStream* iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  if (ref == 0) {
    /* destruct */
    if (This->has != NULL) {
      if (This->acmStreamHdr.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
	acmStreamUnprepareHeader(This->has, &This->acmStreamHdr, 0);
      acmStreamClose(This->has, 0);
      This->has = NULL;
    }
    free(This->acmStreamHdr.pbSrc);
    This->acmStreamHdr.pbSrc = NULL;
    free(This->acmStreamHdr.pbDst);
    This->acmStreamHdr.pbDst = NULL;
    if (This->lpInFormat != NULL) {
      free(This->lpInFormat);
      This->lpInFormat = NULL;
      This->cbInFormat = 0;
    }
    if (This->lpOutFormat != NULL) {
      free(This->lpOutFormat);
      This->lpOutFormat = NULL;
      This->cbOutFormat = 0;
    }
    if (This->pStream != NULL) {
      IAVIStream_Release(This->pStream);
      This->pStream = NULL;
    }
    free(This);

    return 0;
  }

  /* also release reference to the nested stream */
  if (This->pStream != NULL)
    IAVIStream_Release(This->pStream);

  return ref;
}

/* lParam1: PAVISTREAM
 * lParam2: LPAVICOMPRESSOPTIONS -- even if doc's say LPWAVEFORMAT
 */
static HRESULT WINAPI ACMStream_fnCreate(IAVIStream *iface, LPARAM lParam1,
					  LPARAM lParam2)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08IX,0x%08IX)\n", iface, lParam1, lParam2);

  /* check for swapped parameters */
  if ((LPVOID)lParam1 != NULL &&
      ((LPAVICOMPRESSOPTIONS)lParam1)->fccType == streamtypeAUDIO) {
    LPARAM tmp = lParam1;

    lParam1 = lParam2;
    lParam2 = tmp;
  }

  if ((LPVOID)lParam1 == NULL)
    return AVIERR_BADPARAM;

  IAVIStream_Info((PAVISTREAM)lParam1, &This->sInfo, sizeof(This->sInfo));
  if (This->sInfo.fccType != streamtypeAUDIO)
    return AVIERR_ERROR; /* error in registry or AVIMakeCompressedStream */

  This->sInfo.fccHandler = 0; /* be paranoid */

  /* FIXME: check ACM version? Which version does we need? */

  if ((LPVOID)lParam2 != NULL) {
    /* We only need the format from the compress-options */
    if (((LPAVICOMPRESSOPTIONS)lParam2)->fccType == streamtypeAUDIO)
      lParam2 = (LPARAM)((LPAVICOMPRESSOPTIONS)lParam2)->lpFormat;

    if (((LPWAVEFORMATEX)lParam2)->wFormatTag != WAVE_FORMAT_PCM)
      This->cbOutFormat = sizeof(WAVEFORMATEX) + ((LPWAVEFORMATEX)lParam2)->cbSize;
    else
      This->cbOutFormat = sizeof(WAVEFORMATEX);

    This->lpOutFormat = malloc(This->cbOutFormat);
    if (This->lpOutFormat == NULL)
      return AVIERR_MEMORY;

    memcpy(This->lpOutFormat, (LPVOID)lParam2, This->cbOutFormat);
  } else {
    This->lpOutFormat = NULL;
    This->cbOutFormat = 0;
  }

  This->pStream = (PAVISTREAM)lParam1;
  IAVIStream_AddRef(This->pStream);

  return AVIERR_OK;
}

static HRESULT WINAPI ACMStream_fnInfo(IAVIStream *iface,LPAVISTREAMINFOW psi,
					LONG size)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%p,%ld)\n", iface, psi, size);

  if (psi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  /* Need codec to correct some values in structure */
  if (This->has == NULL) {
    HRESULT hr = AVIFILE_OpenCompressor(This);

    if (FAILED(hr))
      return hr;
  }

  memcpy(psi, &This->sInfo, min(size, (LONG)sizeof(This->sInfo)));

  if (size < (LONG)sizeof(This->sInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static LONG WINAPI ACMStream_fnFindSample(IAVIStream *iface, LONG pos,
					   LONG flags)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,0x%08lX)\n",iface,pos,flags);

  if (flags & FIND_FROM_START) {
    pos = This->sInfo.dwStart;
    flags &= ~(FIND_FROM_START|FIND_PREV);
    flags |= FIND_NEXT;
  }

  /* convert pos from our 'space' to This->pStream's one */
  CONVERT_THIS_to_STREAM(&pos);

  /* ask stream */
  pos = IAVIStream_FindSample(This->pStream, pos, flags);

  if (pos != -1) {
    /* convert pos back to our 'space' if it's no size or physical pos */
    if ((flags & FIND_RET) == 0)
      CONVERT_STREAM_to_THIS(&pos);
  }

  return pos;
}

static HRESULT WINAPI ACMStream_fnReadFormat(IAVIStream *iface, LONG pos,
					      LPVOID format, LONG *formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%p,%p)\n", iface, pos, format, formatsize);

  if (formatsize == NULL)
    return AVIERR_BADPARAM;

  if (This->has == NULL) {
    HRESULT hr = AVIFILE_OpenCompressor(This);

    if (FAILED(hr))
      return hr;
  }

  /* only interested in needed buffersize? */
  if (format == NULL || *formatsize <= 0) {
    *formatsize = This->cbOutFormat;

    return AVIERR_OK;
  }

  /* copy initial format (only as much as will fit) */
  memcpy(format, This->lpOutFormat, min(*formatsize, This->cbOutFormat));
  if (*formatsize < This->cbOutFormat) {
    *formatsize = This->cbOutFormat;
    return AVIERR_BUFFERTOOSMALL;
  }

  *formatsize = This->cbOutFormat;
  return AVIERR_OK;
}

static HRESULT WINAPI ACMStream_fnSetFormat(IAVIStream *iface, LONG pos,
					     LPVOID format, LONG formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  HRESULT hr;

  TRACE("(%p,%ld,%p,%ld)\n", iface, pos, format, formatsize);

  /* check parameters */
  if (format == NULL || formatsize <= 0)
    return AVIERR_BADPARAM;

  /* Input format already known?
   * Changing is unsupported, but be quiet if it's the same */
  if (This->lpInFormat != NULL) {
    if (This->cbInFormat != formatsize ||
	memcmp(format, This->lpInFormat, formatsize) != 0)
      return AVIERR_UNSUPPORTED;

    return AVIERR_OK;
  }

  /* Does the nested stream support writing? */
  if ((This->sInfo.dwCaps & AVIFILECAPS_CANWRITE) == 0)
    return AVIERR_READONLY;

  This->lpInFormat = malloc(formatsize);
  if (This->lpInFormat == NULL)
    return AVIERR_MEMORY;
  This->cbInFormat = formatsize;
  memcpy(This->lpInFormat, format, formatsize);

  /* initialize formats and get compressor */
  hr = AVIFILE_OpenCompressor(This);
  if (FAILED(hr))
    return hr;

  CONVERT_THIS_to_STREAM(&pos);

  /* tell the nested stream the new format */
  return IAVIStream_SetFormat(This->pStream, pos, This->lpOutFormat,
			      This->cbOutFormat);
}

static HRESULT WINAPI ACMStream_fnRead(IAVIStream *iface, LONG start,
					LONG samples, LPVOID buffer,
					LONG buffersize, LPLONG bytesread,
					LPLONG samplesread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  HRESULT hr;
  DWORD   size;

  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", iface, start, samples, buffer,
 	buffersize, bytesread, samplesread);

  /* clear return parameters if given */
  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;

  /* Do we have our compressor? */
  if (This->has == NULL) {
    hr = AVIFILE_OpenCompressor(This);

    if (FAILED(hr))
      return hr;
  }

  /* only need to pass through? */
  if (This->cbInFormat == This->cbOutFormat &&
      memcmp(This->lpInFormat, This->lpOutFormat, This->cbInFormat) == 0) {
    return IAVIStream_Read(This->pStream, start, samples, buffer, buffersize,
			   bytesread, samplesread);
  }

  /* read as much as fit? */
  if (samples == -1)
    samples = buffersize / This->lpOutFormat->nBlockAlign;
  /* limit to buffersize */
  if (samples * This->lpOutFormat->nBlockAlign > buffersize)
    samples = buffersize / This->lpOutFormat->nBlockAlign;

  /* only return needed size? */
  if (buffer == NULL || buffersize <= 0 || samples == 0) {
    if (bytesread == NULL && samplesread == NULL)
      return AVIERR_BADPARAM;

    if (bytesread != NULL)
      *bytesread = samples * This->lpOutFormat->nBlockAlign;
    if (samplesread != NULL)
      *samplesread = samples;

    return AVIERR_OK;
  }

  /* map our positions to pStream positions */
  CONVERT_THIS_to_STREAM(&start);

  /* our needed internal buffersize */
  size = samples * This->lpInFormat->nBlockAlign;

  /* Need to free destination buffer used for writing? */
  if (This->acmStreamHdr.pbDst != NULL) {
    free(This->acmStreamHdr.pbDst);
    This->acmStreamHdr.pbDst     = NULL;
    This->acmStreamHdr.dwDstUser = 0;
  }

  /* need bigger source buffer? */
  if (This->acmStreamHdr.pbSrc == NULL ||
      This->acmStreamHdr.dwSrcUser < size) {
    This->acmStreamHdr.pbSrc = realloc(This->acmStreamHdr.pbSrc, size);
    if (This->acmStreamHdr.pbSrc == NULL)
      return AVIERR_MEMORY;
    This->acmStreamHdr.dwSrcUser = size;
  }

  This->acmStreamHdr.cbStruct = sizeof(This->acmStreamHdr);
  This->acmStreamHdr.cbSrcLengthUsed = 0;
  This->acmStreamHdr.cbDstLengthUsed = 0;
  This->acmStreamHdr.cbSrcLength     = size;

  /* read source data */
  hr = IAVIStream_Read(This->pStream, start, -1, This->acmStreamHdr.pbSrc,
		       This->acmStreamHdr.cbSrcLength,
		       (LONG *)&This->acmStreamHdr.cbSrcLength, NULL);
  if (FAILED(hr) || This->acmStreamHdr.cbSrcLength == 0)
    return hr;

  /* need to prepare stream? */
  This->acmStreamHdr.pbDst       = buffer;
  This->acmStreamHdr.cbDstLength = buffersize;
  if ((This->acmStreamHdr.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED) == 0) {
    if (acmStreamPrepareHeader(This->has, &This->acmStreamHdr, 0) != S_OK) {
      This->acmStreamHdr.pbDst       = NULL;
      This->acmStreamHdr.cbDstLength = 0;
      return AVIERR_COMPRESSOR;
    }
  }

  /* now do the conversion */
  /* FIXME: use ACM_CONVERTF_* flags */
  if (acmStreamConvert(This->has, &This->acmStreamHdr, 0) != S_OK)
    hr = AVIERR_COMPRESSOR;

  This->acmStreamHdr.pbDst       = NULL;
  This->acmStreamHdr.cbDstLength = 0;

  /* fill out return parameters if given */
  if (bytesread != NULL)
    *bytesread = This->acmStreamHdr.cbDstLengthUsed;
  if (samplesread != NULL)
    *samplesread =
      This->acmStreamHdr.cbDstLengthUsed / This->lpOutFormat->nBlockAlign;

  return hr;
}

static HRESULT WINAPI ACMStream_fnWrite(IAVIStream *iface, LONG start,
					 LONG samples, LPVOID buffer,
					 LONG buffersize, DWORD flags,
					 LPLONG sampwritten,
					 LPLONG byteswritten)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  HRESULT hr;
  ULONG   size;

  TRACE("(%p,%ld,%ld,%p,%ld,0x%08lX,%p,%p)\n", iface, start, samples,
	buffer, buffersize, flags, sampwritten, byteswritten);

  /* clear return parameters if given */
  if (sampwritten != NULL)
    *sampwritten = 0;
  if (byteswritten != NULL)
    *byteswritten = 0;

  /* check parameters */
  if (buffer == NULL && (buffersize > 0 || samples > 0))
    return AVIERR_BADPARAM;

  /* Have we write capability? */
  if ((This->sInfo.dwCaps & AVIFILECAPS_CANWRITE) == 0)
    return AVIERR_READONLY;

  /* also need a compressor */
  if (This->has == NULL)
    return AVIERR_NOCOMPRESSOR;

  /* map our sizes to pStream sizes */
  size = buffersize;
  CONVERT_THIS_to_STREAM(&size);
  CONVERT_THIS_to_STREAM(&start);

  /* no bytes to write? -- short circuit */
  if (size == 0) {
    return IAVIStream_Write(This->pStream, -1, samples, buffer, size,
			    flags, sampwritten, byteswritten);
  }

  /* Need to free source buffer used for reading? */
  if (This->acmStreamHdr.pbSrc != NULL) {
    free(This->acmStreamHdr.pbSrc);
    This->acmStreamHdr.pbSrc     = NULL;
    This->acmStreamHdr.dwSrcUser = 0;
  }

  /* Need bigger destination buffer? */
  if (This->acmStreamHdr.pbDst == NULL ||
      This->acmStreamHdr.dwDstUser < size) {
    This->acmStreamHdr.pbDst = realloc(This->acmStreamHdr.pbDst, size);
    if (This->acmStreamHdr.pbDst == NULL)
      return AVIERR_MEMORY;
    This->acmStreamHdr.dwDstUser = size;
  }
  This->acmStreamHdr.cbStruct        = sizeof(This->acmStreamHdr);
  This->acmStreamHdr.cbSrcLengthUsed = 0;
  This->acmStreamHdr.cbDstLengthUsed = 0;
  This->acmStreamHdr.cbDstLength     = This->acmStreamHdr.dwDstUser;

  /* need to prepare stream? */
  This->acmStreamHdr.pbSrc       = buffer;
  This->acmStreamHdr.cbSrcLength = buffersize;
  if ((This->acmStreamHdr.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED) == 0) {
    if (acmStreamPrepareHeader(This->has, &This->acmStreamHdr, 0) != S_OK) {
      This->acmStreamHdr.pbSrc       = NULL;
      This->acmStreamHdr.cbSrcLength = 0;
      return AVIERR_COMPRESSOR;
    }
  }

  /* now do the conversion */
  /* FIXME: use ACM_CONVERTF_* flags */
  if (acmStreamConvert(This->has, &This->acmStreamHdr, 0) != S_OK)
    hr = AVIERR_COMPRESSOR;
  else
    hr = AVIERR_OK;

  This->acmStreamHdr.pbSrc       = NULL;
  This->acmStreamHdr.cbSrcLength = 0;

  if (FAILED(hr))
    return hr;

  return IAVIStream_Write(This->pStream,-1,This->acmStreamHdr.cbDstLengthUsed /
			  This->lpOutFormat->nBlockAlign,This->acmStreamHdr.pbDst,
			  This->acmStreamHdr.cbDstLengthUsed,flags,sampwritten,
			  byteswritten);
}

static HRESULT WINAPI ACMStream_fnDelete(IAVIStream *iface, LONG start,
					  LONG samples)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%ld)\n", iface, start, samples);

  /* check parameters */
  if (start < 0 || samples < 0)
    return AVIERR_BADPARAM;

  /* Delete before start of stream? */
  if ((DWORD)(start + samples) < This->sInfo.dwStart)
    return AVIERR_OK;

  /* Delete after end of stream? */
  if ((DWORD)start > This->sInfo.dwLength)
    return AVIERR_OK;

  /* For the rest we need write capability */
  if ((This->sInfo.dwCaps & AVIFILECAPS_CANWRITE) == 0)
    return AVIERR_READONLY;

  /* A compressor is also necessary */
  if (This->has == NULL)
    return AVIERR_NOCOMPRESSOR;

  /* map our positions to pStream positions */
  CONVERT_THIS_to_STREAM(&start);
  CONVERT_THIS_to_STREAM(&samples);

  return IAVIStream_Delete(This->pStream, start, samples);
}

static HRESULT WINAPI ACMStream_fnReadData(IAVIStream *iface, DWORD fcc,
					    LPVOID lp, LPLONG lpread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lX,%p,%p)\n", iface, fcc, lp, lpread);

  assert(This->pStream != NULL);

  return IAVIStream_ReadData(This->pStream, fcc, lp, lpread);
}

static HRESULT WINAPI ACMStream_fnWriteData(IAVIStream *iface, DWORD fcc,
					     LPVOID lp, LONG size)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lx,%p,%ld)\n", iface, fcc, lp, size);

  assert(This->pStream != NULL);

  return IAVIStream_WriteData(This->pStream, fcc, lp, size);
}

static HRESULT WINAPI ACMStream_fnSetInfo(IAVIStream *iface,
					   LPAVISTREAMINFOW info, LONG infolen)
{
  FIXME("(%p,%p,%ld): stub\n", iface, info, infolen);

  return E_FAIL;
}

static const struct IAVIStreamVtbl iacmst = {
  ACMStream_fnQueryInterface,
  ACMStream_fnAddRef,
  ACMStream_fnRelease,
  ACMStream_fnCreate,
  ACMStream_fnInfo,
  ACMStream_fnFindSample,
  ACMStream_fnReadFormat,
  ACMStream_fnSetFormat,
  ACMStream_fnRead,
  ACMStream_fnWrite,
  ACMStream_fnDelete,
  ACMStream_fnReadData,
  ACMStream_fnWriteData,
  ACMStream_fnSetInfo
};

HRESULT AVIFILE_CreateACMStream(REFIID riid, LPVOID *ppv)
{
  IAVIStreamImpl *pstream;
  HRESULT         hr;

  assert(riid != NULL && ppv != NULL);

  *ppv = NULL;

  pstream = calloc(1, sizeof(IAVIStreamImpl));
  if (pstream == NULL)
    return AVIERR_MEMORY;

  pstream->IAVIStream_iface.lpVtbl = &iacmst;

  hr = IAVIStream_QueryInterface(&pstream->IAVIStream_iface, riid, ppv);
  if (FAILED(hr))
    free(pstream);

  return hr;
}
