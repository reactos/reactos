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

#define COBJMACROS
#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winerror.h"
#include "mmsystem.h"
#include "vfw.h"
#include "msacm.h"

#include "avifile_private.h"
#include "extrachunk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************/

#define formtypeWAVE    mmioFOURCC('W','A','V','E')
#define ckidWAVEFORMAT  mmioFOURCC('f','m','t',' ')
#define ckidWAVEFACT    mmioFOURCC('f','a','c','t')
#define ckidWAVEDATA    mmioFOURCC('d','a','t','a')

/***********************************************************************/

#define ENDIAN_SWAPWORD(x)  ((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))
#define ENDIAN_SWAPDWORD(x) (ENDIAN_SWAPWORD((x >> 16) & 0xFFFF) | \
                             ENDIAN_SWAPWORD(x & 0xFFFF) << 16)

#ifdef WORDS_BIGENDIAN
#define BE2H_WORD(x)  (x)
#define BE2H_DWORD(x) (x)
#define LE2H_WORD(x)  ENDIAN_SWAPWORD(x)
#define LE2H_DWORD(x) ENDIAN_SWAPDWORD(x)
#else
#define BE2H_WORD(x)  ENDIAN_SWAPWORD(x)
#define BE2H_DWORD(x) ENDIAN_SWAPDWORD(x)
#define LE2H_WORD(x)  (x)
#define LE2H_DWORD(x) (x)
#endif

typedef struct {
  FOURCC  fccType;
  DWORD   offset;
  DWORD   size;
  INT     encoding;
  DWORD   sampleRate;
  DWORD   channels;
} SUNAUDIOHEADER;

#define AU_ENCODING_ULAW_8                 1
#define AU_ENCODING_PCM_8                  2
#define AU_ENCODING_PCM_16                 3
#define AU_ENCODING_PCM_24                 4
#define AU_ENCODING_PCM_32                 5
#define AU_ENCODING_FLOAT                  6
#define AU_ENCODING_DOUBLE                 7
#define AU_ENCODING_ADPCM_G721_32         23
#define AU_ENCODING_ADPCM_G722            24
#define AU_ENCODING_ADPCM_G723_24         25
#define AU_ENCODING_ADPCM_G723_5          26
#define AU_ENCODING_ALAW_8                27

/***********************************************************************/

typedef struct _IAVIFileImpl {
  IUnknown          IUnknown_inner;
  IAVIFile          IAVIFile_iface;
  IPersistFile      IPersistFile_iface;
  IAVIStream        IAVIStream_iface;
  IUnknown          *outer_unk;
  LONG              ref;
  /* IAVIFile, IAVIStream stuff... */
  AVIFILEINFOW      fInfo;
  AVISTREAMINFOW    sInfo;

  LPWAVEFORMATEX    lpFormat;
  LONG              cbFormat;

  MMCKINFO          ckData;

  EXTRACHUNKS       extra;

  /* IPersistFile stuff ... */
  HMMIO             hmmio;
  LPWSTR            szFileName;
  UINT              uMode;
  BOOL              fDirty;
} IAVIFileImpl;

/***********************************************************************/

static HRESULT AVIFILE_LoadFile(IAVIFileImpl *This);
static HRESULT AVIFILE_LoadSunFile(IAVIFileImpl *This);
static HRESULT AVIFILE_SaveFile(const IAVIFileImpl *This);

static inline IAVIFileImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IAVIFileImpl, IUnknown_inner);
}

static HRESULT WINAPI IUnknown_fnQueryInterface(IUnknown *iface, REFIID riid, void **ret_iface)
{
    IAVIFileImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
        *ret_iface = &This->IUnknown_inner;
    else if (IsEqualGUID(&IID_IAVIFile, riid))
        *ret_iface = &This->IAVIFile_iface;
    else if (IsEqualGUID(&IID_IAVIStream, riid))
        *ret_iface = &This->IAVIStream_iface;
    else if (IsEqualGUID(&IID_IPersistFile, riid))
        *ret_iface = &This->IPersistFile_iface;
    else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ret_iface);
        *ret_iface = NULL;
        return E_NOINTERFACE;
    }

    /* Violation of the COM aggregation ref counting rule */
    IUnknown_AddRef(&This->IUnknown_inner);
    return S_OK;
}

static ULONG WINAPI IUnknown_fnAddRef(IUnknown *iface)
{
    IAVIFileImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IUnknown_fnRelease(IUnknown *iface)
{
    IAVIFileImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        /* need to write headers to file */
        if (This->fDirty)
            AVIFILE_SaveFile(This);

        free(This->lpFormat);
        This->lpFormat = NULL;
        This->cbFormat = 0;
        free(This->extra.lp);
        This->extra.lp = NULL;
        This->extra.cb = 0;
        free(This->szFileName);
        This->szFileName = NULL;
        if (This->hmmio) {
            mmioClose(This->hmmio, 0);
            This->hmmio = NULL;
        }
        free(This);
    }

    return ref;
}

static const IUnknownVtbl unk_vtbl =
{
    IUnknown_fnQueryInterface,
    IUnknown_fnAddRef,
    IUnknown_fnRelease
};

static inline IAVIFileImpl *impl_from_IAVIFile(IAVIFile *iface)
{
    return CONTAINING_RECORD(iface, IAVIFileImpl, IAVIFile_iface);
}

static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile *iface, REFIID riid, void **ret_iface)
{
    IAVIFileImpl *This = impl_from_IAVIFile(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ret_iface);
}

static ULONG WINAPI IAVIFile_fnAddRef(IAVIFile *iface)
{
    IAVIFileImpl *This = impl_from_IAVIFile(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI IAVIFile_fnRelease(IAVIFile *iface)
{
    IAVIFileImpl *This = impl_from_IAVIFile(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI IAVIFile_fnInfo(IAVIFile *iface, AVIFILEINFOW *afi, LONG size)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,%p,%ld)\n",iface,afi,size);

  if (afi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  /* update file info */
  This->fInfo.dwFlags = 0;
  This->fInfo.dwCaps  = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
  if (This->lpFormat != NULL) {
    assert(This->sInfo.dwScale != 0);

    This->fInfo.dwStreams             = 1;
    This->fInfo.dwScale               = This->sInfo.dwScale;
    This->fInfo.dwRate                = This->sInfo.dwRate;
    This->fInfo.dwLength              = This->sInfo.dwLength;
    This->fInfo.dwSuggestedBufferSize = This->ckData.cksize;
    This->fInfo.dwMaxBytesPerSec =
      MulDiv(This->sInfo.dwSampleSize,This->sInfo.dwRate,This->sInfo.dwScale);
  }

  memcpy(afi, &This->fInfo, min((DWORD)size, sizeof(This->fInfo)));

  if ((DWORD)size < sizeof(This->fInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnGetStream(IAVIFile *iface, IAVIStream **avis, DWORD fccType,
        LONG lParam)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,%p,0x%08lX,%ld)\n", iface, avis, fccType, lParam);

  /* check parameter */
  if (avis == NULL)
    return AVIERR_BADPARAM;

  *avis = NULL;

  /* Does our stream exists? */
  if (lParam != 0 || This->fInfo.dwStreams == 0)
    return AVIERR_NODATA;
  if (fccType != 0 && fccType != streamtypeAUDIO)
    return AVIERR_NODATA;

  *avis = &This->IAVIStream_iface;
  IAVIStream_AddRef(*avis);

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnCreateStream(IAVIFile *iface, IAVIStream **avis,
        AVISTREAMINFOW *asi)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,%p,%p)\n", iface, avis, asi);

  /* check parameters */
  if (avis == NULL || asi == NULL)
    return AVIERR_BADPARAM;

  *avis = NULL;

  /* We only support one audio stream */
  if (This->fInfo.dwStreams != 0 || This->lpFormat != NULL)
    return AVIERR_UNSUPPORTED;
  if (asi->fccType != streamtypeAUDIO)
    return AVIERR_UNSUPPORTED;

  /* Does the user have write permission? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  This->cbFormat = 0;
  This->lpFormat = NULL;

  memcpy(&This->sInfo, asi, sizeof(This->sInfo));

  /* make sure streaminfo if okay for us */
  This->sInfo.fccHandler          = 0;
  This->sInfo.dwFlags             = 0;
  This->sInfo.dwCaps              = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
  This->sInfo.dwStart             = 0;
  This->sInfo.dwInitialFrames     = 0;
  This->sInfo.dwFormatChangeCount = 0;
  SetRectEmpty(&This->sInfo.rcFrame);

  This->fInfo.dwStreams = 1;
  This->fInfo.dwScale   = This->sInfo.dwScale;
  This->fInfo.dwRate    = This->sInfo.dwRate;
  This->fInfo.dwLength  = This->sInfo.dwLength;

  This->ckData.dwDataOffset = 0;
  This->ckData.cksize       = 0;

  *avis = &This->IAVIStream_iface;
  IAVIStream_AddRef(*avis);

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnWriteData(IAVIFile *iface, DWORD ckid, void *lpData, LONG size)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,0x%08lX,%p,%ld)\n", iface, ckid, lpData, size);

  /* check parameters */
  if (lpData == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  /* Do we have write permission? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  This->fDirty = TRUE;

  return WriteExtraChunk(&This->extra, ckid, lpData, size);
}

static HRESULT WINAPI IAVIFile_fnReadData(IAVIFile *iface, DWORD ckid, void *lpData, LONG *size)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,0x%08lX,%p,%p)\n", iface, ckid, lpData, size);

  return ReadExtraChunk(&This->extra, ckid, lpData, size);
}

static HRESULT WINAPI IAVIFile_fnEndRecord(IAVIFile *iface)
{
  TRACE("(%p)\n",iface);

  /* This is only needed for interleaved files.
   * We have only one stream, which can't be interleaved.
   */
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnDeleteStream(IAVIFile *iface, DWORD fccType, LONG lParam)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,0x%08lX,%ld)\n", iface, fccType, lParam);

  /* check parameter */
  if (lParam < 0)
    return AVIERR_BADPARAM;

  /* Do we have our audio stream? */
  if (lParam != 0 || This->fInfo.dwStreams == 0 ||
      (fccType != 0 && fccType != streamtypeAUDIO))
    return AVIERR_NODATA;

  /* Have user write permissions? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  free(This->lpFormat);
  This->lpFormat = NULL;
  This->cbFormat = 0;

  /* update infos */
  This->ckData.dwDataOffset = 0;
  This->ckData.cksize       = 0;

  This->sInfo.dwScale   = 0;
  This->sInfo.dwRate    = 0;
  This->sInfo.dwLength  = 0;
  This->sInfo.dwSuggestedBufferSize = 0;

  This->fInfo.dwStreams = 0;
  This->fInfo.dwEditCount++;

  This->fDirty = TRUE;

  return AVIERR_OK;
}

static const struct IAVIFileVtbl iwavft = {
    IAVIFile_fnQueryInterface,
    IAVIFile_fnAddRef,
    IAVIFile_fnRelease,
    IAVIFile_fnInfo,
    IAVIFile_fnGetStream,
    IAVIFile_fnCreateStream,
    IAVIFile_fnWriteData,
    IAVIFile_fnReadData,
    IAVIFile_fnEndRecord,
    IAVIFile_fnDeleteStream
};

/***********************************************************************/

static inline IAVIFileImpl *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, IAVIFileImpl, IPersistFile_iface);
}

static HRESULT WINAPI IPersistFile_fnQueryInterface(IPersistFile *iface, REFIID riid,
        void **ret_iface)
{
    IAVIFileImpl *This = impl_from_IPersistFile(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ret_iface);
}

static ULONG   WINAPI IPersistFile_fnAddRef(IPersistFile *iface)
{
    IAVIFileImpl *This = impl_from_IPersistFile(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG   WINAPI IPersistFile_fnRelease(IPersistFile *iface)
{
    IAVIFileImpl *This = impl_from_IPersistFile(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile *iface,
						LPCLSID pClassID)
{
  TRACE("(%p,%p)\n", iface, pClassID);

  if (pClassID == NULL)
    return AVIERR_BADPARAM;

  *pClassID = CLSID_WAVFile;

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile *iface)
{
    IAVIFileImpl *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", iface);

    return (This->fDirty ? S_OK : S_FALSE);
}

static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile *iface, LPCOLESTR pszFileName, DWORD dwMode)
{
  IAVIFileImpl *This = impl_from_IPersistFile(iface);
  WCHAR wszStreamFmt[50];
  INT   len;

  TRACE("(%p,%s,0x%08lX)\n", iface, debugstr_w(pszFileName), dwMode);

  /* check parameter */
  if (pszFileName == NULL)
    return AVIERR_BADPARAM;

  if (This->hmmio != NULL)
    return AVIERR_ERROR; /* No reuse of this object for another file! */

  /* remember mode and name */
  This->uMode = dwMode;

  len = lstrlenW(pszFileName) + 1;
  This->szFileName = malloc(len * sizeof(WCHAR));
  if (This->szFileName == NULL)
    return AVIERR_MEMORY;
  lstrcpyW(This->szFileName, pszFileName);

  /* try to open the file */
  This->hmmio = mmioOpenW(This->szFileName, NULL, MMIO_ALLOCBUF | dwMode);
  if (This->hmmio == NULL) {
    /* mmioOpenW not in native DLLs of Win9x -- try mmioOpenA */
    LPSTR szFileName;
    len = WideCharToMultiByte(CP_ACP, 0, This->szFileName, -1,
                              NULL, 0, NULL, NULL);
    szFileName = malloc(len * sizeof(CHAR));
    if (szFileName == NULL)
      return AVIERR_MEMORY;

    WideCharToMultiByte(CP_ACP, 0, This->szFileName, -1, szFileName,
			len, NULL, NULL);

    This->hmmio = mmioOpenA(szFileName, NULL, MMIO_ALLOCBUF | dwMode);
    free(szFileName);
    if (This->hmmio == NULL)
      return AVIERR_FILEOPEN;
  }

  memset(& This->fInfo, 0, sizeof(This->fInfo));
  memset(& This->sInfo, 0, sizeof(This->sInfo));

  LoadStringW(AVIFILE_hModule, IDS_WAVEFILETYPE, This->fInfo.szFileType,
	      ARRAY_SIZE(This->fInfo.szFileType));
  if (LoadStringW(AVIFILE_hModule, IDS_WAVESTREAMFORMAT,
		  wszStreamFmt, ARRAY_SIZE(wszStreamFmt)) > 0) {
    wsprintfW(This->sInfo.szName, wszStreamFmt,
	      AVIFILE_BasenameW(This->szFileName));
  }

  /* should we create a new file? */
  if (dwMode & OF_CREATE) {
    /* nothing more to do */
    return AVIERR_OK;
  } else
    return AVIFILE_LoadFile(This);
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile *iface,
					  LPCOLESTR pszFileName,BOOL fRemember)
{
  TRACE("(%p,%s,%d)\n", iface, debugstr_w(pszFileName), fRemember);

  /* We write directly to disk, so nothing to do. */

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile *iface,
						   LPCOLESTR pszFileName)
{
  TRACE("(%p,%s)\n", iface, debugstr_w(pszFileName));

  /* We write directly to disk, so nothing to do. */

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile *iface, LPOLESTR *ppszFileName)
{
  IAVIFileImpl *This = impl_from_IPersistFile(iface);

  TRACE("(%p,%p)\n", iface, ppszFileName);

  if (ppszFileName == NULL)
    return AVIERR_BADPARAM;

  *ppszFileName = NULL;

  if (This->szFileName) {
    int len = lstrlenW(This->szFileName) + 1;

    *ppszFileName = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*ppszFileName == NULL)
      return AVIERR_MEMORY;

    lstrcpyW(*ppszFileName, This->szFileName);
  }

  return AVIERR_OK;
}

static const struct IPersistFileVtbl iwavpft = {
    IPersistFile_fnQueryInterface,
    IPersistFile_fnAddRef,
    IPersistFile_fnRelease,
    IPersistFile_fnGetClassID,
    IPersistFile_fnIsDirty,
    IPersistFile_fnLoad,
    IPersistFile_fnSave,
    IPersistFile_fnSaveCompleted,
    IPersistFile_fnGetCurFile
};

/***********************************************************************/

static inline IAVIFileImpl *impl_from_IAVIStream(IAVIStream *iface)
{
    return CONTAINING_RECORD(iface, IAVIFileImpl, IAVIStream_iface);
}

static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream *iface, REFIID riid, void **ret_iface)
{
    IAVIFileImpl *This = impl_from_IAVIStream(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ret_iface);
}

static ULONG WINAPI IAVIStream_fnAddRef(IAVIStream *iface)
{
    IAVIFileImpl *This = impl_from_IAVIStream(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI IAVIStream_fnRelease(IAVIStream* iface)
{
    IAVIFileImpl *This = impl_from_IAVIStream(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream *iface, LPARAM lParam1,
					  LPARAM lParam2)
{
  TRACE("(%p,0x%08IX,0x%08IX)\n", iface, lParam1, lParam2);

  /* This IAVIStream interface needs an WAVFile */
  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream *iface, AVISTREAMINFOW *psi, LONG size)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

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

static LONG WINAPI IAVIStream_fnFindSample(IAVIStream *iface, LONG pos, LONG flags)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,0x%08lX)\n",iface,pos,flags);

  /* Do we have data? */
  if (This->lpFormat == NULL)
    return -1;

  /* We don't have an index */
  if (flags & FIND_INDEX)
    return -1;

  if (flags & FIND_FROM_START) {
    pos = This->sInfo.dwStart;
    flags &= ~(FIND_FROM_START|FIND_PREV);
    flags |= FIND_NEXT;
  }

  if (flags & FIND_FORMAT) {
    if ((flags & FIND_NEXT) && pos > 0)
      pos = -1;
    else
      pos = 0;
  }

  if ((flags & FIND_RET) == FIND_LENGTH ||
      (flags & FIND_RET) == FIND_SIZE)
    return This->sInfo.dwSampleSize;
  if ((flags & FIND_RET) == FIND_OFFSET)
    return This->ckData.dwDataOffset + pos * This->sInfo.dwSampleSize;

  return pos;
}

static HRESULT WINAPI IAVIStream_fnReadFormat(IAVIStream *iface, LONG pos, void *format,
        LONG *formatsize)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%p,%p)\n", iface, pos, format, formatsize);

  if (formatsize == NULL)
    return AVIERR_BADPARAM;

  /* only interested in needed buffersize? */
  if (format == NULL || *formatsize <= 0) {
    *formatsize = This->cbFormat;

    return AVIERR_OK;
  }

  /* copy initial format (only as much as will fit) */
  memcpy(format, This->lpFormat, min(*formatsize, This->cbFormat));
  if (*formatsize < This->cbFormat) {
    *formatsize = This->cbFormat;
    return AVIERR_BUFFERTOOSMALL;
  }

  *formatsize = This->cbFormat;
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnSetFormat(IAVIStream *iface, LONG pos, void *format,
        LONG formatsize)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%p,%ld)\n", iface, pos, format, formatsize);

  /* check parameters */
  if (format == NULL || formatsize <= sizeof(PCMWAVEFORMAT))
    return AVIERR_BADPARAM;

  /* We can only do this to an empty wave file, but ignore call
   * if still same format */
  if (This->lpFormat != NULL) {
    if (formatsize != This->cbFormat ||
	memcmp(format, This->lpFormat, formatsize) != 0)
      return AVIERR_UNSUPPORTED;

    return AVIERR_OK;
  }

  /* only support start at position 0 */
  if (pos != 0)
    return AVIERR_UNSUPPORTED;

  /* Do we have write permission? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* get memory for format and copy it */
  This->lpFormat = malloc(formatsize);
  if (This->lpFormat == NULL)
    return AVIERR_MEMORY;

  This->cbFormat = formatsize;
  memcpy(This->lpFormat, format, formatsize);

  /* update info's about 'data' chunk */
  This->ckData.dwDataOffset = formatsize + 7 * sizeof(DWORD);
  This->ckData.cksize       = 0;

  /* for non-pcm format we need also a 'fact' chunk */
  if (This->lpFormat->wFormatTag != WAVE_FORMAT_PCM)
    This->ckData.dwDataOffset += 3 * sizeof(DWORD);

  /* update stream and file info */
  This->sInfo.dwSampleSize = This->lpFormat->nBlockAlign;
  This->sInfo.dwScale      = This->lpFormat->nBlockAlign;
  This->sInfo.dwRate       = This->lpFormat->nAvgBytesPerSec;
  This->sInfo.dwLength     = 0;
  This->sInfo.dwSuggestedBufferSize = 0;

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnRead(IAVIStream *iface, LONG start, LONG samples, void *buffer,
        LONG buffersize, LONG *bytesread, LONG *samplesread)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", iface, start, samples, buffer,
	buffersize, bytesread, samplesread);

  /* clear return parameters if given */
  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;

  /* positions without data */
  if (start < 0 || (DWORD)start > This->sInfo.dwLength)
    return AVIERR_OK;

  /* check samples */
  if (samples < 0)
    samples = 0;
  if (buffersize > 0) {
    if (samples > 0)
      samples = min((DWORD)samples, buffersize / This->sInfo.dwSampleSize);
    else
      samples = buffersize / This->sInfo.dwSampleSize;
  }

  /* limit to end of stream */
  if ((DWORD)(start + samples) > This->sInfo.dwLength)
    samples = This->sInfo.dwLength - start;

  /* request only the sizes? */
  if (buffer == NULL || buffersize <= 0) {
    /* then I need at least one parameter for it */
    if (bytesread == NULL && samplesread == NULL)
      return AVIERR_BADPARAM;

    if (bytesread != NULL)
      *bytesread = samples * This->sInfo.dwSampleSize;
    if (samplesread != NULL)
      *samplesread = samples;

    return AVIERR_OK;
  }

  /* nothing to read? */
  if (samples == 0)
    return AVIERR_OK;

  /* Can I read at least one sample? */
  if ((DWORD)buffersize < This->sInfo.dwSampleSize)
    return AVIERR_BUFFERTOOSMALL;

  buffersize = samples * This->sInfo.dwSampleSize;

  if (mmioSeek(This->hmmio, This->ckData.dwDataOffset
	       + start * This->sInfo.dwSampleSize, SEEK_SET) == -1)
    return AVIERR_FILEREAD;
  if (mmioRead(This->hmmio, buffer, buffersize) != buffersize)
    return AVIERR_FILEREAD;

  /* fill out return parameters if given */
  if (bytesread != NULL)
    *bytesread = buffersize;
  if (samplesread != NULL)
    *samplesread = samples;  

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnWrite(IAVIStream *iface, LONG start, LONG samples, void *buffer,
        LONG buffersize, DWORD flags, LONG *sampwritten, LONG *byteswritten)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

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

  /* Do we have write permission? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* < 0 means "append" */
  if (start < 0)
    start = This->sInfo.dwStart + This->sInfo.dwLength;

  /* check buffersize -- must multiple of samplesize */
  if (buffersize & ~(This->sInfo.dwSampleSize - 1))
    return AVIERR_BADSIZE;

  /* do we have anything to write? */
  if (buffer != NULL && buffersize > 0) {
    This->fDirty = TRUE;

    if (mmioSeek(This->hmmio, This->ckData.dwDataOffset +
		 start * This->sInfo.dwSampleSize, SEEK_SET) == -1)
      return AVIERR_FILEWRITE;
    if (mmioWrite(This->hmmio, buffer, buffersize) != buffersize)
      return AVIERR_FILEWRITE;

    This->sInfo.dwLength = max(This->sInfo.dwLength, (DWORD)start + samples);
    This->ckData.cksize  = max(This->ckData.cksize,
			       start * This->sInfo.dwSampleSize + buffersize);

    /* fill out return parameters if given */
    if (sampwritten != NULL)
      *sampwritten = samples;
    if (byteswritten != NULL)
      *byteswritten = buffersize;
  }

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnDelete(IAVIStream *iface, LONG start, LONG samples)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

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

  /* For the rest we need write permissions */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  if ((DWORD)(start + samples) >= This->sInfo.dwLength) {
    /* deletion at end */
    samples = This->sInfo.dwLength - start;
    This->sInfo.dwLength -= samples;
    This->ckData.cksize  -= samples * This->sInfo.dwSampleSize;
  } else if ((DWORD)start <= This->sInfo.dwStart) {
    /* deletion at start */
    samples = This->sInfo.dwStart - start;
    start   = This->sInfo.dwStart;
    This->ckData.dwDataOffset += samples * This->sInfo.dwSampleSize;
    This->ckData.cksize       -= samples * This->sInfo.dwSampleSize;
  } else {
    /* deletion inside stream -- needs playlist and cue's */
    FIXME(": deletion inside of stream not supported!\n");

    return AVIERR_UNSUPPORTED;
  }

  This->fDirty = TRUE;

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnReadData(IAVIStream *iface, DWORD fcc, void *lp, LONG *lpread)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  return IAVIFile_ReadData(&This->IAVIFile_iface, fcc, lp, lpread);
}

static HRESULT WINAPI IAVIStream_fnWriteData(IAVIStream *iface, DWORD fcc, void *lp, LONG size)
{
  IAVIFileImpl *This = impl_from_IAVIStream(iface);

  return IAVIFile_WriteData(&This->IAVIFile_iface, fcc, lp, size);
}

static HRESULT WINAPI IAVIStream_fnSetInfo(IAVIStream *iface,
					   LPAVISTREAMINFOW info, LONG infolen)
{
  FIXME("(%p,%p,%ld): stub\n", iface, info, infolen);

  return E_FAIL;
}

static const struct IAVIStreamVtbl iwavst = {
    IAVIStream_fnQueryInterface,
    IAVIStream_fnAddRef,
    IAVIStream_fnRelease,
    IAVIStream_fnCreate,
    IAVIStream_fnInfo,
    IAVIStream_fnFindSample,
    IAVIStream_fnReadFormat,
    IAVIStream_fnSetFormat,
    IAVIStream_fnRead,
    IAVIStream_fnWrite,
    IAVIStream_fnDelete,
    IAVIStream_fnReadData,
    IAVIStream_fnWriteData,
    IAVIStream_fnSetInfo
};

HRESULT AVIFILE_CreateWAVFile(IUnknown *outer_unk, REFIID riid, void **ret_iface)
{
    IAVIFileImpl *pfile;
    HRESULT hr;

    *ret_iface = NULL;

    pfile = calloc(1, sizeof(*pfile));
    if (!pfile)
        return AVIERR_MEMORY;

    pfile->IUnknown_inner.lpVtbl = &unk_vtbl;
    pfile->IAVIFile_iface.lpVtbl = &iwavft;
    pfile->IPersistFile_iface.lpVtbl = &iwavpft;
    pfile->IAVIStream_iface.lpVtbl = &iwavst;
    pfile->ref = 1;
    if (outer_unk)
        pfile->outer_unk = outer_unk;
    else
        pfile->outer_unk = &pfile->IUnknown_inner;

    hr = IUnknown_QueryInterface(&pfile->IUnknown_inner, riid, ret_iface);
    IUnknown_Release(&pfile->IUnknown_inner);

    return hr;
}

/***********************************************************************/

static HRESULT AVIFILE_LoadFile(IAVIFileImpl *This)
{
  MMCKINFO ckRIFF;
  MMCKINFO ck;

  This->sInfo.dwLength = 0; /* just to be sure */
  This->fDirty = FALSE;

  /* search for RIFF chunk */
  ckRIFF.fccType = 0; /* find any */
  if (mmioDescend(This->hmmio, &ckRIFF, NULL, MMIO_FINDRIFF) != S_OK) {
    return AVIFILE_LoadSunFile(This);
  }

  if (ckRIFF.fccType != formtypeWAVE)
    return AVIERR_BADFORMAT;

  /* search WAVE format chunk */
  ck.ckid = ckidWAVEFORMAT;
  if (FindChunkAndKeepExtras(&This->extra, This->hmmio, &ck,
			     &ckRIFF, MMIO_FINDCHUNK) != S_OK)
    return AVIERR_FILEREAD;

  /* get memory for format and read it */
  This->lpFormat = malloc(ck.cksize);
  if (This->lpFormat == NULL)
    return AVIERR_FILEREAD;
  This->cbFormat = ck.cksize;

  if (mmioRead(This->hmmio, (HPSTR)This->lpFormat, ck.cksize) != ck.cksize)
    return AVIERR_FILEREAD;
  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEREAD;

  /* Non-pcm formats have a fact chunk.
   * We don't need it, so simply add it to the extra chunks.
   */

  /* find the big data chunk */
  This->ckData.ckid = ckidWAVEDATA;
  if (FindChunkAndKeepExtras(&This->extra, This->hmmio, &This->ckData,
			     &ckRIFF, MMIO_FINDCHUNK) != S_OK)
    return AVIERR_FILEREAD;

  memset(&This->sInfo, 0, sizeof(This->sInfo));
  This->sInfo.fccType      = streamtypeAUDIO;
  This->sInfo.dwRate       = This->lpFormat->nAvgBytesPerSec;
  This->sInfo.dwSampleSize =
    This->sInfo.dwScale    = This->lpFormat->nBlockAlign;
  This->sInfo.dwLength     = This->ckData.cksize / This->lpFormat->nBlockAlign;
  This->sInfo.dwSuggestedBufferSize = This->ckData.cksize;

  This->fInfo.dwStreams = 1;

  if (mmioAscend(This->hmmio, &This->ckData, 0) != S_OK) {
    /* seems to be truncated */
    WARN(": file seems to be truncated!\n");
    This->ckData.cksize  = mmioSeek(This->hmmio, 0, SEEK_END) -
      This->ckData.dwDataOffset;
    This->sInfo.dwLength = This->ckData.cksize / This->lpFormat->nBlockAlign;
    This->sInfo.dwSuggestedBufferSize = This->ckData.cksize;
  }

  /* ignore errors */
  FindChunkAndKeepExtras(&This->extra, This->hmmio, &ck, &ckRIFF, 0);

  return AVIERR_OK;
}

static HRESULT AVIFILE_LoadSunFile(IAVIFileImpl *This)
{
  SUNAUDIOHEADER auhdr;

  mmioSeek(This->hmmio, 0, SEEK_SET);
  if (mmioRead(This->hmmio, (HPSTR)&auhdr, sizeof(auhdr)) != sizeof(auhdr))
    return AVIERR_FILEREAD;

  if (auhdr.fccType == 0x0064732E) {
    /* header in little endian */
    This->ckData.dwDataOffset = LE2H_DWORD(auhdr.offset);
    This->ckData.cksize       = LE2H_DWORD(auhdr.size);

    auhdr.encoding   = LE2H_DWORD(auhdr.encoding);
    auhdr.sampleRate = LE2H_DWORD(auhdr.sampleRate);
    auhdr.channels   = LE2H_DWORD(auhdr.channels);
  } else if (auhdr.fccType == mmioFOURCC('.','s','n','d')) {
    /* header in big endian */
    This->ckData.dwDataOffset = BE2H_DWORD(auhdr.offset);
    This->ckData.cksize       = BE2H_DWORD(auhdr.size);

    auhdr.encoding   = BE2H_DWORD(auhdr.encoding);
    auhdr.sampleRate = BE2H_DWORD(auhdr.sampleRate);
    auhdr.channels   = BE2H_DWORD(auhdr.channels);
  } else
    return AVIERR_FILEREAD;

  if (auhdr.channels < 1)
    return AVIERR_BADFORMAT;

  /* get size of header */
  switch(auhdr.encoding) {
  case AU_ENCODING_ADPCM_G721_32:
    This->cbFormat = sizeof(G721_ADPCMWAVEFORMAT); break;
  case AU_ENCODING_ADPCM_G723_24:
    This->cbFormat = sizeof(G723_ADPCMWAVEFORMAT); break;
  case AU_ENCODING_ADPCM_G722:
  case AU_ENCODING_ADPCM_G723_5:
    WARN("unsupported Sun audio format %d\n", auhdr.encoding);
    return AVIERR_UNSUPPORTED; /* FIXME */
  default:
    This->cbFormat = sizeof(WAVEFORMATEX); break;
  };

  This->lpFormat = malloc(This->cbFormat);
  if (This->lpFormat == NULL)
    return AVIERR_MEMORY;

  This->lpFormat->nChannels      = auhdr.channels;
  This->lpFormat->nSamplesPerSec = auhdr.sampleRate;
  switch(auhdr.encoding) {
  case AU_ENCODING_ULAW_8:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_MULAW;
    This->lpFormat->wBitsPerSample = 8;
    break;
  case AU_ENCODING_PCM_8:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_PCM;
    This->lpFormat->wBitsPerSample = 8;
    break;
  case AU_ENCODING_PCM_16:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_PCM;
    This->lpFormat->wBitsPerSample = 16;
    break;
  case AU_ENCODING_PCM_24:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_PCM;
    This->lpFormat->wBitsPerSample = 24;
    break;
  case AU_ENCODING_PCM_32:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_PCM;
    This->lpFormat->wBitsPerSample = 32;
    break;
  case AU_ENCODING_ALAW_8:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_ALAW;
    This->lpFormat->wBitsPerSample = 8;
    break;
  case AU_ENCODING_ADPCM_G721_32:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_G721_ADPCM;
    This->lpFormat->wBitsPerSample = (3*5*8);
    This->lpFormat->nBlockAlign    = 15*15*8;
    This->lpFormat->cbSize         = sizeof(WORD);
    ((LPG721_ADPCMWAVEFORMAT)This->lpFormat)->nAuxBlockSize = 0;
    break;
  case AU_ENCODING_ADPCM_G723_24:
    This->lpFormat->wFormatTag     = WAVE_FORMAT_G723_ADPCM;
    This->lpFormat->wBitsPerSample = (3*5*8);
    This->lpFormat->nBlockAlign    = 15*15*8;
    This->lpFormat->cbSize         = 2*sizeof(WORD);
    ((LPG723_ADPCMWAVEFORMAT)This->lpFormat)->cbExtraSize   = 0;
    ((LPG723_ADPCMWAVEFORMAT)This->lpFormat)->nAuxBlockSize = 0;
    break;
  default:
    WARN("unsupported Sun audio format %d\n", auhdr.encoding);
    return AVIERR_UNSUPPORTED;
  };

  This->lpFormat->nBlockAlign =
    (This->lpFormat->nChannels * This->lpFormat->wBitsPerSample) / 8;
  if (This->lpFormat->nBlockAlign == 0 && This->lpFormat->wBitsPerSample < 8)
    This->lpFormat->nBlockAlign++;
  This->lpFormat->nAvgBytesPerSec =
    This->lpFormat->nBlockAlign * This->lpFormat->nSamplesPerSec;

  This->fDirty = FALSE;

  This->sInfo.fccType               = streamtypeAUDIO;
  This->sInfo.fccHandler            = 0;
  This->sInfo.dwFlags               = 0;
  This->sInfo.wPriority             = 0;
  This->sInfo.wLanguage             = 0;
  This->sInfo.dwInitialFrames       = 0;
  This->sInfo.dwScale               = This->lpFormat->nBlockAlign;
  This->sInfo.dwRate                = This->lpFormat->nAvgBytesPerSec;
  This->sInfo.dwStart               = 0;
  This->sInfo.dwLength              =
    This->ckData.cksize / This->lpFormat->nBlockAlign;
  This->sInfo.dwSuggestedBufferSize = This->sInfo.dwLength;
  This->sInfo.dwSampleSize          = This->lpFormat->nBlockAlign;

  This->fInfo.dwStreams = 1;
  This->fInfo.dwScale   = 1;
  This->fInfo.dwRate    = This->lpFormat->nSamplesPerSec;
  This->fInfo.dwLength  =
    MulDiv(This->ckData.cksize, This->lpFormat->nSamplesPerSec,
	   This->lpFormat->nAvgBytesPerSec);

  return AVIERR_OK;
}

static HRESULT AVIFILE_SaveFile(const IAVIFileImpl *This)
{
  MMCKINFO ckRIFF;
  MMCKINFO ck;

  mmioSeek(This->hmmio, 0, SEEK_SET);

  /* create the RIFF chunk with formtype WAVE */
  ckRIFF.fccType = formtypeWAVE;
  ckRIFF.cksize  = 0;
  if (mmioCreateChunk(This->hmmio, &ckRIFF, MMIO_CREATERIFF) != S_OK)
    return AVIERR_FILEWRITE;

  /* the next chunk is the format */
  ck.ckid   = ckidWAVEFORMAT;
  ck.cksize = This->cbFormat;
  if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;
  if (This->lpFormat != NULL && This->cbFormat > 0) {
    if (mmioWrite(This->hmmio, (HPSTR)This->lpFormat, ck.cksize) != ck.cksize)
      return AVIERR_FILEWRITE;
  }
  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* fact chunk is needed for non-pcm waveforms */
  if (This->lpFormat != NULL && This->cbFormat > sizeof(PCMWAVEFORMAT) &&
      This->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
    WAVEFORMATEX wfx;
    DWORD        dwFactLength;
    HACMSTREAM   has;

    /* try to open an appropriate audio codec to figure out
     * data for fact-chunk */
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    if (acmFormatSuggest(NULL, This->lpFormat, &wfx,
			 sizeof(wfx), ACM_FORMATSUGGESTF_WFORMATTAG)) {
      acmStreamOpen(&has, NULL, This->lpFormat, &wfx, NULL,
		    0, 0, ACM_STREAMOPENF_NONREALTIME);
      acmStreamSize(has, This->ckData.cksize, &dwFactLength,
		    ACM_STREAMSIZEF_SOURCE);
      dwFactLength /= wfx.nBlockAlign;
      acmStreamClose(has, 0);

      /* create the fact chunk */
      ck.ckid   = ckidWAVEFACT;
      ck.cksize = sizeof(dwFactLength);

      /* test for enough space before data chunk */
      if (mmioSeek(This->hmmio, 0, SEEK_CUR) > This->ckData.dwDataOffset
	  - ck.cksize - 4 * sizeof(DWORD))
	return AVIERR_FILEWRITE;
      if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;
      if (mmioWrite(This->hmmio, (HPSTR)&dwFactLength, ck.cksize) != ck.cksize)
	return AVIERR_FILEWRITE;
      if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;
    } else
      ERR(": fact chunk is needed for non-pcm files -- currently no codec found, so skipped!\n");
  }

  /* if there was extra stuff, we need to fill it with JUNK */
  if (mmioSeek(This->hmmio, 0, SEEK_CUR) + 2 * sizeof(DWORD) < This->ckData.dwDataOffset) {
    ck.ckid   = ckidAVIPADDING;
    ck.cksize = 0;
    if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;

    if (mmioSeek(This->hmmio, This->ckData.dwDataOffset
		 - 2 * sizeof(DWORD), SEEK_SET) == -1)
      return AVIERR_FILEWRITE;
    if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;
  }

  /* create the data chunk */
  ck.ckid   = ckidWAVEDATA;
  ck.cksize = This->ckData.cksize;
  if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;
  if (mmioSeek(This->hmmio, This->ckData.cksize, SEEK_CUR) == -1)
    return AVIERR_FILEWRITE;
  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* some optional extra chunks? */
  if (This->extra.lp != NULL && This->extra.cb > 0) {
    /* chunk headers are already in structure */
    if (mmioWrite(This->hmmio, This->extra.lp, This->extra.cb) != This->extra.cb)
      return AVIERR_FILEWRITE;
  }

  /* close RIFF chunk */
  if (mmioAscend(This->hmmio, &ckRIFF, 0) != S_OK)
    return AVIERR_FILEWRITE;
  if (mmioFlush(This->hmmio, 0) != S_OK)
    return AVIERR_FILEWRITE;

  return AVIERR_OK;
}
