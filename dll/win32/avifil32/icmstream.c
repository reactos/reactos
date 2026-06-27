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

#include "avifile_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

#define MAX_FRAMESIZE       (16 * 1024 * 1024)
#define MAX_FRAMESIZE_DIFF  512

/***********************************************************************/

typedef struct _IAVIStreamImpl {
  /* IUnknown stuff */
  IAVIStream         IAVIStream_iface;
  LONG               ref;

  /* IAVIStream stuff */
  PAVISTREAM         pStream;
  AVISTREAMINFOW     sInfo;

  PGETFRAME          pg;
  HIC                hic;
  DWORD              dwICMFlags;

  LONG               lCurrent;
  LONG               lLastKey;
  LONG               lKeyFrameEvery;
  DWORD              dwLastQuality;
  DWORD              dwBytesPerFrame;
  DWORD              dwUnusedBytes;

  LPBITMAPINFOHEADER lpbiCur;  /* current frame */
  LPVOID             lpCur;
  LPBITMAPINFOHEADER lpbiPrev; /* previous frame */
  LPVOID             lpPrev;

  LPBITMAPINFOHEADER lpbiOutput; /* output format of codec */
  LONG               cbOutput;
  LPBITMAPINFOHEADER lpbiInput;  /* input format for codec */
  LONG               cbInput;
} IAVIStreamImpl;

/***********************************************************************/

static HRESULT AVIFILE_EncodeFrame(IAVIStreamImpl *This,
				   LPBITMAPINFOHEADER lpbi, LPVOID lpBits);
static HRESULT AVIFILE_OpenGetFrame(IAVIStreamImpl *This);

static inline IAVIStreamImpl *impl_from_IAVIStream(IAVIStream *iface)
{
  return CONTAINING_RECORD(iface, IAVIStreamImpl, IAVIStream_iface);
}

static inline void AVIFILE_Reset(IAVIStreamImpl *This)
{
  This->lCurrent      = -1;
  This->lLastKey      = 0;
  This->dwLastQuality = ICQUALITY_HIGH;
  This->dwUnusedBytes = 0;
}

static HRESULT WINAPI ICMStream_fnQueryInterface(IAVIStream *iface,
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

static ULONG WINAPI ICMStream_fnAddRef(IAVIStream *iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  /* also add reference to the nested stream */
  if (This->pStream != NULL)
    IAVIStream_AddRef(This->pStream);

  return ref;
}

static ULONG WINAPI ICMStream_fnRelease(IAVIStream* iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) -> %ld\n", iface, ref);

  if (ref == 0) {
    /* destruct */
    if (This->pg != NULL) {
      AVIStreamGetFrameClose(This->pg);
      This->pg = NULL;
    }
    if (This->pStream != NULL) {
      IAVIStream_Release(This->pStream);
      This->pStream = NULL;
    }
    if (This->hic != NULL) {
      if (This->lpbiPrev != NULL) {
	ICDecompressEnd(This->hic);
	free(This->lpbiPrev);
	This->lpbiPrev = NULL;
	This->lpPrev   = NULL;
      }
      ICCompressEnd(This->hic);
      This->hic = NULL;
    }
    if (This->lpbiCur != NULL) {
      free(This->lpbiCur);
      This->lpbiCur = NULL;
      This->lpCur   = NULL;
    }
    if (This->lpbiOutput != NULL) {
      free(This->lpbiOutput);
      This->lpbiOutput = NULL;
      This->cbOutput   = 0;
    }
    if (This->lpbiInput != NULL) {
      free(This->lpbiInput);
      This->lpbiInput = NULL;
      This->cbInput   = 0;
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
 * lParam2: LPAVICOMPRESSOPTIONS
 */
static HRESULT WINAPI ICMStream_fnCreate(IAVIStream *iface, LPARAM lParam1,
					  LPARAM lParam2)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  ICINFO               icinfo;
  ICCOMPRESSFRAMES     icFrames;
  LPAVICOMPRESSOPTIONS pco = (LPAVICOMPRESSOPTIONS)lParam2;

  TRACE("(%p,0x%08IX,0x%08IX)\n", iface, lParam1, lParam2);

  /* check parameter */
  if ((LPVOID)lParam1 == NULL)
    return AVIERR_BADPARAM;

  /* get infos from stream */
  IAVIStream_Info((PAVISTREAM)lParam1, &This->sInfo, sizeof(This->sInfo));
  if (This->sInfo.fccType != streamtypeVIDEO)
    return AVIERR_ERROR; /* error in registry or AVIMakeCompressedStream */

  /* add reference to the stream */
  This->pStream = (PAVISTREAM)lParam1;
  IAVIStream_AddRef(This->pStream);

  AVIFILE_Reset(This);

  if (pco != NULL && pco->fccHandler != comptypeDIB) {
    /* we should compress */
    This->sInfo.fccHandler = pco->fccHandler;

    This->hic = ICOpen(ICTYPE_VIDEO, pco->fccHandler, ICMODE_COMPRESS);
    if (This->hic == NULL)
      return AVIERR_NOCOMPRESSOR;

    /* restore saved state of codec */
    if (pco->cbParms > 0 && pco->lpParms != NULL) {
      ICSetState(This->hic, pco->lpParms, pco->cbParms);
    }

    /* set quality -- resolve default quality */
    This->sInfo.dwQuality = pco->dwQuality;
    if (pco->dwQuality == ICQUALITY_DEFAULT)
      This->sInfo.dwQuality = ICGetDefaultQuality(This->hic);

    /* get capabilities of codec */
    ICGetInfo(This->hic, &icinfo, sizeof(icinfo));
    This->dwICMFlags = icinfo.dwFlags;

    /* use keyframes? */
    if ((pco->dwFlags & AVICOMPRESSF_KEYFRAMES) &&
	(icinfo.dwFlags & (VIDCF_TEMPORAL|VIDCF_FASTTEMPORALC))) {
      This->lKeyFrameEvery = pco->dwKeyFrameEvery;
    } else
      This->lKeyFrameEvery = 1;

    /* use datarate? */
    if ((pco->dwFlags & AVICOMPRESSF_DATARATE)) {
      /* Do we have a chance to reduce size to desired one? */
      if ((icinfo.dwFlags & (VIDCF_CRUNCH|VIDCF_QUALITY)) == 0)
	return AVIERR_NOCOMPRESSOR;

      assert(This->sInfo.dwRate != 0);

      This->dwBytesPerFrame = MulDiv(pco->dwBytesPerSecond,
				     This->sInfo.dwScale, This->sInfo.dwRate);
    } else {
      pco->dwBytesPerSecond = 0;
      This->dwBytesPerFrame = 0;
    }

    if (icinfo.dwFlags & VIDCF_COMPRESSFRAMES) {
      memset(&icFrames, 0, sizeof(icFrames));
      icFrames.lpbiOutput  = This->lpbiOutput;
      icFrames.lpbiInput   = This->lpbiInput;
      icFrames.lFrameCount = This->sInfo.dwLength;
      icFrames.lQuality    = This->sInfo.dwQuality;
      icFrames.lDataRate   = pco->dwBytesPerSecond;
      icFrames.lKeyRate    = This->lKeyFrameEvery;
      icFrames.dwRate      = This->sInfo.dwRate;
      icFrames.dwScale     = This->sInfo.dwScale;
      ICSendMessage(This->hic, ICM_COMPRESS_FRAMES_INFO,
		    (LPARAM)&icFrames, (LPARAM)sizeof(icFrames));
    }
  } else
    This->sInfo.fccHandler = comptypeDIB;

  return AVIERR_OK;
}

static HRESULT WINAPI ICMStream_fnInfo(IAVIStream *iface,LPAVISTREAMINFOW psi,
					LONG size)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%p,%ld)\n", iface, psi, size);

  if (psi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  memcpy(psi, &This->sInfo, min((DWORD)size, sizeof(This->sInfo)));

  if ((DWORD)size < sizeof(This->sInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static LONG WINAPI ICMStream_fnFindSample(IAVIStream *iface, LONG pos,
					   LONG flags)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,0x%08lX)\n",iface,pos,flags);

  if (flags & FIND_FROM_START) {
    pos = This->sInfo.dwStart;
    flags &= ~(FIND_FROM_START|FIND_PREV);
    flags |= FIND_NEXT;
  }

  if (flags & FIND_RET)
    WARN(": FIND_RET flags will be ignored!\n");

  if (flags & FIND_KEY) {
    if (This->hic == NULL)
      return pos; /* we decompress so every frame is a keyframe */

    if (flags & FIND_PREV) {
      /* need to read old or new frames? */
      if (This->lLastKey <= pos || pos < This->lCurrent)
	IAVIStream_Read(iface, pos, 1, NULL, 0, NULL, NULL);

      return This->lLastKey;
    }
  } else if (flags & FIND_ANY) {
    return pos; /* We really don't know, reread is too expensive, so guess. */
  } else if (flags & FIND_FORMAT) {
    if (flags & FIND_PREV)
      return 0;
  }

  return -1;
}

static HRESULT WINAPI ICMStream_fnReadFormat(IAVIStream *iface, LONG pos,
					      LPVOID format, LONG *formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  LPBITMAPINFOHEADER lpbi;
  HRESULT            hr;

  TRACE("(%p,%ld,%p,%p)\n", iface, pos, format, formatsize);

  if (formatsize == NULL)
    return AVIERR_BADPARAM;

  if (This->pg == NULL) {
    hr = AVIFILE_OpenGetFrame(This);

    if (FAILED(hr))
      return hr;
  }

  lpbi = AVIStreamGetFrame(This->pg, pos);
  if (lpbi == NULL)
    return AVIERR_MEMORY;

  if (This->hic == NULL) {
    LONG size = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

    if (size > 0) {
      if (This->sInfo.dwSuggestedBufferSize < lpbi->biSizeImage)
	This->sInfo.dwSuggestedBufferSize = lpbi->biSizeImage;

      This->cbOutput = size;
      if (format != NULL) {
	if (This->lpbiOutput != NULL)
	  memcpy(format, This->lpbiOutput, min(*formatsize, This->cbOutput));
	else
	  memcpy(format, lpbi, min(*formatsize, size));
      }
    }
  } else if (format != NULL)
    memcpy(format, This->lpbiOutput, min(*formatsize, This->cbOutput));

  if (*formatsize < This->cbOutput)
    hr = AVIERR_BUFFERTOOSMALL;
  else
    hr = AVIERR_OK;

  *formatsize = This->cbOutput;
  return hr;
}

static HRESULT WINAPI ICMStream_fnSetFormat(IAVIStream *iface, LONG pos,
					     LPVOID format, LONG formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%p,%ld)\n", iface, pos, format, formatsize);

  /* check parameters */
  if (format == NULL || formatsize <= 0)
    return AVIERR_BADPARAM;

  /* We can only accept RGB data for writing */
  if (((LPBITMAPINFOHEADER)format)->biCompression != BI_RGB) {
    WARN(": need RGB data as input\n");
    return AVIERR_UNSUPPORTED;
  }

  /* Input format already known?
   * Changing of palette is supported, but be quiet if it's the same */
  if (This->lpbiInput != NULL) {
    if (This->cbInput != formatsize)
      return AVIERR_UNSUPPORTED;

    if (memcmp(format, This->lpbiInput, formatsize) == 0)
      return AVIERR_OK;
  }

  /* Does the nested stream support writing? */
  if ((This->sInfo.dwCaps & AVIFILECAPS_CANWRITE) == 0)
    return AVIERR_READONLY;

  /* check if frame is already written */
  if (This->sInfo.dwLength + This->sInfo.dwStart > pos)
    return AVIERR_UNSUPPORTED;

  /* check if we should compress */
  if (This->sInfo.fccHandler == 0 ||
      This->sInfo.fccHandler == mmioFOURCC('N','O','N','E'))
    This->sInfo.fccHandler = comptypeDIB;

  /* only pass through? */
  if (This->sInfo.fccHandler == comptypeDIB)
    return IAVIStream_SetFormat(This->pStream, pos, format, formatsize);

  /* initial format setting? */
  if (This->lpbiInput == NULL) {
    ULONG size;

    assert(This->hic != NULL);

    /* get memory for input format */
    This->lpbiInput = malloc(formatsize);
    if (This->lpbiInput == NULL)
      return AVIERR_MEMORY;
    This->cbInput = formatsize;
    memcpy(This->lpbiInput, format, formatsize);

    /* get output format */
    size = ICCompressGetFormatSize(This->hic, This->lpbiInput);
    if (size < sizeof(BITMAPINFOHEADER))
      return AVIERR_COMPRESSOR;
    This->lpbiOutput = malloc(size);
    if (This->lpbiOutput == NULL)
      return AVIERR_MEMORY;
    This->cbOutput = size;
    if (ICCompressGetFormat(This->hic,This->lpbiInput,This->lpbiOutput) < S_OK)
      return AVIERR_COMPRESSOR;

    /* update AVISTREAMINFO structure */
    This->sInfo.rcFrame.right  =
      This->sInfo.rcFrame.left + This->lpbiOutput->biWidth;
    This->sInfo.rcFrame.bottom =
      This->sInfo.rcFrame.top  + This->lpbiOutput->biHeight;

    /* prepare codec for compression */
    if (ICCompressBegin(This->hic, This->lpbiInput, This->lpbiOutput) != S_OK)
      return AVIERR_COMPRESSOR;

    /* allocate memory for compressed frame */
    size = ICCompressGetSize(This->hic, This->lpbiInput, This->lpbiOutput);
    This->lpbiCur = malloc(This->cbOutput + size);
    if (This->lpbiCur == NULL)
      return AVIERR_MEMORY;
    memcpy(This->lpbiCur, This->lpbiOutput, This->cbOutput);
    This->lpCur = DIBPTR(This->lpbiCur);

    /* allocate memory for last frame if needed */
    if (This->lKeyFrameEvery != 1 &&
	(This->dwICMFlags & VIDCF_FASTTEMPORALC) == 0) {
      size = ICDecompressGetFormatSize(This->hic, This->lpbiOutput);
      This->lpbiPrev = malloc(size);
      if (This->lpbiPrev == NULL)
	return AVIERR_MEMORY;
      if (ICDecompressGetFormat(This->hic, This->lpbiOutput, This->lpbiPrev) < S_OK)
	return AVIERR_COMPRESSOR;

      if (This->lpbiPrev->biSizeImage == 0) {
	This->lpbiPrev->biSizeImage =
	  DIBWIDTHBYTES(*This->lpbiPrev) * This->lpbiPrev->biHeight;
      }

      /* get memory for format and picture */
      size += This->lpbiPrev->biSizeImage;
      This->lpbiPrev = realloc(This->lpbiPrev, size);
      if (This->lpbiPrev == NULL)
	return AVIERR_MEMORY;
      This->lpPrev = DIBPTR(This->lpbiPrev);

      /* prepare codec also for decompression */
      if (ICDecompressBegin(This->hic,This->lpbiOutput,This->lpbiPrev) != S_OK)
	return AVIERR_COMPRESSOR;
    }
  } else {
    /* format change -- check that's only the palette */
    LPBITMAPINFOHEADER lpbi = format;

    if (lpbi->biSize != This->lpbiInput->biSize ||
	lpbi->biWidth != This->lpbiInput->biWidth ||
	lpbi->biHeight != This->lpbiInput->biHeight ||
	lpbi->biBitCount != This->lpbiInput->biBitCount ||
	lpbi->biPlanes != This->lpbiInput->biPlanes ||
	lpbi->biCompression != This->lpbiInput->biCompression ||
	lpbi->biClrUsed != This->lpbiInput->biClrUsed)
      return AVIERR_UNSUPPORTED;

    /* get new output format */
    if (ICCompressGetFormat(This->hic, lpbi, This->lpbiOutput) < S_OK)
      return AVIERR_BADFORMAT;

    /* restart compression */
    ICCompressEnd(This->hic);
    if (ICCompressBegin(This->hic, lpbi, This->lpbiOutput) != S_OK)
      return AVIERR_COMPRESSOR;

    /* check if we need to restart decompression also */
    if (This->lKeyFrameEvery != 1 &&
	(This->dwICMFlags & VIDCF_FASTTEMPORALC) == 0) {
      ICDecompressEnd(This->hic);
      if (ICDecompressGetFormat(This->hic,This->lpbiOutput,This->lpbiPrev) < S_OK)
	return AVIERR_COMPRESSOR;
      if (ICDecompressBegin(This->hic,This->lpbiOutput,This->lpbiPrev) != S_OK)
	return AVIERR_COMPRESSOR;
    }
  }

  /* tell nested stream the new format */
  return IAVIStream_SetFormat(This->pStream, pos,
			      This->lpbiOutput, This->cbOutput);
}

static HRESULT WINAPI ICMStream_fnRead(IAVIStream *iface, LONG start,
					LONG samples, LPVOID buffer,
					LONG buffersize, LPLONG bytesread,
					LPLONG samplesread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  LPBITMAPINFOHEADER lpbi;

  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", iface, start, samples, buffer,
 	buffersize, bytesread, samplesread);

  /* clear return parameters if given */
  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;

  if (samples == 0)
    return AVIERR_OK;

  /* check parameters */
  if (samples != 1 && (bytesread == NULL && samplesread == NULL))
    return AVIERR_BADPARAM;
  if (samples == -1) /* read as much as we could */
    samples = 1;

  if (This->pg == NULL) {
    HRESULT hr = AVIFILE_OpenGetFrame(This);

    if (FAILED(hr))
      return hr;
  }

  /* compress or decompress? */
  if (This->hic == NULL) {
    /* decompress */
    lpbi = AVIStreamGetFrame(This->pg, start);
    if (lpbi == NULL)
      return AVIERR_MEMORY;

    if (buffer != NULL && buffersize > 0) {
      /* check buffersize */
      if (buffersize < lpbi->biSizeImage)
	return AVIERR_BUFFERTOOSMALL;

      memcpy(buffer, DIBPTR(lpbi), lpbi->biSizeImage);
    }

    /* fill out return parameters if given */
    if (bytesread != NULL)
      *bytesread = lpbi->biSizeImage;
  } else {
    /* compress */
    if (This->lCurrent > start)
      AVIFILE_Reset(This);

    while (start > This->lCurrent) {
      HRESULT hr;

      lpbi = AVIStreamGetFrame(This->pg, ++This->lCurrent);
      if (lpbi == NULL) {
	AVIFILE_Reset(This);
	return AVIERR_MEMORY;
      }

      hr = AVIFILE_EncodeFrame(This, lpbi, DIBPTR(lpbi));
      if (FAILED(hr)) {
	AVIFILE_Reset(This);
	return hr;
      }
    }

    if (buffer != NULL && buffersize > 0) {
      /* check buffersize */
      if (This->lpbiCur->biSizeImage > buffersize)
	return AVIERR_BUFFERTOOSMALL;

      memcpy(buffer, This->lpCur, This->lpbiCur->biSizeImage);
    }

    /* fill out return parameters if given */
    if (bytesread != NULL)
      *bytesread = This->lpbiCur->biSizeImage;
  }

  /* fill out return parameters if given */
  if (samplesread != NULL)
    *samplesread = 1;

  return AVIERR_OK;
}

static HRESULT WINAPI ICMStream_fnWrite(IAVIStream *iface, LONG start,
					 LONG samples, LPVOID buffer,
					 LONG buffersize, DWORD flags,
					 LPLONG sampwritten,
					 LPLONG byteswritten)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  HRESULT hr;

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

  if (This->sInfo.fccHandler == comptypeDIB) {
    /* only pass through */
    flags |= AVIIF_KEYFRAME;

    return IAVIStream_Write(This->pStream, start, samples, buffer, buffersize,
			    flags, sampwritten, byteswritten);
  } else {
    /* compress data before writing to pStream */
    if (samples != 1 && (sampwritten == NULL && byteswritten == NULL))
      return AVIERR_UNSUPPORTED;

    This->lCurrent = start;
    hr = AVIFILE_EncodeFrame(This, This->lpbiInput, buffer);
    if (FAILED(hr))
      return hr;

    if (This->lLastKey == start)
      flags |= AVIIF_KEYFRAME;

    return IAVIStream_Write(This->pStream, start, samples, This->lpCur,
			    This->lpbiCur->biSizeImage, flags, byteswritten,
			    sampwritten);
  }
}

static HRESULT WINAPI ICMStream_fnDelete(IAVIStream *iface, LONG start,
					  LONG samples)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%ld)\n", iface, start, samples);

  return IAVIStream_Delete(This->pStream, start, samples);
}

static HRESULT WINAPI ICMStream_fnReadData(IAVIStream *iface, DWORD fcc,
					    LPVOID lp, LPLONG lpread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lX,%p,%p)\n", iface, fcc, lp, lpread);

  assert(This->pStream != NULL);

  return IAVIStream_ReadData(This->pStream, fcc, lp, lpread);
}

static HRESULT WINAPI ICMStream_fnWriteData(IAVIStream *iface, DWORD fcc,
					     LPVOID lp, LONG size)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lx,%p,%ld)\n", iface, fcc, lp, size);

  assert(This->pStream != NULL);

  return IAVIStream_WriteData(This->pStream, fcc, lp, size);
}

static HRESULT WINAPI ICMStream_fnSetInfo(IAVIStream *iface,
					   LPAVISTREAMINFOW info, LONG infolen)
{
  FIXME("(%p,%p,%ld): stub\n", iface, info, infolen);

  return E_FAIL;
}

static const struct IAVIStreamVtbl iicmst = {
  ICMStream_fnQueryInterface,
  ICMStream_fnAddRef,
  ICMStream_fnRelease,
  ICMStream_fnCreate,
  ICMStream_fnInfo,
  ICMStream_fnFindSample,
  ICMStream_fnReadFormat,
  ICMStream_fnSetFormat,
  ICMStream_fnRead,
  ICMStream_fnWrite,
  ICMStream_fnDelete,
  ICMStream_fnReadData,
  ICMStream_fnWriteData,
  ICMStream_fnSetInfo
};

HRESULT AVIFILE_CreateICMStream(REFIID riid, LPVOID *ppv)
{
  IAVIStreamImpl *pstream;
  HRESULT         hr;

  assert(riid != NULL && ppv != NULL);

  *ppv = NULL;

  pstream = calloc(1, sizeof(IAVIStreamImpl));
  if (pstream == NULL)
    return AVIERR_MEMORY;

  pstream->IAVIStream_iface.lpVtbl = &iicmst;
  AVIFILE_Reset(pstream);

  hr = IAVIStream_QueryInterface(&pstream->IAVIStream_iface, riid, ppv);
  if (FAILED(hr))
    free(pstream);

  return hr;
}

/***********************************************************************/

static HRESULT AVIFILE_EncodeFrame(IAVIStreamImpl *This,
				   LPBITMAPINFOHEADER lpbi, LPVOID lpBits)
{
  DWORD dwMinQual, dwMaxQual, dwCurQual;
  DWORD dwRequest;
  DWORD icmFlags = 0;
  DWORD idxFlags = 0;
  BOOL  bDecreasedQual = FALSE;
  BOOL  doSizeCheck;
  BOOL  noPrev;

  /* make lKeyFrameEvery and at start a keyframe */
  if ((This->lKeyFrameEvery != 0 &&
       (This->lCurrent - This->lLastKey) >= This->lKeyFrameEvery) ||
      This->lCurrent == This->sInfo.dwStart) {
    idxFlags = AVIIF_KEYFRAME;
    icmFlags = ICCOMPRESS_KEYFRAME;
  }

  if (This->lKeyFrameEvery != 0) {
    if (This->lCurrent == This->sInfo.dwStart) {
      if (idxFlags & AVIIF_KEYFRAME) {
	/* allow keyframes to consume all unused bytes */
	dwRequest = This->dwBytesPerFrame + This->dwUnusedBytes;
	This->dwUnusedBytes = 0;
      } else {
	/* for non-keyframes only allow some of the unused bytes to be consumed */
	DWORD tmp1 = 0;
	DWORD tmp2;

	if (This->dwBytesPerFrame >= This->dwUnusedBytes)
	  tmp1 = This->dwBytesPerFrame / This->lKeyFrameEvery;
	tmp2 = (This->dwUnusedBytes + tmp1) / This->lKeyFrameEvery;

	dwRequest = This->dwBytesPerFrame - tmp1 + tmp2;
	This->dwUnusedBytes -= tmp2;
      }
    } else
      dwRequest = MAX_FRAMESIZE;
  } else {
    /* only one keyframe at start desired */
    if (This->lCurrent == This->sInfo.dwStart) {
      dwRequest = This->dwBytesPerFrame + This->dwUnusedBytes;
      This->dwUnusedBytes = 0;
    } else
      dwRequest = MAX_FRAMESIZE;
  }

  /* must we check for frame size to gain the requested
   * data rate or can we trust the codec? */
  doSizeCheck = (dwRequest != 0 && ((This->dwICMFlags & (VIDCF_CRUNCH|VIDCF_QUALITY)) == 0));

  dwMaxQual = dwCurQual = This->sInfo.dwQuality;
  dwMinQual = ICQUALITY_LOW;

  noPrev = TRUE;
  if ((icmFlags & ICCOMPRESS_KEYFRAME) == 0 && 
      (This->dwICMFlags & VIDCF_FASTTEMPORALC) == 0)
    noPrev = FALSE;

  do {
    DWORD   idxCkid = 0;
    DWORD   res;

    res = ICCompress(This->hic,icmFlags,This->lpbiCur,This->lpCur,lpbi,lpBits,
		     &idxCkid, &idxFlags, This->lCurrent, dwRequest, dwCurQual,
		     noPrev ? NULL:This->lpbiPrev, noPrev ? NULL:This->lpPrev);
    if (res == ICERR_NEWPALETTE) {
      FIXME(": codec has changed palette -- unhandled!\n");
    } else if (res != ICERR_OK)
      return AVIERR_COMPRESSOR;

    /* need to check for framesize */
    if (! doSizeCheck)
      break;

    if (dwRequest >= This->lpbiCur->biSizeImage) {
      /* frame is smaller -- try to maximize quality */
      if (dwMaxQual - dwCurQual > 10) {
	DWORD tmp = dwRequest / 8;

	if (tmp < MAX_FRAMESIZE_DIFF)
	  tmp = MAX_FRAMESIZE_DIFF;

	if (tmp < dwRequest - This->lpbiCur->biSizeImage && bDecreasedQual) {
	  tmp = dwCurQual;
	  dwCurQual = (dwMinQual + dwMaxQual) / 2;
	  dwMinQual = tmp;
	  continue;
	}
      } else
	break;
    } else if (dwMaxQual - dwMinQual <= 1) {
      break;
    } else {
      dwMaxQual = dwCurQual;

      if (bDecreasedQual || dwCurQual == This->dwLastQuality)
	dwCurQual = (dwMinQual + dwMaxQual) / 2;
      else
	FIXME(": no new quality computed min=%lu cur=%lu max=%lu last=%lu\n",
	      dwMinQual, dwCurQual, dwMaxQual, This->dwLastQuality);

      bDecreasedQual = TRUE;
    }
  } while (TRUE);

  /* remember some values */
  This->dwLastQuality = dwCurQual;
  This->dwUnusedBytes = dwRequest - This->lpbiCur->biSizeImage;
  if (icmFlags & ICCOMPRESS_KEYFRAME)
    This->lLastKey = This->lCurrent;

  /* Does we manage previous frame? */
  if (This->lpPrev != NULL && This->lKeyFrameEvery != 1)
    ICDecompress(This->hic, 0, This->lpbiCur, This->lpCur,
		 This->lpbiPrev, This->lpPrev);

  return AVIERR_OK;
}

static HRESULT AVIFILE_OpenGetFrame(IAVIStreamImpl *This)
{
  LPBITMAPINFOHEADER lpbi;
  DWORD              size;

  /* pre-conditions */
  assert(This != NULL);
  assert(This->pStream != NULL);
  assert(This->pg == NULL);

  This->pg = AVIStreamGetFrameOpen(This->pStream, NULL);
  if (This->pg == NULL)
    return AVIERR_ERROR;

  /* When we only decompress this is enough */
  if (This->sInfo.fccHandler == comptypeDIB)
    return AVIERR_OK;

  assert(This->hic != NULL);
  assert(This->lpbiOutput == NULL);

  /* get input format */
  lpbi = AVIStreamGetFrame(This->pg, This->sInfo.dwStart);
  if (lpbi == NULL)
    return AVIERR_MEMORY;

  /* get memory for output format */
  size = ICCompressGetFormatSize(This->hic, lpbi);
  if ((LONG)size < (LONG)sizeof(BITMAPINFOHEADER))
    return AVIERR_COMPRESSOR;
  This->lpbiOutput = malloc(size);
  if (This->lpbiOutput == NULL)
    return AVIERR_MEMORY;
  This->cbOutput = size;

  if (ICCompressGetFormat(This->hic, lpbi, This->lpbiOutput) < S_OK)
    return AVIERR_BADFORMAT;

  /* update AVISTREAMINFO structure */
  This->sInfo.rcFrame.right  =
    This->sInfo.rcFrame.left + This->lpbiOutput->biWidth;
  This->sInfo.rcFrame.bottom =
    This->sInfo.rcFrame.top  + This->lpbiOutput->biHeight;
  This->sInfo.dwSuggestedBufferSize =
    ICCompressGetSize(This->hic, lpbi, This->lpbiOutput);

  /* prepare codec for compression */
  if (ICCompressBegin(This->hic, lpbi, This->lpbiOutput) != S_OK)
    return AVIERR_COMPRESSOR;

  /* allocate memory for current frame */
  size += This->sInfo.dwSuggestedBufferSize;
  This->lpbiCur = malloc(size);
  if (This->lpbiCur == NULL)
    return AVIERR_MEMORY;
  memcpy(This->lpbiCur, This->lpbiOutput, This->cbOutput);
  This->lpCur = DIBPTR(This->lpbiCur);

  /* allocate memory for last frame if needed */
  if (This->lKeyFrameEvery != 1 &&
      (This->dwICMFlags & VIDCF_FASTTEMPORALC) == 0) {
    size = ICDecompressGetFormatSize(This->hic, This->lpbiOutput);
    This->lpbiPrev = malloc(size);
    if (This->lpbiPrev == NULL)
      return AVIERR_MEMORY;
    if (ICDecompressGetFormat(This->hic, This->lpbiOutput, This->lpbiPrev) < S_OK)
      return AVIERR_COMPRESSOR;

    if (This->lpbiPrev->biSizeImage == 0) {
      This->lpbiPrev->biSizeImage =
	DIBWIDTHBYTES(*This->lpbiPrev) * This->lpbiPrev->biHeight;
    }

    /* get memory for format and picture */
    size += This->lpbiPrev->biSizeImage;
    This->lpbiPrev = realloc(This->lpbiPrev, size);
    if (This->lpbiPrev == NULL)
      return AVIERR_MEMORY;
    This->lpPrev = DIBPTR(This->lpbiPrev);

    /* prepare codec also for decompression */
    if (ICDecompressBegin(This->hic,This->lpbiOutput,This->lpbiPrev) != S_OK)
      return AVIERR_COMPRESSOR;
  }

  return AVIERR_OK;
}
