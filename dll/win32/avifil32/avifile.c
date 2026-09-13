/*
 * Copyright 1999 Marcus Meissner
 * Copyright 2002-2003 Michael Günnewig
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

/* TODO:
 *  - IAVIStreaming interface is missing for the IAVIStreamImpl
 *  - IAVIStream_fnFindSample: FIND_INDEX isn't supported.
 *  - IAVIStream_fnReadFormat: formatchanges aren't read in.
 *  - IAVIStream_fnDelete: a stub.
 *  - IAVIStream_fnSetInfo: a stub.
 *  - make thread safe
 *
 * KNOWN Bugs:
 *  - native version can hangup when reading a file generated with this DLL.
 *    When index is missing it works, but index seems to be okay.
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

#include "avifile_private.h"
#include "extrachunk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

#ifndef IDX_PER_BLOCK
#define IDX_PER_BLOCK 2730
#endif

typedef struct _IAVIFileImpl IAVIFileImpl;

typedef struct _IAVIStreamImpl {
  IAVIStream        IAVIStream_iface;
  LONG              ref;

  IAVIFileImpl     *paf;
  DWORD             nStream;       /* the n-th stream in file */
  AVISTREAMINFOW    sInfo;

  LPVOID            lpFormat;
  DWORD             cbFormat;

  LPVOID            lpHandlerData;
  DWORD             cbHandlerData;

  EXTRACHUNKS       extra;

  LPDWORD           lpBuffer;
  DWORD             cbBuffer;       /* size of lpBuffer */
  DWORD             dwCurrentFrame; /* frame/block currently in lpBuffer */

  LONG              lLastFrame;    /* last correct index in idxFrames */
  AVIINDEXENTRY    *idxFrames;
  DWORD             nIdxFrames;     /* upper index limit of idxFrames */
  AVIINDEXENTRY    *idxFmtChanges;
  DWORD             nIdxFmtChanges; /* upper index limit of idxFmtChanges */
} IAVIStreamImpl;

static inline IAVIStreamImpl *impl_from_IAVIStream(IAVIStream *iface)
{
  return CONTAINING_RECORD(iface, IAVIStreamImpl, IAVIStream_iface);
}

struct _IAVIFileImpl {
  IUnknown          IUnknown_inner;
  IAVIFile          IAVIFile_iface;
  IPersistFile      IPersistFile_iface;
  IUnknown         *outer_unk;
  LONG              ref;

  AVIFILEINFOW      fInfo;
  IAVIStreamImpl   *ppStreams[MAX_AVISTREAMS];

  EXTRACHUNKS       fileextra;

  DWORD             dwMoviChunkPos;  /* some stuff for saving ... */
  DWORD             dwIdxChunkPos;
  DWORD             dwNextFramePos;
  DWORD             dwInitialFrames;

  MMCKINFO          ckLastRecord;
  AVIINDEXENTRY    *idxRecords;      /* won't be updated while loading */
  DWORD             nIdxRecords;     /* current fill level */
  DWORD             cbIdxRecords;    /* size of idxRecords */

  /* IPersistFile stuff ... */
  HMMIO             hmmio;
  LPWSTR            szFileName;
  UINT              uMode;
  BOOL              fDirty;
};

static inline IAVIFileImpl *impl_from_IUnknown(IUnknown *iface)
{
  return CONTAINING_RECORD(iface, IAVIFileImpl, IUnknown_inner);
}

static inline IAVIFileImpl *impl_from_IAVIFile(IAVIFile *iface)
{
  return CONTAINING_RECORD(iface, IAVIFileImpl, IAVIFile_iface);
}

static inline IAVIFileImpl *impl_from_IPersistFile(IPersistFile *iface)
{
  return CONTAINING_RECORD(iface, IAVIFileImpl, IPersistFile_iface);
}

/***********************************************************************/

static HRESULT AVIFILE_AddFrame(IAVIStreamImpl *This, DWORD ckid, DWORD size,
				DWORD offset, DWORD flags);
static HRESULT AVIFILE_AddRecord(IAVIFileImpl *This);
static DWORD   AVIFILE_ComputeMoviStart(IAVIFileImpl *This);
static void    AVIFILE_ConstructAVIStream(IAVIFileImpl *paf, DWORD nr,
					  const AVISTREAMINFOW *asi);
static void    AVIFILE_DestructAVIStream(IAVIStreamImpl *This);
static HRESULT AVIFILE_LoadFile(IAVIFileImpl *This);
static HRESULT AVIFILE_LoadIndex(const IAVIFileImpl *This, DWORD size, DWORD offset);
static HRESULT AVIFILE_ParseIndex(const IAVIFileImpl *This, AVIINDEXENTRY *lp,
				  LONG count, DWORD pos, BOOL *bAbsolute);
static HRESULT AVIFILE_ReadBlock(IAVIStreamImpl *This, DWORD start,
				 LPVOID buffer, DWORD size);
static void    AVIFILE_SamplesToBlock(const IAVIStreamImpl *This, LPLONG pos,
				      LPLONG offset);
static HRESULT AVIFILE_SaveFile(IAVIFileImpl *This);
static HRESULT AVIFILE_SaveIndex(const IAVIFileImpl *This);
static ULONG   AVIFILE_SearchStream(const IAVIFileImpl *This, DWORD fccType,
				    LONG lSkip);
static void    AVIFILE_UpdateInfo(IAVIFileImpl *This);
static HRESULT AVIFILE_WriteBlock(IAVIStreamImpl *This, DWORD block,
				  FOURCC ckid, DWORD flags, LPCVOID buffer,
				  LONG size);

static HRESULT WINAPI IUnknown_fnQueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
  IAVIFileImpl *This = impl_from_IUnknown(iface);

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);

  if (!ppv) {
    WARN("invalid parameter\n");
    return E_INVALIDARG;
  }
  *ppv = NULL;

  if (IsEqualIID(riid, &IID_IUnknown))
    *ppv = &This->IUnknown_inner;
  else if (IsEqualIID(riid, &IID_IAVIFile))
    *ppv = &This->IAVIFile_iface;
  else if (IsEqualGUID(riid, &IID_IPersistFile))
    *ppv = &This->IPersistFile_iface;
  else {
    WARN("unknown IID %s\n", debugstr_guid(riid));
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
  UINT i;

  TRACE("(%p) ref=%ld\n", This, ref);

  if (!ref) {
    if (This->fDirty)
      AVIFILE_SaveFile(This);

    for (i = 0; i < This->fInfo.dwStreams; i++) {
      if (This->ppStreams[i] != NULL) {
        if (This->ppStreams[i]->ref != 0)
          ERR(": someone has still %lu reference to stream %u (%p)!\n",
              This->ppStreams[i]->ref, i, This->ppStreams[i]);
        AVIFILE_DestructAVIStream(This->ppStreams[i]);
        free(This->ppStreams[i]);
        This->ppStreams[i] = NULL;
      }
    }

    if (This->idxRecords != NULL) {
      free(This->idxRecords);
      This->idxRecords  = NULL;
      This->nIdxRecords = 0;
    }

    if (This->fileextra.lp != NULL) {
      free(This->fileextra.lp);
      This->fileextra.lp = NULL;
      This->fileextra.cb = 0;
    }

    free(This->szFileName);
    This->szFileName = NULL;

    if (This->hmmio != NULL) {
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

static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile *iface, REFIID riid, void **ppv)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
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

  AVIFILE_UpdateInfo(This);

  memcpy(afi, &This->fInfo, min((DWORD)size, sizeof(This->fInfo)));

  if ((DWORD)size < sizeof(This->fInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnGetStream(IAVIFile *iface, IAVIStream **avis, DWORD fccType,
    LONG lParam)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);
  ULONG nStream;

  TRACE("(%p,%p,0x%08lX,%ld)\n", iface, avis, fccType, lParam);

  if (avis == NULL || lParam < 0)
    return AVIERR_BADPARAM;

  nStream = AVIFILE_SearchStream(This, fccType, lParam);

  /* Does the requested stream exist? */
  if (nStream < This->fInfo.dwStreams &&
      This->ppStreams[nStream] != NULL) {
    *avis = &This->ppStreams[nStream]->IAVIStream_iface;
    IAVIStream_AddRef(*avis);

    return AVIERR_OK;
  }

  /* Sorry, but the specified stream doesn't exist */
  *avis = NULL;
  return AVIERR_NODATA;
}

static HRESULT WINAPI IAVIFile_fnCreateStream(IAVIFile *iface, IAVIStream **avis,
    AVISTREAMINFOW *asi)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);
  DWORD n;

  TRACE("(%p,%p,%p)\n", iface, avis, asi);

  /* check parameters */
  if (avis == NULL || asi == NULL)
    return AVIERR_BADPARAM;

  *avis = NULL;

  /* Does the user have write permission? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* Can we add another stream? */
  n = This->fInfo.dwStreams;
  if (n >= MAX_AVISTREAMS || This->dwMoviChunkPos != 0) {
    /* already reached max nr of streams
     * or have already written frames to disk */
    return AVIERR_UNSUPPORTED;
  }

  /* check AVISTREAMINFO for some really needed things */
  if (asi->fccType == 0 || asi->dwScale == 0 || asi->dwRate == 0)
    return AVIERR_BADFORMAT;

  /* now it seems to be save to add the stream */
  assert(This->ppStreams[n] == NULL);
  This->ppStreams[n] = calloc(1, sizeof(IAVIStreamImpl));
  if (This->ppStreams[n] == NULL)
    return AVIERR_MEMORY;

  /* initialize the new allocated stream */
  AVIFILE_ConstructAVIStream(This, n, asi);

  This->fInfo.dwStreams++;
  This->fDirty = TRUE;

  /* update our AVIFILEINFO structure */
  AVIFILE_UpdateInfo(This);

  /* return it */
  *avis = &This->ppStreams[n]->IAVIStream_iface;
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

  return WriteExtraChunk(&This->fileextra, ckid, lpData, size);
}

static HRESULT WINAPI IAVIFile_fnReadData(IAVIFile *iface, DWORD ckid, void *lpData, LONG *size)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p,0x%08lX,%p,%p)\n", iface, ckid, lpData, size);

  return ReadExtraChunk(&This->fileextra, ckid, lpData, size);
}

static HRESULT WINAPI IAVIFile_fnEndRecord(IAVIFile *iface)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);

  TRACE("(%p)\n",iface);

  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  This->fDirty = TRUE;

  /* no frames written to any stream? -- compute start of 'movi'-chunk */
  if (This->dwMoviChunkPos == 0)
    AVIFILE_ComputeMoviStart(This);

  This->fInfo.dwFlags  |= AVIFILEINFO_ISINTERLEAVED;

  /* already written frames to any stream, ... */
  if (This->ckLastRecord.dwFlags & MMIO_DIRTY) {
    /* close last record */
    if (mmioAscend(This->hmmio, &This->ckLastRecord, 0) != 0)
      return AVIERR_FILEWRITE;

    AVIFILE_AddRecord(This);

    if (This->fInfo.dwSuggestedBufferSize < This->ckLastRecord.cksize + 3 * sizeof(DWORD))
      This->fInfo.dwSuggestedBufferSize = This->ckLastRecord.cksize + 3 * sizeof(DWORD);
  }

  /* write out a new record into file, but don't close it */
  This->ckLastRecord.cksize  = 0;
  This->ckLastRecord.fccType = listtypeAVIRECORD;
  if (mmioSeek(This->hmmio, This->dwNextFramePos, SEEK_SET) == -1)
    return AVIERR_FILEWRITE;
  if (mmioCreateChunk(This->hmmio, &This->ckLastRecord, MMIO_CREATELIST) != 0)
    return AVIERR_FILEWRITE;
  This->dwNextFramePos += 3 * sizeof(DWORD);

  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnDeleteStream(IAVIFile *iface, DWORD fccType, LONG lParam)
{
  IAVIFileImpl *This = impl_from_IAVIFile(iface);
  ULONG nStream;

  TRACE("(%p,0x%08lX,%ld)\n", iface, fccType, lParam);

  /* check parameter */
  if (lParam < 0)
    return AVIERR_BADPARAM;

  /* Have user write permissions? */
  if ((This->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  nStream = AVIFILE_SearchStream(This, fccType, lParam);

  /* Does the requested stream exist? */
  if (nStream < This->fInfo.dwStreams &&
      This->ppStreams[nStream] != NULL) {
    /* ... so delete it now */
    free(This->ppStreams[nStream]);
    This->fInfo.dwStreams--;
    if (nStream < This->fInfo.dwStreams)
      memmove(&This->ppStreams[nStream], &This->ppStreams[nStream + 1],
             (This->fInfo.dwStreams - nStream) * sizeof(This->ppStreams[0]));

    This->ppStreams[This->fInfo.dwStreams] = NULL;
    This->fDirty = TRUE;

    /* This->fInfo will be updated further when asked for */
    return AVIERR_OK;
  } else
    return AVIERR_NODATA;
}

static const struct IAVIFileVtbl avif_vt = {
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


static HRESULT WINAPI IPersistFile_fnQueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
  IAVIFileImpl *This = impl_from_IPersistFile(iface);

  return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile *iface)
{
  IAVIFileImpl *This = impl_from_IPersistFile(iface);

  return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI IPersistFile_fnRelease(IPersistFile *iface)
{
  IAVIFileImpl *This = impl_from_IPersistFile(iface);

  return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile *iface, LPCLSID pClassID)
{
  TRACE("(%p,%p)\n", iface, pClassID);

  if (pClassID == NULL)
    return AVIERR_BADPARAM;

  *pClassID = CLSID_AVIFile;

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
  int len;

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

    len = WideCharToMultiByte(CP_ACP, 0, This->szFileName, -1, NULL, 0, NULL, NULL);
    szFileName = malloc(len * sizeof(CHAR));
    if (szFileName == NULL)
      return AVIERR_MEMORY;

    WideCharToMultiByte(CP_ACP, 0, This->szFileName, -1, szFileName, len, NULL, NULL);

    This->hmmio = mmioOpenA(szFileName, NULL, MMIO_ALLOCBUF | dwMode);
    free(szFileName);
    if (This->hmmio == NULL)
      return AVIERR_FILEOPEN;
  }

  /* should we create a new file? */
  if (dwMode & OF_CREATE) {
    memset(& This->fInfo, 0, sizeof(This->fInfo));
    This->fInfo.dwFlags = AVIFILEINFO_HASINDEX | AVIFILEINFO_TRUSTCKTYPE;

    return AVIERR_OK;
  } else
    return AVIFILE_LoadFile(This);
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile *iface, LPCOLESTR pszFileName,
    BOOL fRemember)
{
  TRACE("(%p,%s,%d)\n", iface, debugstr_w(pszFileName), fRemember);

  /* We write directly to disk, so nothing to do. */

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile *iface, LPCOLESTR pszFileName)
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

  if (This->szFileName != NULL) {
    int len = lstrlenW(This->szFileName) + 1;

    *ppszFileName = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*ppszFileName == NULL)
      return AVIERR_MEMORY;

    lstrcpyW(*ppszFileName, This->szFileName);
  }

  return AVIERR_OK;
}

static const struct IPersistFileVtbl pf_vt = {
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

HRESULT AVIFILE_CreateAVIFile(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
  IAVIFileImpl *obj;
  HRESULT hr;

  *ppv = NULL;
  obj = calloc(1, sizeof(IAVIFileImpl));
  if (!obj)
    return AVIERR_MEMORY;

  obj->IUnknown_inner.lpVtbl = &unk_vtbl;
  obj->IAVIFile_iface.lpVtbl = &avif_vt;
  obj->IPersistFile_iface.lpVtbl = &pf_vt;
  obj->ref = 1;
  if (pUnkOuter)
    obj->outer_unk = pUnkOuter;
  else
    obj->outer_unk = &obj->IUnknown_inner;

  hr = IUnknown_QueryInterface(&obj->IUnknown_inner, riid, ppv);
  IUnknown_Release(&obj->IUnknown_inner);

  return hr;
}


static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream *iface, REFIID riid, void **ppv)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);

  if (!ppv) {
    WARN("invalid parameter\n");
    return E_INVALIDARG;
  }
  *ppv = NULL;

  if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IAVIStream, riid)) {
    *ppv = iface;
    IAVIStream_AddRef(iface);

    return S_OK;
  }
  /* FIXME: IAVIStreaming interface */

  return E_NOINTERFACE;
}

static ULONG WINAPI IAVIStream_fnAddRef(IAVIStream *iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) ref=%ld\n", This, ref);

  /* also add ref to parent, so that it doesn't kill us */
  if (This->paf != NULL && ref == 1)
    IAVIFile_AddRef(&This->paf->IAVIFile_iface);

  return ref;
}

static ULONG WINAPI IAVIStream_fnRelease(IAVIStream *iface)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) ref=%ld\n", This, ref);

  if (This->paf != NULL && ref == 0)
    IAVIFile_Release(&This->paf->IAVIFile_iface);

  return ref;
}

static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream *iface, LPARAM lParam1, LPARAM lParam2)
{
  TRACE("(%p,0x%08IX,0x%08IX)\n", iface, lParam1, lParam2);

  /* This IAVIStream interface needs an AVIFile */
  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream *iface, AVISTREAMINFOW *psi, LONG size)
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

static LONG WINAPI IAVIStream_fnFindSample(IAVIStream *iface, LONG pos, LONG flags)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  LONG offset = 0;

  TRACE("(%p,%ld,0x%08lX)\n",iface,pos,flags);

  if (flags & FIND_FROM_START) {
    pos = This->sInfo.dwStart;
    flags &= ~(FIND_FROM_START|FIND_PREV);
    flags |= FIND_NEXT;
  }

  if (This->sInfo.dwSampleSize != 0) {
    /* convert samples into block number with offset */
    AVIFILE_SamplesToBlock(This, &pos, &offset);
  }

  if (flags & FIND_TYPE) {
    if (flags & FIND_KEY) {
      while (0 <= pos && pos <= This->lLastFrame) {
	if (This->idxFrames[pos].dwFlags & AVIIF_KEYFRAME)
	  goto RETURN_FOUND;

	if (flags & FIND_NEXT)
	  pos++;
	else
	  pos--;
      };
    } else if (flags & FIND_ANY) {
      while (0 <= pos && pos <= This->lLastFrame) {
	if (This->idxFrames[pos].dwChunkLength > 0)
	  goto RETURN_FOUND;

	if (flags & FIND_NEXT)
	  pos++;
	else
	  pos--;

      };
    } else if ((flags & FIND_FORMAT) && This->idxFmtChanges != NULL &&
	       This->sInfo.fccType == streamtypeVIDEO) {
      if (flags & FIND_NEXT) {
	ULONG n;

	for (n = 0; n < This->sInfo.dwFormatChangeCount; n++)
	  if (This->idxFmtChanges[n].ckid >= pos) {
            pos = This->idxFmtChanges[n].ckid;
	    goto RETURN_FOUND;
          }
      } else {
	LONG n;

	for (n = (LONG)This->sInfo.dwFormatChangeCount; n >= 0; n--) {
	  if (This->idxFmtChanges[n].ckid <= pos) {
            pos = This->idxFmtChanges[n].ckid;
	    goto RETURN_FOUND;
          }
	}

	if (pos > (LONG)This->sInfo.dwStart)
	  return 0; /* format changes always for first frame */
      }
    }

    return -1;
  }

 RETURN_FOUND:
  if (pos < (LONG)This->sInfo.dwStart)
    return -1;

  switch (flags & FIND_RET) {
  case FIND_LENGTH:
    /* physical size */
    pos = This->idxFrames[pos].dwChunkLength;
    break;
  case FIND_OFFSET:
    /* physical position */
    pos = This->idxFrames[pos].dwChunkOffset + 2 * sizeof(DWORD)
      + offset * This->sInfo.dwSampleSize;
    break;
  case FIND_SIZE:
    /* logical size */
    if (This->sInfo.dwSampleSize)
      pos = This->sInfo.dwSampleSize;
    else
      pos = 1;
    break;
  case FIND_INDEX:
    FIXME(": FIND_INDEX flag is not supported!\n");
    /* This is an index in the index-table on disc. */
    break;
  }; /* else logical position */

  return pos;
}

static HRESULT WINAPI IAVIStream_fnReadFormat(IAVIStream *iface, LONG pos, void *format,
    LONG *formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,%ld,%p,%p)\n", iface, pos, format, formatsize);

  if (formatsize == NULL)
    return AVIERR_BADPARAM;

  /* only interested in needed buffersize? */
  if (format == NULL || *formatsize <= 0) {
    *formatsize = This->cbFormat;

    return AVIERR_OK;
  }

  /* copy initial format (only as much as will fit) */
  memcpy(format, This->lpFormat, min(*(DWORD*)formatsize, This->cbFormat));
  if (*(DWORD*)formatsize < This->cbFormat) {
    *formatsize = This->cbFormat;
    return AVIERR_BUFFERTOOSMALL;
  }

  /* Could format change? When yes will it change? */
  if ((This->sInfo.dwFlags & AVISTREAMINFO_FORMATCHANGES) &&
      pos > This->sInfo.dwStart) {
    LONG lLastFmt;

    lLastFmt = IAVIStream_fnFindSample(iface, pos, FIND_FORMAT|FIND_PREV);
    if (lLastFmt > 0) {
      FIXME(": need to read formatchange for %ld -- unimplemented!\n",lLastFmt);
    }
  }

  *formatsize = This->cbFormat;
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIStream_fnSetFormat(IAVIStream *iface, LONG pos, void *format,
    LONG formatsize)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  BITMAPINFOHEADER *lpbiNew = format;

  TRACE("(%p,%ld,%p,%ld)\n", iface, pos, format, formatsize);

  /* check parameters */
  if (format == NULL || formatsize <= 0)
    return AVIERR_BADPARAM;

  /* Do we have write permission? */
  if ((This->paf->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* can only set format before frame is written! */
  if (This->lLastFrame > pos)
    return AVIERR_UNSUPPORTED;

  /* initial format or a formatchange? */
  if (This->lpFormat == NULL) {
    /* initial format */
    if (This->paf->dwMoviChunkPos != 0)
      return AVIERR_ERROR; /* user has used API in wrong sequence! */

    This->lpFormat = malloc(formatsize);
    if (This->lpFormat == NULL)
      return AVIERR_MEMORY;
    This->cbFormat = formatsize;

    memcpy(This->lpFormat, format, formatsize);

    /* update some infos about stream */
    if (This->sInfo.fccType == streamtypeVIDEO) {
      LONG lDim;

      lDim = This->sInfo.rcFrame.right - This->sInfo.rcFrame.left;
      if (lDim < lpbiNew->biWidth)
	This->sInfo.rcFrame.right = This->sInfo.rcFrame.left + lpbiNew->biWidth;
      lDim = This->sInfo.rcFrame.bottom - This->sInfo.rcFrame.top;
      if (lDim < lpbiNew->biHeight)
	This->sInfo.rcFrame.bottom = This->sInfo.rcFrame.top + lpbiNew->biHeight;
    } else if (This->sInfo.fccType == streamtypeAUDIO)
      This->sInfo.dwSampleSize = ((LPWAVEFORMATEX)This->lpFormat)->nBlockAlign;

    return AVIERR_OK;
  } else {
    MMCKINFO           ck;
    LPBITMAPINFOHEADER lpbiOld = This->lpFormat;
    RGBQUAD           *rgbNew  = (RGBQUAD*)((LPBYTE)lpbiNew + lpbiNew->biSize);
    AVIPALCHANGE      *lppc = NULL;
    UINT               n;

    /* perhaps format change, check it ... */
    if (This->cbFormat != formatsize)
      return AVIERR_UNSUPPORTED;

    /* no format change, only the initial one */
    if (memcmp(This->lpFormat, format, formatsize) == 0)
      return AVIERR_OK;

    /* check that's only the palette, which changes */
    if (lpbiOld->biSize        != lpbiNew->biSize ||
	lpbiOld->biWidth       != lpbiNew->biWidth ||
	lpbiOld->biHeight      != lpbiNew->biHeight ||
	lpbiOld->biPlanes      != lpbiNew->biPlanes ||
	lpbiOld->biBitCount    != lpbiNew->biBitCount ||
	lpbiOld->biCompression != lpbiNew->biCompression ||
	lpbiOld->biClrUsed     != lpbiNew->biClrUsed)
      return AVIERR_UNSUPPORTED;

    This->sInfo.dwFlags |= AVISTREAMINFO_FORMATCHANGES;

    /* simply say all colors have changed */
    ck.ckid   = MAKEAVICKID(cktypePALchange, This->nStream);
    ck.cksize = 2 * sizeof(WORD) + lpbiOld->biClrUsed * sizeof(PALETTEENTRY);
    lppc = malloc(ck.cksize);
    if (lppc == NULL)
      return AVIERR_MEMORY;

    lppc->bFirstEntry = 0;
    lppc->bNumEntries = (lpbiOld->biClrUsed < 256 ? lpbiOld->biClrUsed : 0);
    lppc->wFlags      = 0;
    for (n = 0; n < lpbiOld->biClrUsed; n++) {
      lppc->peNew[n].peRed   = rgbNew[n].rgbRed;
      lppc->peNew[n].peGreen = rgbNew[n].rgbGreen;
      lppc->peNew[n].peBlue  = rgbNew[n].rgbBlue;
      lppc->peNew[n].peFlags = 0;
    }

    if (mmioSeek(This->paf->hmmio, This->paf->dwNextFramePos, SEEK_SET) == -1 ||
        mmioCreateChunk(This->paf->hmmio, &ck, 0) != S_OK ||
        mmioWrite(This->paf->hmmio, (HPSTR)lppc, ck.cksize) != ck.cksize ||
        mmioAscend(This->paf->hmmio, &ck, 0) != S_OK)
    {
      free(lppc);
      return AVIERR_FILEWRITE;
    }

    This->paf->dwNextFramePos += ck.cksize + 2 * sizeof(DWORD);

    free(lppc);

    return AVIFILE_AddFrame(This, cktypePALchange, n, ck.dwDataOffset, 0);
  }
}

static HRESULT WINAPI IAVIStream_fnRead(IAVIStream *iface, LONG start, LONG samples, void *buffer,
    LONG buffersize, LONG *bytesread, LONG *samplesread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  DWORD size;
  HRESULT hr;

  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", iface, start, samples, buffer,
 	buffersize, bytesread, samplesread);

  /* clear return parameters if given */
  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;

  /* check parameters */
  if ((LONG)This->sInfo.dwStart > start)
    return AVIERR_NODATA; /* couldn't read before start of stream */
  if (This->sInfo.dwStart + This->sInfo.dwLength < (DWORD)start)
    return AVIERR_NODATA; /* start is past end of stream */

  /* should we read as much as possible? */
  if (samples == -1) {
    /* User should know how much we have read */
    if (bytesread == NULL && samplesread == NULL)
      return AVIERR_BADPARAM;

    if (This->sInfo.dwSampleSize != 0)
      samples = buffersize / This->sInfo.dwSampleSize;
    else
      samples = 1;
  }

  /* limit to end of stream */
  if ((LONG)This->sInfo.dwLength < samples)
    samples = This->sInfo.dwLength;
  if ((start - This->sInfo.dwStart) > (This->sInfo.dwLength - samples))
    samples = This->sInfo.dwLength - (start - This->sInfo.dwStart);

  /* nothing to read? Then leave ... */
  if (samples == 0)
    return AVIERR_OK;

  if (This->sInfo.dwSampleSize != 0) {
    /* fixed samplesize -- we can read over frame/block boundaries */
    LONG block = start;
    LONG offset = 0;

    if (!buffer)
    {
      if (bytesread)
        *bytesread = samples*This->sInfo.dwSampleSize;
      if (samplesread)
        *samplesread = samples;
      return AVIERR_OK;
    }

    /* convert start sample to block,offset pair */
    AVIFILE_SamplesToBlock(This, &block, &offset);

    /* convert samples to bytes */
    samples *= This->sInfo.dwSampleSize;

    while (samples > 0 && buffersize > 0) {
      LONG blocksize;
      if (block != This->dwCurrentFrame) {
	hr = AVIFILE_ReadBlock(This, block, NULL, 0);
	if (FAILED(hr))
	  return hr;
      }

      size = min((DWORD)samples, (DWORD)buffersize);
      blocksize = This->lpBuffer[1];
      TRACE("blocksize = %lu\n",blocksize);
      size = min(size, blocksize - offset);
      memcpy(buffer, ((BYTE*)&This->lpBuffer[2]) + offset, size);

      block++;
      offset = 0;
      buffer = ((LPBYTE)buffer)+size;
      samples    -= size;
      buffersize -= size;

      /* fill out return parameters if given */
      if (bytesread != NULL)
	*bytesread   += size;
      if (samplesread != NULL)
	*samplesread += size / This->sInfo.dwSampleSize;
    }

    if (samples == 0)
      return AVIERR_OK;
    else
      return AVIERR_BUFFERTOOSMALL;
  } else {
    /* variable samplesize -- we can only read one full frame/block */
    if (samples > 1)
      samples = 1;

    assert(start <= This->lLastFrame);
    size = This->idxFrames[start].dwChunkLength;
    if (buffer != NULL && buffersize >= size) {
      hr = AVIFILE_ReadBlock(This, start, buffer, size);
      if (FAILED(hr))
	return hr;
    } else if (buffer != NULL)
      return AVIERR_BUFFERTOOSMALL;

    /* fill out return parameters if given */
    if (bytesread != NULL)
      *bytesread = size;
    if (samplesread != NULL)
      *samplesread = samples;

    return AVIERR_OK;
  }
}

static HRESULT WINAPI IAVIStream_fnWrite(IAVIStream *iface, LONG start, LONG samples, void *buffer,
    LONG buffersize, DWORD flags, LONG *sampwritten, LONG *byteswritten)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);
  FOURCC ckid;
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

  /* Have we write permission? */
  if ((This->paf->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  switch (This->sInfo.fccType) {
  case streamtypeAUDIO:
    ckid = MAKEAVICKID(cktypeWAVEbytes, This->nStream);
    break;
  default:
    if ((flags & AVIIF_KEYFRAME) && buffersize != 0)
      ckid = MAKEAVICKID(cktypeDIBbits, This->nStream);
    else
      ckid = MAKEAVICKID(cktypeDIBcompressed, This->nStream);
    break;
  };

  /* append to end of stream? */
  if (start == -1) {
    if (This->lLastFrame == -1)
      start = This->sInfo.dwStart;
    else
      start = This->sInfo.dwLength;
  } else if (This->lLastFrame == -1)
    This->sInfo.dwStart = start;

  if (This->sInfo.dwSampleSize != 0) {
    /* fixed sample size -- audio like */
    if (samples * This->sInfo.dwSampleSize != buffersize)
      return AVIERR_BADPARAM;

    /* Couldn't skip audio-like data -- User must supply appropriate silence */
    if (This->sInfo.dwLength != start)
      return AVIERR_UNSUPPORTED;

    /* Convert position to frame/block */
    start = This->lLastFrame + 1;

    if ((This->paf->fInfo.dwFlags & AVIFILEINFO_ISINTERLEAVED) == 0) {
      FIXME(": not interleaved, could collect audio data!\n");
    }
  } else {
    /* variable sample size -- video like */
    if (samples > 1)
      return AVIERR_UNSUPPORTED;

    /* must we fill up with empty frames? */
    if (This->lLastFrame != -1) {
      FOURCC ckid2 = MAKEAVICKID(cktypeDIBcompressed, This->nStream);

      while (start > This->lLastFrame + 1) {
	hr = AVIFILE_WriteBlock(This, This->lLastFrame + 1, ckid2, 0, NULL, 0);
	if (FAILED(hr))
	  return hr;
      }
    }
  }

  /* write the block now */
  hr = AVIFILE_WriteBlock(This, start, ckid, flags, buffer, buffersize);
  if (SUCCEEDED(hr)) {
    /* fill out return parameters if given */
    if (sampwritten != NULL)
      *sampwritten = samples;
    if (byteswritten != NULL)
      *byteswritten = buffersize;
  }

  return hr;
}

static HRESULT WINAPI IAVIStream_fnDelete(IAVIStream *iface, LONG start, LONG samples)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  FIXME("(%p,%ld,%ld): stub\n", iface, start, samples);

  /* check parameters */
  if (start < 0 || samples < 0)
    return AVIERR_BADPARAM;

  /* Delete before start of stream? */
  if (start + samples < This->sInfo.dwStart)
    return AVIERR_OK;

  /* Delete after end of stream? */
  if (start > This->sInfo.dwLength)
    return AVIERR_OK;

  /* For the rest we need write permissions */
  if ((This->paf->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* 1. overwrite the data with JUNK
   *
   * if ISINTERLEAVED {
   *   2. concat all neighboured JUNK-blocks in this record to one
   *   3. if this record only contains JUNK and is at end set dwNextFramePos
   *      to start of this record, repeat this.
   * } else {
   *   2. concat all neighboured JUNK-blocks.
   *   3. if the JUNK block is at the end, then set dwNextFramePos to
   *      start of this block.
   * }
   */

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IAVIStream_fnReadData(IAVIStream *iface, DWORD fcc, void *lp, LONG *lpread)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lX,%p,%p)\n", iface, fcc, lp, lpread);

  if (fcc == ckidSTREAMHANDLERDATA) {
    if (This->lpHandlerData != NULL && This->cbHandlerData > 0) {
      if (lp == NULL || *lpread <= 0) {
	*lpread = This->cbHandlerData;
	return AVIERR_OK;
      }

      memcpy(lp, This->lpHandlerData, min(This->cbHandlerData, *lpread));
      if (*lpread < This->cbHandlerData)
	return AVIERR_BUFFERTOOSMALL;
      return AVIERR_OK;
    } else
      return AVIERR_NODATA;
  } else
    return ReadExtraChunk(&This->extra, fcc, lp, lpread);
}

static HRESULT WINAPI IAVIStream_fnWriteData(IAVIStream *iface, DWORD fcc, void *lp, LONG size)
{
  IAVIStreamImpl *This = impl_from_IAVIStream(iface);

  TRACE("(%p,0x%08lx,%p,%ld)\n", iface, fcc, lp, size);

  /* check parameters */
  if (lp == NULL)
    return AVIERR_BADPARAM;
  if (size <= 0)
    return AVIERR_BADSIZE;

  /* need write permission */
  if ((This->paf->uMode & MMIO_RWMODE) == 0)
    return AVIERR_READONLY;

  /* already written something to this file? */
  if (This->paf->dwMoviChunkPos != 0) {
    /* the data will be inserted before the 'movi' chunk, so check for
     * enough space */
    DWORD dwPos = AVIFILE_ComputeMoviStart(This->paf);

    /* ckid,size => 2 * sizeof(DWORD) */
    dwPos += 2 * sizeof(DWORD) + size;
    if (dwPos >= This->paf->dwMoviChunkPos - 2 * sizeof(DWORD))
      return AVIERR_UNSUPPORTED; /* not enough space left */
  }

  This->paf->fDirty = TRUE;

  if (fcc == ckidSTREAMHANDLERDATA) {
    if (This->lpHandlerData != NULL) {
      FIXME(": handler data already set -- overwrite?\n");
      return AVIERR_UNSUPPORTED;
    }

    This->lpHandlerData = malloc(size);
    if (This->lpHandlerData == NULL)
      return AVIERR_MEMORY;
    This->cbHandlerData = size;
    memcpy(This->lpHandlerData, lp, size);

    return AVIERR_OK;
  } else
    return WriteExtraChunk(&This->extra, fcc, lp, size);
}

static HRESULT WINAPI IAVIStream_fnSetInfo(IAVIStream *iface, AVISTREAMINFOW *info, LONG infolen)
{
  FIXME("(%p,%p,%ld): stub\n", iface, info, infolen);

  return E_FAIL;
}

static const struct IAVIStreamVtbl avist_vt = {
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


static HRESULT AVIFILE_AddFrame(IAVIStreamImpl *This, DWORD ckid, DWORD size, DWORD offset, DWORD flags)
{
  UINT n;

  /* pre-conditions */
  assert(This != NULL);

  switch (TWOCCFromFOURCC(ckid)) {
  case cktypeDIBbits:
    if (This->paf->fInfo.dwFlags & AVIFILEINFO_TRUSTCKTYPE)
      flags |= AVIIF_KEYFRAME;
    break;
  case cktypeDIBcompressed:
    if (This->paf->fInfo.dwFlags & AVIFILEINFO_TRUSTCKTYPE)
      flags &= ~AVIIF_KEYFRAME;
    break;
  case cktypePALchange:
    if (This->sInfo.fccType != streamtypeVIDEO) {
      ERR(": found palette change in non-video stream!\n");
      return AVIERR_BADFORMAT;
    }

    if (This->idxFmtChanges == NULL || This->nIdxFmtChanges <= This->sInfo.dwFormatChangeCount) {
      DWORD new_count = This->nIdxFmtChanges + 16;
      void *new_buffer = _recalloc(This->idxFmtChanges, new_count, sizeof(AVIINDEXENTRY));
      if (!new_buffer) return AVIERR_MEMORY;
      This->idxFmtChanges = new_buffer;
      This->nIdxFmtChanges = new_count;
    }

    This->sInfo.dwFlags |= AVISTREAMINFO_FORMATCHANGES;
    n = ++This->sInfo.dwFormatChangeCount;
    This->idxFmtChanges[n].ckid          = This->lLastFrame;
    This->idxFmtChanges[n].dwFlags       = 0;
    This->idxFmtChanges[n].dwChunkOffset = offset;
    This->idxFmtChanges[n].dwChunkLength = size;

    return AVIERR_OK;
  case cktypeWAVEbytes:
    if (This->paf->fInfo.dwFlags & AVIFILEINFO_TRUSTCKTYPE)
      flags |= AVIIF_KEYFRAME;
    break;
  default:
    WARN(": unknown TWOCC 0x%04X found\n", TWOCCFromFOURCC(ckid));
    break;
  };

  /* first frame is always a keyframe */
  if (This->lLastFrame == -1)
    flags |= AVIIF_KEYFRAME;

  if (This->sInfo.dwSuggestedBufferSize < size)
    This->sInfo.dwSuggestedBufferSize = size;

  /* get memory for index */
  if (This->idxFrames == NULL || This->lLastFrame + 1 >= This->nIdxFrames) {
    This->nIdxFrames += 512;
    This->idxFrames = _recalloc(This->idxFrames, This->nIdxFrames, sizeof(AVIINDEXENTRY));
    if (This->idxFrames == NULL)
      return AVIERR_MEMORY;
  }

  This->lLastFrame++;
  This->idxFrames[This->lLastFrame].ckid          = ckid;
  This->idxFrames[This->lLastFrame].dwFlags       = flags;
  This->idxFrames[This->lLastFrame].dwChunkOffset = offset;
  This->idxFrames[This->lLastFrame].dwChunkLength = size;

  /* update AVISTREAMINFO structure if necessary */
  if (This->sInfo.dwLength <= This->lLastFrame)
    This->sInfo.dwLength = This->lLastFrame + 1;

  return AVIERR_OK;
}

static HRESULT AVIFILE_AddRecord(IAVIFileImpl *This)
{
  /* pre-conditions */
  assert(This != NULL && This->ppStreams[0] != NULL);

  if (This->idxRecords == NULL || This->cbIdxRecords / sizeof(AVIINDEXENTRY) <= This->nIdxRecords) {
    DWORD new_count = This->cbIdxRecords + 1024 * sizeof(AVIINDEXENTRY);
    void *mem;
    mem = _recalloc(This->idxRecords, 1, new_count);
    if (mem) {
      This->cbIdxRecords = new_count;
      This->idxRecords = mem;
    } else {
      free(This->idxRecords);
      This->idxRecords = NULL;
      return AVIERR_MEMORY;
    }
  }

  assert(This->nIdxRecords < This->cbIdxRecords/sizeof(AVIINDEXENTRY));

  This->idxRecords[This->nIdxRecords].ckid          = listtypeAVIRECORD;
  This->idxRecords[This->nIdxRecords].dwFlags       = AVIIF_LIST;
  This->idxRecords[This->nIdxRecords].dwChunkOffset =
    This->ckLastRecord.dwDataOffset - 2 * sizeof(DWORD);
  This->idxRecords[This->nIdxRecords].dwChunkLength =
    This->ckLastRecord.cksize;
  This->nIdxRecords++;

  return AVIERR_OK;
}

static DWORD   AVIFILE_ComputeMoviStart(IAVIFileImpl *This)
{
  DWORD dwPos;
  DWORD nStream;

  /* RIFF,hdrl,movi,avih => (3 * 3 + 2) * sizeof(DWORD) = 11 * sizeof(DWORD) */
  dwPos = 11 * sizeof(DWORD) + sizeof(MainAVIHeader);

  for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
    IAVIStreamImpl *pStream = This->ppStreams[nStream];

    /* strl,strh,strf => (3 + 2 * 2) * sizeof(DWORD) = 7 * sizeof(DWORD) */
    dwPos += 7 * sizeof(DWORD) + sizeof(AVIStreamHeader);
    dwPos += ((pStream->cbFormat + 1) & ~1U);
    if (pStream->lpHandlerData != NULL && pStream->cbHandlerData > 0)
      dwPos += 2 * sizeof(DWORD) + ((pStream->cbHandlerData + 1) & ~1U);
    if (pStream->sInfo.szName[0])
      dwPos += 2 * sizeof(DWORD) + ((lstrlenW(pStream->sInfo.szName) + 1) & ~1U);
  }

  if (This->dwMoviChunkPos == 0) {
    This->dwNextFramePos = dwPos;

    /* pad to multiple of AVI_HEADERSIZE only if we are more than 8 bytes away from it */
    if (((dwPos + AVI_HEADERSIZE) & ~(AVI_HEADERSIZE - 1)) - dwPos > 2 * sizeof(DWORD))
      This->dwNextFramePos = (dwPos + AVI_HEADERSIZE) & ~(AVI_HEADERSIZE - 1);

    This->dwMoviChunkPos = This->dwNextFramePos - sizeof(DWORD);
  }

  return dwPos;
}

static void AVIFILE_ConstructAVIStream(IAVIFileImpl *paf, DWORD nr, const AVISTREAMINFOW *asi)
{
  IAVIStreamImpl *pstream;

  /* pre-conditions */
  assert(paf != NULL);
  assert(nr < MAX_AVISTREAMS);
  assert(paf->ppStreams[nr] != NULL);

  pstream = paf->ppStreams[nr];

  pstream->IAVIStream_iface.lpVtbl = &avist_vt;
  pstream->ref            = 0;
  pstream->paf            = paf;
  pstream->nStream        = nr;
  pstream->dwCurrentFrame = (DWORD)-1;
  pstream->lLastFrame    = -1;

  if (asi != NULL) {
    memcpy(&pstream->sInfo, asi, sizeof(pstream->sInfo));

    if (asi->dwLength > 0) {
      /* pre-allocate mem for frame-index structure */
      pstream->idxFrames = calloc(asi->dwLength, sizeof(AVIINDEXENTRY));
      if (pstream->idxFrames != NULL)
	pstream->nIdxFrames = asi->dwLength;
    }
    if (asi->dwFormatChangeCount > 0) {
      /* pre-allocate mem for formatchange-index structure */
      pstream->idxFmtChanges = calloc(asi->dwFormatChangeCount, sizeof(AVIINDEXENTRY));
      if (pstream->idxFmtChanges != NULL)
	pstream->nIdxFmtChanges = asi->dwFormatChangeCount;
    }

    /* These values will be computed */
    pstream->sInfo.dwLength              = 0;
    pstream->sInfo.dwSuggestedBufferSize = 0;
    pstream->sInfo.dwFormatChangeCount   = 0;
    pstream->sInfo.dwEditCount           = 1;
    if (pstream->sInfo.dwSampleSize > 0)
      SetRectEmpty(&pstream->sInfo.rcFrame);
  }

  pstream->sInfo.dwCaps = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
}

static void    AVIFILE_DestructAVIStream(IAVIStreamImpl *This)
{
  /* pre-conditions */
  assert(This != NULL);

  This->dwCurrentFrame = (DWORD)-1;
  This->lLastFrame    = -1;
  This->paf = NULL;
  if (This->idxFrames != NULL) {
    free(This->idxFrames);
    This->idxFrames  = NULL;
    This->nIdxFrames = 0;
  }
  free(This->idxFmtChanges);
  This->idxFmtChanges = NULL;
  if (This->lpBuffer != NULL) {
    free(This->lpBuffer);
    This->lpBuffer = NULL;
    This->cbBuffer = 0;
  }
  if (This->lpHandlerData != NULL) {
    free(This->lpHandlerData);
    This->lpHandlerData = NULL;
    This->cbHandlerData = 0;
  }
  if (This->extra.lp != NULL) {
    free(This->extra.lp);
    This->extra.lp = NULL;
    This->extra.cb = 0;
  }
  if (This->lpFormat != NULL) {
    free(This->lpFormat);
    This->lpFormat = NULL;
    This->cbFormat = 0;
  }
}

static HRESULT AVIFILE_LoadFile(IAVIFileImpl *This)
{
  MainAVIHeader   MainAVIHdr;
  MMCKINFO        ckRIFF;
  MMCKINFO        ckLIST1;
  MMCKINFO        ckLIST2;
  MMCKINFO        ck;
  IAVIStreamImpl *pStream;
  DWORD           nStream;
  HRESULT         hr;

  if (This->hmmio == NULL)
    return AVIERR_FILEOPEN;

  /* initialize stream ptr's */
  memset(This->ppStreams, 0, sizeof(This->ppStreams));

  /* try to get "RIFF" chunk -- must not be at beginning of file! */
  ckRIFF.fccType = formtypeAVI;
  if (mmioDescend(This->hmmio, &ckRIFF, NULL, MMIO_FINDRIFF) != S_OK) {
    ERR(": not an AVI!\n");
    return AVIERR_FILEREAD;
  }

  /* get "LIST" "hdrl" */
  ckLIST1.fccType = listtypeAVIHEADER;
  hr = FindChunkAndKeepExtras(&This->fileextra, This->hmmio, &ckLIST1, &ckRIFF, MMIO_FINDLIST);
  if (FAILED(hr))
    return hr;

  /* get "avih" chunk */
  ck.ckid = ckidAVIMAINHDR;
  hr = FindChunkAndKeepExtras(&This->fileextra, This->hmmio, &ck, &ckLIST1, MMIO_FINDCHUNK);
  if (FAILED(hr))
    return hr;

  if (ck.cksize != sizeof(MainAVIHdr)) {
    ERR(": invalid size of %ld for MainAVIHeader!\n", ck.cksize);
    return AVIERR_BADFORMAT;
  }
  if (mmioRead(This->hmmio, (HPSTR)&MainAVIHdr, ck.cksize) != ck.cksize)
    return AVIERR_FILEREAD;

  /* check for MAX_AVISTREAMS limit */
  if (MainAVIHdr.dwStreams > MAX_AVISTREAMS) {
    WARN("file contains %lu streams, but only supports %d -- change MAX_AVISTREAMS!\n", MainAVIHdr.dwStreams, MAX_AVISTREAMS);
    return AVIERR_UNSUPPORTED;
  }

  /* adjust permissions if copyrighted material in file */
  if (MainAVIHdr.dwFlags & AVIFILEINFO_COPYRIGHTED) {
    This->uMode &= ~MMIO_RWMODE;
    This->uMode |= MMIO_READ;
  }

  /* convert MainAVIHeader into AVIFILINFOW */
  memset(&This->fInfo, 0, sizeof(This->fInfo));
  This->fInfo.dwRate                = MainAVIHdr.dwMicroSecPerFrame;
  This->fInfo.dwScale               = 1000000;
  This->fInfo.dwMaxBytesPerSec      = MainAVIHdr.dwMaxBytesPerSec;
  This->fInfo.dwFlags               = MainAVIHdr.dwFlags;
  This->fInfo.dwCaps                = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
  This->fInfo.dwLength              = MainAVIHdr.dwTotalFrames;
  This->fInfo.dwStreams             = MainAVIHdr.dwStreams;
  This->fInfo.dwSuggestedBufferSize = 0;
  This->fInfo.dwWidth               = MainAVIHdr.dwWidth;
  This->fInfo.dwHeight              = MainAVIHdr.dwHeight;
  LoadStringW(AVIFILE_hModule, IDS_AVIFILETYPE, This->fInfo.szFileType,
	      ARRAY_SIZE(This->fInfo.szFileType));

  /* go back to into header list */
  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEREAD;

  /* foreach stream exists a "LIST","strl" chunk */
  for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
    /* get next nested chunk in this "LIST","strl" */
    if (mmioDescend(This->hmmio, &ckLIST2, &ckLIST1, 0) != S_OK)
      return AVIERR_FILEREAD;

    /* nested chunk must be of type "LIST","strl" -- when not normally JUNK */
    if (ckLIST2.ckid == FOURCC_LIST &&
	ckLIST2.fccType == listtypeSTREAMHEADER) {
      pStream = This->ppStreams[nStream] = calloc(1, sizeof(IAVIStreamImpl));
      if (pStream == NULL)
	return AVIERR_MEMORY;
      AVIFILE_ConstructAVIStream(This, nStream, NULL);

      ck.ckid = 0;
      while (mmioDescend(This->hmmio, &ck, &ckLIST2, 0) == S_OK) {
	switch (ck.ckid) {
	case ckidSTREAMHANDLERDATA:
	  if (pStream->lpHandlerData != NULL)
	    return AVIERR_BADFORMAT;
	  pStream->lpHandlerData = malloc(ck.cksize);
	  if (pStream->lpHandlerData == NULL)
	    return AVIERR_MEMORY;
	  pStream->cbHandlerData = ck.cksize;

          if (mmioRead(This->hmmio, pStream->lpHandlerData, ck.cksize) != ck.cksize)
	    return AVIERR_FILEREAD;
	  break;
	case ckidSTREAMFORMAT:
	  if (pStream->lpFormat != NULL)
	    return AVIERR_BADFORMAT;
          if (ck.cksize == 0)
            break;

	  pStream->lpFormat = malloc(ck.cksize);
	  if (pStream->lpFormat == NULL)
	    return AVIERR_MEMORY;
	  pStream->cbFormat = ck.cksize;

          if (mmioRead(This->hmmio, pStream->lpFormat, ck.cksize) != ck.cksize)
	    return AVIERR_FILEREAD;

	  if (pStream->sInfo.fccType == streamtypeVIDEO) {
            LPBITMAPINFOHEADER lpbi = pStream->lpFormat;

	    /* some corrections to the video format */
	    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
	      lpbi->biClrUsed = 1u << lpbi->biBitCount;
	    if (lpbi->biCompression == BI_RGB && lpbi->biSizeImage == 0)
	      lpbi->biSizeImage = DIBWIDTHBYTES(*lpbi) * lpbi->biHeight;
	    if (lpbi->biCompression != BI_RGB && lpbi->biBitCount == 8) {
	      if (pStream->sInfo.fccHandler == mmioFOURCC('R','L','E','0') ||
		  pStream->sInfo.fccHandler == mmioFOURCC('R','L','E',' '))
		lpbi->biCompression = BI_RLE8;
	    }
	    if (lpbi->biCompression == BI_RGB &&
		(pStream->sInfo.fccHandler == 0 ||
		 pStream->sInfo.fccHandler == mmioFOURCC('N','O','N','E')))
	      pStream->sInfo.fccHandler = comptypeDIB;

	    /* init rcFrame if it's empty */
	    if (IsRectEmpty(&pStream->sInfo.rcFrame))
	      SetRect(&pStream->sInfo.rcFrame, 0, 0, lpbi->biWidth, lpbi->biHeight);
	  }
	  break;
	case ckidSTREAMHEADER:
	  {
	    AVIStreamHeader streamHdr;
	    WCHAR           szType[25];
	    UINT            count;
	    LONG            n = ck.cksize;

	    if (ck.cksize > sizeof(streamHdr))
	      n = sizeof(streamHdr);

	    if (mmioRead(This->hmmio, (HPSTR)&streamHdr, n) != n)
	      return AVIERR_FILEREAD;

	    pStream->sInfo.fccType               = streamHdr.fccType;
	    pStream->sInfo.fccHandler            = streamHdr.fccHandler;
	    pStream->sInfo.dwFlags               = streamHdr.dwFlags;
	    pStream->sInfo.wPriority             = streamHdr.wPriority;
	    pStream->sInfo.wLanguage             = streamHdr.wLanguage;
	    pStream->sInfo.dwInitialFrames       = streamHdr.dwInitialFrames;
	    pStream->sInfo.dwScale               = streamHdr.dwScale;
	    pStream->sInfo.dwRate                = streamHdr.dwRate;
	    pStream->sInfo.dwStart               = streamHdr.dwStart;
	    pStream->sInfo.dwLength              = streamHdr.dwLength;
	    pStream->sInfo.dwSuggestedBufferSize = 0;
	    pStream->sInfo.dwQuality             = streamHdr.dwQuality;
	    pStream->sInfo.dwSampleSize          = streamHdr.dwSampleSize;
	    pStream->sInfo.rcFrame.left          = streamHdr.rcFrame.left;
	    pStream->sInfo.rcFrame.top           = streamHdr.rcFrame.top;
	    pStream->sInfo.rcFrame.right         = streamHdr.rcFrame.right;
	    pStream->sInfo.rcFrame.bottom        = streamHdr.rcFrame.bottom;
	    pStream->sInfo.dwEditCount           = 0;
	    pStream->sInfo.dwFormatChangeCount   = 0;

	    /* generate description for stream like "filename.avi Type #n" */
	    if (streamHdr.fccType == streamtypeVIDEO)
	      LoadStringW(AVIFILE_hModule, IDS_VIDEO, szType, ARRAY_SIZE(szType));
	    else if (streamHdr.fccType == streamtypeAUDIO)
	      LoadStringW(AVIFILE_hModule, IDS_AUDIO, szType, ARRAY_SIZE(szType));
	    else
              wsprintfW(szType, L"%4.4hs", (char*)&streamHdr.fccType);

	    /* get count of this streamtype up to this stream */
	    count = 0;
	    for (n = nStream; 0 <= n; n--) {
	      if (This->ppStreams[n]->sInfo.fccHandler == streamHdr.fccType)
		count++;
	    }

	    memset(pStream->sInfo.szName, 0, sizeof(pStream->sInfo.szName));

	    /* FIXME: avoid overflow -- better use wsnprintfW, which doesn't exists ! */
            wsprintfW(pStream->sInfo.szName, L"%s %s #%d",
		      AVIFILE_BasenameW(This->szFileName), szType, count);
	  }
	  break;
	case ckidSTREAMNAME:
	  { /* streamname will be saved as ASCII string */
	    char *str = malloc(ck.cksize);
	    if (str == NULL)
	      return AVIERR_MEMORY;

	    if (mmioRead(This->hmmio, str, ck.cksize) != ck.cksize)
	    {
	      free(str);
	      return AVIERR_FILEREAD;
	    }

	    MultiByteToWideChar(CP_ACP, 0, str, -1, pStream->sInfo.szName,
				ARRAY_SIZE(pStream->sInfo.szName));

	    free(str);
	  }
	  break;
	case ckidAVIPADDING:
	case mmioFOURCC('p','a','d','d'):
	  break;
	default:
          WARN(": found extra chunk 0x%08lX\n", ck.ckid);
	  hr = ReadChunkIntoExtra(&pStream->extra, This->hmmio, &ck);
	  if (FAILED(hr))
	    return hr;
	};
	if (pStream->lpFormat != NULL && pStream->sInfo.fccType == streamtypeAUDIO)
	{
	  WAVEFORMATEX *wfx = pStream->lpFormat;          /* wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample / 8; could be added */
	  pStream->sInfo.dwSampleSize = wfx->nBlockAlign; /* to deal with corrupt wfx->nBlockAlign but Windows doesn't do this */
	  TRACE("Block size reset to %u, chan=%u bpp=%u\n", wfx->nBlockAlign, wfx->nChannels, wfx->wBitsPerSample);
	  pStream->sInfo.dwScale = 1;
	  pStream->sInfo.dwRate = wfx->nSamplesPerSec;
	}
	if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
	  return AVIERR_FILEREAD;
      }
    } else {
      /* nested chunks in "LIST","hdrl" which are not of type "LIST","strl" */
      hr = ReadChunkIntoExtra(&This->fileextra, This->hmmio, &ckLIST2);
      if (FAILED(hr))
	return hr;
    }
    if (mmioAscend(This->hmmio, &ckLIST2, 0) != S_OK)
      return AVIERR_FILEREAD;
  }

  /* read any extra headers in "LIST","hdrl" */
  FindChunkAndKeepExtras(&This->fileextra, This->hmmio, &ck, &ckLIST1, 0);
  if (mmioAscend(This->hmmio, &ckLIST1, 0) != S_OK)
    return AVIERR_FILEREAD;

  /* search "LIST","movi" chunk in "RIFF","AVI " */
  ckLIST1.fccType = listtypeAVIMOVIE;
  hr = FindChunkAndKeepExtras(&This->fileextra, This->hmmio, &ckLIST1, &ckRIFF,
			      MMIO_FINDLIST);
  if (FAILED(hr))
    return hr;

  This->dwMoviChunkPos = ckLIST1.dwDataOffset;
  This->dwIdxChunkPos  = ckLIST1.cksize + ckLIST1.dwDataOffset;
  if (mmioAscend(This->hmmio, &ckLIST1, 0) != S_OK)
    return AVIERR_FILEREAD;

  /* try to find an index */
  ck.ckid = ckidAVINEWINDEX;
  hr = FindChunkAndKeepExtras(&This->fileextra, This->hmmio,
			      &ck, &ckRIFF, MMIO_FINDCHUNK);
  if (SUCCEEDED(hr) && ck.cksize > 0) {
    if (FAILED(AVIFILE_LoadIndex(This, ck.cksize, ckLIST1.dwDataOffset)))
      This->fInfo.dwFlags &= ~AVIFILEINFO_HASINDEX;
  }

  /* when we haven't found an index or it's bad, then build one
   * by parsing 'movi' chunk */
  if ((This->fInfo.dwFlags & AVIFILEINFO_HASINDEX) == 0) {
    for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++)
      This->ppStreams[nStream]->lLastFrame = -1;

    if (mmioSeek(This->hmmio, ckLIST1.dwDataOffset + sizeof(DWORD), SEEK_SET) == -1) {
      ERR(": Oops, can't seek back to 'movi' chunk!\n");
      return AVIERR_FILEREAD;
    }

    /* seek through the 'movi' list until end */
    while (mmioDescend(This->hmmio, &ck, &ckLIST1, 0) == S_OK) {
      if (ck.ckid != FOURCC_LIST) {
	if (mmioAscend(This->hmmio, &ck, 0) == S_OK) {
	  nStream = StreamFromFOURCC(ck.ckid);

	  if (nStream > This->fInfo.dwStreams)
	    return AVIERR_BADFORMAT;

	  AVIFILE_AddFrame(This->ppStreams[nStream], ck.ckid, ck.cksize,
			   ck.dwDataOffset - 2 * sizeof(DWORD), 0);
	} else {
	  nStream = StreamFromFOURCC(ck.ckid);
	  WARN(": file seems to be truncated!\n");
	  if (nStream <= This->fInfo.dwStreams &&
	      This->ppStreams[nStream]->sInfo.dwSampleSize > 0) {
	    ck.cksize = mmioSeek(This->hmmio, 0, SEEK_END);
	    if (ck.cksize != -1) {
	      ck.cksize -= ck.dwDataOffset;
	      ck.cksize &= ~(This->ppStreams[nStream]->sInfo.dwSampleSize - 1);

	      AVIFILE_AddFrame(This->ppStreams[nStream], ck.ckid, ck.cksize,
			       ck.dwDataOffset - 2 * sizeof(DWORD), 0);
	    }
	  }
	}
      }
    }
  }

  for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++)
  {
    DWORD sugbuf =  This->ppStreams[nStream]->sInfo.dwSuggestedBufferSize;
    if (This->fInfo.dwSuggestedBufferSize < sugbuf)
      This->fInfo.dwSuggestedBufferSize = sugbuf;
  }

  /* find other chunks */
  FindChunkAndKeepExtras(&This->fileextra, This->hmmio, &ck, &ckRIFF, 0);

  return AVIERR_OK;
}

static HRESULT AVIFILE_LoadIndex(const IAVIFileImpl *This, DWORD size, DWORD offset)
{
  AVIINDEXENTRY *lp;
  DWORD          pos, n;
  HRESULT        hr = AVIERR_OK;
  BOOL           bAbsolute = TRUE;

  lp = malloc(IDX_PER_BLOCK * sizeof(AVIINDEXENTRY));
  if (lp == NULL)
    return AVIERR_MEMORY;

  /* adjust limits for index tables, so that inserting will be faster */
  for (n = 0; n < This->fInfo.dwStreams; n++) {
    IAVIStreamImpl *pStream = This->ppStreams[n];

    pStream->lLastFrame = -1;

    if (pStream->idxFrames != NULL) {
      free(pStream->idxFrames);
      pStream->idxFrames  = NULL;
      pStream->nIdxFrames = 0;
    }

    if (pStream->sInfo.dwSampleSize != 0) {
      if (n > 0 && This->fInfo.dwFlags & AVIFILEINFO_ISINTERLEAVED) {
	pStream->nIdxFrames = This->ppStreams[0]->nIdxFrames;
      } else if (pStream->sInfo.dwSuggestedBufferSize) {
	pStream->nIdxFrames =
	  pStream->sInfo.dwLength / pStream->sInfo.dwSuggestedBufferSize;
      }
    } else
      pStream->nIdxFrames = pStream->sInfo.dwLength;

    pStream->idxFrames = calloc(pStream->nIdxFrames, sizeof(AVIINDEXENTRY));
    if (pStream->idxFrames == NULL && pStream->nIdxFrames > 0) {
      pStream->nIdxFrames = 0;
      free(lp);
      return AVIERR_MEMORY;
    }
  }

  pos = (DWORD)-1;
  while (size != 0) {
    LONG read = min(IDX_PER_BLOCK * sizeof(AVIINDEXENTRY), size);

    if (mmioRead(This->hmmio, (HPSTR)lp, read) != read) {
      hr = AVIERR_FILEREAD;
      break;
    }
    size -= read;

    if (pos == (DWORD)-1)
      pos = offset - lp->dwChunkOffset + sizeof(DWORD);

    AVIFILE_ParseIndex(This, lp, read / sizeof(AVIINDEXENTRY),
		       pos, &bAbsolute);
  }

  free(lp);

  /* checking ... */
  for (n = 0; n < This->fInfo.dwStreams; n++) {
    IAVIStreamImpl *pStream = This->ppStreams[n];

    if (pStream->sInfo.dwSampleSize == 0 &&
	pStream->sInfo.dwLength != pStream->lLastFrame+1)
      ERR("stream %lu length mismatch: dwLength=%lu found=%ld\n",
	   n, pStream->sInfo.dwLength, pStream->lLastFrame);
  }

  return hr;
}

static HRESULT AVIFILE_ParseIndex(const IAVIFileImpl *This, AVIINDEXENTRY *lp,
				  LONG count, DWORD pos, BOOL *bAbsolute)
{
  if (lp == NULL)
    return AVIERR_BADPARAM;

  for (; count > 0; count--, lp++) {
    WORD nStream = StreamFromFOURCC(lp->ckid);

    if (lp->ckid == listtypeAVIRECORD || nStream == 0x7F)
      continue; /* skip these */

    if (nStream > This->fInfo.dwStreams)
      return AVIERR_BADFORMAT;

    /* Video frames can be either indexed in a relative position to the
     * "movi" chunk or in a absolute position in the file. If the index
     * is relative the frame offset will always be so small that it will
     * virtually never reach the "movi" offset so we can detect if the
     * video is relative very fast.
     */
    if (*bAbsolute && lp->dwChunkOffset < This->dwMoviChunkPos)
      *bAbsolute = FALSE;

    if (!*bAbsolute)
      lp->dwChunkOffset += pos; /* make the offset absolute */

    if (FAILED(AVIFILE_AddFrame(This->ppStreams[nStream], lp->ckid, lp->dwChunkLength, lp->dwChunkOffset, lp->dwFlags)))
      return AVIERR_MEMORY;
  }

  return AVIERR_OK;
}

static HRESULT AVIFILE_ReadBlock(IAVIStreamImpl *This, DWORD pos,
				 LPVOID buffer, DWORD size)
{
  /* pre-conditions */
  assert(This != NULL);
  assert(This->paf != NULL);
  assert(This->paf->hmmio != NULL);
  assert(This->sInfo.dwStart <= pos && pos < This->sInfo.dwLength);
  assert(pos <= This->lLastFrame);

  /* should we read as much as block gives us? */
  if (size == 0 || size > This->idxFrames[pos].dwChunkLength)
    size = This->idxFrames[pos].dwChunkLength;

  /* read into out own buffer or given one? */
  if (buffer == NULL) {
    /* we also read the chunk */
    size += 2 * sizeof(DWORD);

    /* check that buffer is big enough -- don't trust dwSuggestedBufferSize */
    if (This->lpBuffer == NULL || This->cbBuffer < size) {
      DWORD maxSize = max(size, This->sInfo.dwSuggestedBufferSize);
      void *new_buffer = realloc(This->lpBuffer, maxSize);
      if (!new_buffer) return AVIERR_MEMORY;
      This->lpBuffer = new_buffer;
      This->cbBuffer = maxSize;
    }

    /* now read the complete chunk into our buffer */
    if (mmioSeek(This->paf->hmmio, This->idxFrames[pos].dwChunkOffset, SEEK_SET) == -1)
      return AVIERR_FILEREAD;
    if (mmioRead(This->paf->hmmio, (HPSTR)This->lpBuffer, size) != size)
      return AVIERR_FILEREAD;

    /* check if it was the correct block which we have read */
    if (This->lpBuffer[0] != This->idxFrames[pos].ckid ||
	This->lpBuffer[1] != This->idxFrames[pos].dwChunkLength) {
      ERR(": block %ld not found at 0x%08lX\n", pos, This->idxFrames[pos].dwChunkOffset);
      ERR(": Index says: '%4.4s'(0x%08lX) size 0x%08lX\n",
	  (char*)&This->idxFrames[pos].ckid, This->idxFrames[pos].ckid,
	  This->idxFrames[pos].dwChunkLength);
      ERR(": Data  says: '%4.4s'(0x%08lX) size 0x%08lX\n",
	  (char*)&This->lpBuffer[0], This->lpBuffer[0], This->lpBuffer[1]);
      return AVIERR_FILEREAD;
    }
  } else {
    if (mmioSeek(This->paf->hmmio, This->idxFrames[pos].dwChunkOffset + 2 * sizeof(DWORD), SEEK_SET) == -1)
      return AVIERR_FILEREAD;
    if (mmioRead(This->paf->hmmio, buffer, size) != size)
      return AVIERR_FILEREAD;
  }

  return AVIERR_OK;
}

static void AVIFILE_SamplesToBlock(const IAVIStreamImpl *This, LPLONG pos, LPLONG offset)
{
  LONG block;

  /* pre-conditions */
  assert(This != NULL);
  assert(pos != NULL);
  assert(offset != NULL);
  assert(This->sInfo.dwSampleSize != 0);
  assert(*pos >= This->sInfo.dwStart);

  /* convert start sample to start bytes */
  (*offset)  = (*pos) - This->sInfo.dwStart;
  (*offset) *= This->sInfo.dwSampleSize;

  /* convert bytes to block number */
  for (block = 0; block <= This->lLastFrame; block++) {
    if (This->idxFrames[block].dwChunkLength <= *offset)
      (*offset) -= This->idxFrames[block].dwChunkLength;
    else
      break;
  }

  *pos = block;
}

static HRESULT AVIFILE_SaveFile(IAVIFileImpl *This)
{
  MainAVIHeader   MainAVIHdr;
  IAVIStreamImpl* pStream;
  MMCKINFO        ckRIFF;
  MMCKINFO        ckLIST1;
  MMCKINFO        ckLIST2;
  MMCKINFO        ck;
  DWORD           nStream;
  DWORD           dwPos;
  HRESULT         hr;

  /* initialize some things */
  if (This->dwMoviChunkPos == 0)
    AVIFILE_ComputeMoviStart(This);

  /* written one record too much? */
  if (This->ckLastRecord.dwFlags & MMIO_DIRTY) {
    This->dwNextFramePos -= 3 * sizeof(DWORD);
    if (This->nIdxRecords > 0)
      This->nIdxRecords--;
  }

  AVIFILE_UpdateInfo(This);

  assert(This->fInfo.dwScale != 0);

  memset(&MainAVIHdr, 0, sizeof(MainAVIHdr));
  MainAVIHdr.dwMicroSecPerFrame    = MulDiv(This->fInfo.dwRate, 1000000,
					    This->fInfo.dwScale);
  MainAVIHdr.dwMaxBytesPerSec      = This->fInfo.dwMaxBytesPerSec;
  MainAVIHdr.dwPaddingGranularity  = AVI_HEADERSIZE;
  MainAVIHdr.dwFlags               = This->fInfo.dwFlags;
  MainAVIHdr.dwTotalFrames         = This->fInfo.dwLength;
  MainAVIHdr.dwInitialFrames       = 0;
  MainAVIHdr.dwStreams             = This->fInfo.dwStreams;
  MainAVIHdr.dwSuggestedBufferSize = This->fInfo.dwSuggestedBufferSize;
  MainAVIHdr.dwWidth               = This->fInfo.dwWidth;
  MainAVIHdr.dwHeight              = This->fInfo.dwHeight;
  MainAVIHdr.dwInitialFrames       = This->dwInitialFrames;

  /* now begin writing ... */
  mmioSeek(This->hmmio, 0, SEEK_SET);

  /* RIFF chunk */
  ckRIFF.cksize  = 0;
  ckRIFF.fccType = formtypeAVI;
  if (mmioCreateChunk(This->hmmio, &ckRIFF, MMIO_CREATERIFF) != S_OK)
    return AVIERR_FILEWRITE;

  /* AVI headerlist */
  ckLIST1.cksize  = 0;
  ckLIST1.fccType = listtypeAVIHEADER;
  if (mmioCreateChunk(This->hmmio, &ckLIST1, MMIO_CREATELIST) != S_OK)
    return AVIERR_FILEWRITE;

  /* MainAVIHeader */
  ck.ckid    = ckidAVIMAINHDR;
  ck.cksize  = sizeof(MainAVIHdr);
  ck.fccType = 0;
  if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;
  if (mmioWrite(This->hmmio, (HPSTR)&MainAVIHdr, ck.cksize) != ck.cksize)
    return AVIERR_FILEWRITE;
  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* write the headers of each stream into a separate streamheader list */
  for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
    AVIStreamHeader strHdr;

    pStream = This->ppStreams[nStream];

    /* begin the new streamheader list */
    ckLIST2.cksize  = 0;
    ckLIST2.fccType = listtypeSTREAMHEADER;
    if (mmioCreateChunk(This->hmmio, &ckLIST2, MMIO_CREATELIST) != S_OK)
      return AVIERR_FILEWRITE;

    /* create an AVIStreamHeader from the AVSTREAMINFO */
    strHdr.fccType               = pStream->sInfo.fccType;
    strHdr.fccHandler            = pStream->sInfo.fccHandler;
    strHdr.dwFlags               = pStream->sInfo.dwFlags;
    strHdr.wPriority             = pStream->sInfo.wPriority;
    strHdr.wLanguage             = pStream->sInfo.wLanguage;
    strHdr.dwInitialFrames       = pStream->sInfo.dwInitialFrames;
    strHdr.dwScale               = pStream->sInfo.dwScale;
    strHdr.dwRate                = pStream->sInfo.dwRate;
    strHdr.dwStart               = pStream->sInfo.dwStart;
    strHdr.dwLength              = pStream->sInfo.dwLength;
    strHdr.dwSuggestedBufferSize = pStream->sInfo.dwSuggestedBufferSize;
    strHdr.dwQuality             = pStream->sInfo.dwQuality;
    strHdr.dwSampleSize          = pStream->sInfo.dwSampleSize;
    strHdr.rcFrame.left          = pStream->sInfo.rcFrame.left;
    strHdr.rcFrame.top           = pStream->sInfo.rcFrame.top;
    strHdr.rcFrame.right         = pStream->sInfo.rcFrame.right;
    strHdr.rcFrame.bottom        = pStream->sInfo.rcFrame.bottom;

    /* now write the AVIStreamHeader */
    ck.ckid   = ckidSTREAMHEADER;
    ck.cksize = sizeof(strHdr);
    if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;
    if (mmioWrite(This->hmmio, (HPSTR)&strHdr, ck.cksize) != ck.cksize)
      return AVIERR_FILEWRITE;
    if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;

    /* ... the hopefully ever present streamformat ... */
    ck.ckid   = ckidSTREAMFORMAT;
    ck.cksize = pStream->cbFormat;
    if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;
    if (pStream->lpFormat != NULL && ck.cksize > 0) {
      if (mmioWrite(This->hmmio, pStream->lpFormat, ck.cksize) != ck.cksize)
	return AVIERR_FILEWRITE;
    }
    if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;

    /* ... some optional existing handler data ... */
    if (pStream->lpHandlerData != NULL && pStream->cbHandlerData > 0) {
      ck.ckid   = ckidSTREAMHANDLERDATA;
      ck.cksize = pStream->cbHandlerData;
      if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;
      if (mmioWrite(This->hmmio, pStream->lpHandlerData, ck.cksize) != ck.cksize)
	return AVIERR_FILEWRITE;
      if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;
    }

    /* ... some optional additional extra chunk for this stream ... */
    if (pStream->extra.lp != NULL && pStream->extra.cb > 0) {
      /* the chunk header(s) are already in the structure */
      if (mmioWrite(This->hmmio, pStream->extra.lp, pStream->extra.cb) != pStream->extra.cb)
	return AVIERR_FILEWRITE;
    }

    /* ... an optional name for this stream ... */
    if (pStream->sInfo.szName[0]) {
      LPSTR str;

      ck.ckid   = ckidSTREAMNAME;
      ck.cksize = lstrlenW(pStream->sInfo.szName) + 1;
      if (ck.cksize & 1) /* align */
	ck.cksize++;
      if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;

      /* the streamname must be saved in ASCII not Unicode */
      str = malloc(ck.cksize);
      if (str == NULL)
	return AVIERR_MEMORY;
      WideCharToMultiByte(CP_ACP, 0, pStream->sInfo.szName, -1, str,
			  ck.cksize, NULL, NULL);

      if (mmioWrite(This->hmmio, str, ck.cksize) != ck.cksize) {
	free(str);
	return AVIERR_FILEWRITE;
      }

      free(str);
      if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
	return AVIERR_FILEWRITE;
    }

    /* close streamheader list for this stream */
    if (mmioAscend(This->hmmio, &ckLIST2, 0) != S_OK)
      return AVIERR_FILEWRITE;
  } /* for (0 <= nStream < MainAVIHdr.dwStreams) */

  /* close the aviheader list */
  if (mmioAscend(This->hmmio, &ckLIST1, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* check for padding to pre-guessed 'movi'-chunk position */
  dwPos = ckLIST1.dwDataOffset + ckLIST1.cksize;
  if (This->dwMoviChunkPos - 2 * sizeof(DWORD) > dwPos) {
    ck.ckid   = ckidAVIPADDING;
    ck.cksize = This->dwMoviChunkPos - dwPos - 4 * sizeof(DWORD);
    assert((LONG)ck.cksize >= 0);

    if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;
    if (mmioSeek(This->hmmio, ck.cksize, SEEK_CUR) == -1)
      return AVIERR_FILEWRITE;
    if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
      return AVIERR_FILEWRITE;
  }

  /* now write the 'movi' chunk */
  mmioSeek(This->hmmio, This->dwMoviChunkPos - 2 * sizeof(DWORD), SEEK_SET);
  ckLIST1.cksize  = 0;
  ckLIST1.fccType = listtypeAVIMOVIE;
  if (mmioCreateChunk(This->hmmio, &ckLIST1, MMIO_CREATELIST) != S_OK)
    return AVIERR_FILEWRITE;
  if (mmioSeek(This->hmmio, This->dwNextFramePos, SEEK_SET) == -1)
    return AVIERR_FILEWRITE;
  if (mmioAscend(This->hmmio, &ckLIST1, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* write 'idx1' chunk */
  hr = AVIFILE_SaveIndex(This);
  if (FAILED(hr))
    return hr;

  /* write optional extra file chunks */
  if (This->fileextra.lp != NULL && This->fileextra.cb > 0) {
    /* as for the streams, are the chunk header(s) in the structure */
    if (mmioWrite(This->hmmio, This->fileextra.lp, This->fileextra.cb) != This->fileextra.cb)
      return AVIERR_FILEWRITE;
  }

  /* close RIFF chunk */
  if (mmioAscend(This->hmmio, &ckRIFF, 0) != S_OK)
    return AVIERR_FILEWRITE;

  /* add some JUNK at end for bad parsers */
  memset(&ckRIFF, 0, sizeof(ckRIFF));
  mmioWrite(This->hmmio, (HPSTR)&ckRIFF, sizeof(ckRIFF));
  mmioFlush(This->hmmio, 0);

  return AVIERR_OK;
}

static HRESULT AVIFILE_SaveIndex(const IAVIFileImpl *This)
{
  IAVIStreamImpl *pStream;
  AVIINDEXENTRY   idx;
  MMCKINFO        ck;
  DWORD           nStream;
  LONG            n;

  ck.ckid   = ckidAVINEWINDEX;
  ck.cksize = 0;
  if (mmioCreateChunk(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  if (This->fInfo.dwFlags & AVIFILEINFO_ISINTERLEAVED) {
    /* is interleaved -- write block of corresponding frames */
    LONG lInitialFrames = 0;
    LONG stepsize;
    LONG i;

    if (This->ppStreams[0]->sInfo.dwSampleSize == 0)
      stepsize = 1;
    else
      stepsize = AVIStreamTimeToSample(&This->ppStreams[0]->IAVIStream_iface, 1000000);

    assert(stepsize > 0);

    for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
      if (lInitialFrames < This->ppStreams[nStream]->sInfo.dwInitialFrames)
	lInitialFrames = This->ppStreams[nStream]->sInfo.dwInitialFrames;
    }

    for (i = -lInitialFrames; i < (LONG)This->fInfo.dwLength - lInitialFrames;
	 i += stepsize) {
      DWORD nFrame = lInitialFrames + i;

      assert(nFrame < This->nIdxRecords);

      idx.ckid          = listtypeAVIRECORD;
      idx.dwFlags       = AVIIF_LIST;
      idx.dwChunkLength = This->idxRecords[nFrame].dwChunkLength;
      idx.dwChunkOffset = This->idxRecords[nFrame].dwChunkOffset
	- This->dwMoviChunkPos;
      if (mmioWrite(This->hmmio, (HPSTR)&idx, sizeof(idx)) != sizeof(idx))
	return AVIERR_FILEWRITE;

      for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
	pStream = This->ppStreams[nStream];

	/* heave we reached start of this stream? */
	if (-(LONG)pStream->sInfo.dwInitialFrames > i)
	  continue;

	if (pStream->sInfo.dwInitialFrames < lInitialFrames)
	  nFrame -= (lInitialFrames - pStream->sInfo.dwInitialFrames);

	/* reached end of this stream? */
	if (pStream->lLastFrame <= nFrame)
	  continue;

	if ((pStream->sInfo.dwFlags & AVISTREAMINFO_FORMATCHANGES) &&
	    pStream->sInfo.dwFormatChangeCount != 0 &&
	    pStream->idxFmtChanges != NULL) {
	  DWORD pos;

	  for (pos = 0; pos < pStream->sInfo.dwFormatChangeCount; pos++) {
	    if (pStream->idxFmtChanges[pos].ckid == nFrame) {
	      idx.dwFlags = AVIIF_NOTIME;
	      idx.ckid    = MAKEAVICKID(cktypePALchange, pStream->nStream);
	      idx.dwChunkLength = pStream->idxFmtChanges[pos].dwChunkLength;
	      idx.dwChunkOffset = pStream->idxFmtChanges[pos].dwChunkOffset
		- This->dwMoviChunkPos;

	      if (mmioWrite(This->hmmio, (HPSTR)&idx, sizeof(idx)) != sizeof(idx))
		return AVIERR_FILEWRITE;
	      break;
	    }
	  }
	} /* if have formatchanges */

	idx.ckid          = pStream->idxFrames[nFrame].ckid;
	idx.dwFlags       = pStream->idxFrames[nFrame].dwFlags;
	idx.dwChunkLength = pStream->idxFrames[nFrame].dwChunkLength;
	idx.dwChunkOffset = pStream->idxFrames[nFrame].dwChunkOffset
	  - This->dwMoviChunkPos;
	if (mmioWrite(This->hmmio, (HPSTR)&idx, sizeof(idx)) != sizeof(idx))
	  return AVIERR_FILEWRITE;
      }
    }
  } else {
    /* not interleaved -- write index for each stream at once */
    for (nStream = 0; nStream < This->fInfo.dwStreams; nStream++) {
      pStream = This->ppStreams[nStream];

      for (n = 0; n <= pStream->lLastFrame; n++) {
	if ((pStream->sInfo.dwFlags & AVISTREAMINFO_FORMATCHANGES) &&
	    (pStream->sInfo.dwFormatChangeCount != 0)) {
	  DWORD pos;

	  for (pos = 0; pos < pStream->sInfo.dwFormatChangeCount; pos++) {
	    if (pStream->idxFmtChanges[pos].ckid == n) {
	      idx.dwFlags = AVIIF_NOTIME;
	      idx.ckid    = MAKEAVICKID(cktypePALchange, pStream->nStream);
	      idx.dwChunkLength = pStream->idxFmtChanges[pos].dwChunkLength;
	      idx.dwChunkOffset =
		pStream->idxFmtChanges[pos].dwChunkOffset - This->dwMoviChunkPos;
	      if (mmioWrite(This->hmmio, (HPSTR)&idx, sizeof(idx)) != sizeof(idx))
		return AVIERR_FILEWRITE;
	      break;
	    }
	  }
	} /* if have formatchanges */

	idx.ckid          = pStream->idxFrames[n].ckid;
	idx.dwFlags       = pStream->idxFrames[n].dwFlags;
	idx.dwChunkLength = pStream->idxFrames[n].dwChunkLength;
	idx.dwChunkOffset = pStream->idxFrames[n].dwChunkOffset
	  - This->dwMoviChunkPos;

	if (mmioWrite(This->hmmio, (HPSTR)&idx, sizeof(idx)) != sizeof(idx))
	  return AVIERR_FILEWRITE;
      }
    }
  } /* if not interleaved */

  if (mmioAscend(This->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  return AVIERR_OK;
}

static ULONG  AVIFILE_SearchStream(const IAVIFileImpl *This, DWORD fcc, LONG lSkip)
{
  UINT i;
  UINT nStream;

  /* pre-condition */
  assert(lSkip >= 0);

  if (fcc != 0) {
    /* search the number of the specified stream */
    nStream = (ULONG)-1;
    for (i = 0; i < This->fInfo.dwStreams; i++) {
      assert(This->ppStreams[i] != NULL);

      if (This->ppStreams[i]->sInfo.fccType == fcc) {
	if (lSkip == 0) {
	  nStream = i;
	  break;
	} else
	  lSkip--;
      }
    }
  } else
    nStream = lSkip;

  return nStream;
}

static void    AVIFILE_UpdateInfo(IAVIFileImpl *This)
{
  UINT i;

  /* pre-conditions */
  assert(This != NULL);

  This->fInfo.dwMaxBytesPerSec      = 0;
  This->fInfo.dwCaps                = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
  This->fInfo.dwSuggestedBufferSize = 0;
  This->fInfo.dwWidth               = 0;
  This->fInfo.dwHeight              = 0;
  This->fInfo.dwScale               = 0;
  This->fInfo.dwRate                = 0;
  This->fInfo.dwLength              = 0;
  This->dwInitialFrames             = 0;

  for (i = 0; i < This->fInfo.dwStreams; i++) {
    AVISTREAMINFOW *psi;
    DWORD           n;

    /* pre-conditions */
    assert(This->ppStreams[i] != NULL);

    psi = &This->ppStreams[i]->sInfo;
    assert(psi->dwScale != 0);
    assert(psi->dwRate != 0);

    if (i == 0) {
      /* use first stream timings as base */
      This->fInfo.dwScale  = psi->dwScale;
      This->fInfo.dwRate   = psi->dwRate;
      This->fInfo.dwLength = psi->dwLength;
    } else {
      n = AVIStreamSampleToSample(&This->ppStreams[0]->IAVIStream_iface,
                                  &This->ppStreams[i]->IAVIStream_iface, psi->dwLength);
      if (This->fInfo.dwLength < n)
	This->fInfo.dwLength = n;
    }

    if (This->dwInitialFrames < psi->dwInitialFrames)
      This->dwInitialFrames = psi->dwInitialFrames;

    if (This->fInfo.dwSuggestedBufferSize < psi->dwSuggestedBufferSize)
      This->fInfo.dwSuggestedBufferSize = psi->dwSuggestedBufferSize;

    if (psi->dwSampleSize != 0) {
      /* fixed sample size -- exact computation */
      This->fInfo.dwMaxBytesPerSec += MulDiv(psi->dwSampleSize, psi->dwRate,
					     psi->dwScale);
    } else {
      /* variable sample size -- only upper limit */
      This->fInfo.dwMaxBytesPerSec += MulDiv(psi->dwSuggestedBufferSize,
					     psi->dwRate, psi->dwScale);

      /* update dimensions */
      n = psi->rcFrame.right - psi->rcFrame.left;
      if (This->fInfo.dwWidth < n)
	This->fInfo.dwWidth = n;
      n = psi->rcFrame.bottom - psi->rcFrame.top;
      if (This->fInfo.dwHeight < n)
	This->fInfo.dwHeight = n;
    }
  }
}

static HRESULT AVIFILE_WriteBlock(IAVIStreamImpl *This, DWORD block,
				  FOURCC ckid, DWORD flags, LPCVOID buffer,
				  LONG size)
{
  MMCKINFO ck;

  ck.ckid    = ckid;
  ck.cksize  = size;
  ck.fccType = 0;

  /* if no frame/block is already written, we must compute start of movi chunk */
  if (This->paf->dwMoviChunkPos == 0)
    AVIFILE_ComputeMoviStart(This->paf);

  if (mmioSeek(This->paf->hmmio, This->paf->dwNextFramePos, SEEK_SET) == -1)
    return AVIERR_FILEWRITE;

  if (mmioCreateChunk(This->paf->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;
  if (buffer != NULL && size > 0) {
    if (mmioWrite(This->paf->hmmio, buffer, size) != size)
      return AVIERR_FILEWRITE;
  }
  if (mmioAscend(This->paf->hmmio, &ck, 0) != S_OK)
    return AVIERR_FILEWRITE;

  This->paf->fDirty         = TRUE;
  This->paf->dwNextFramePos = mmioSeek(This->paf->hmmio, 0, SEEK_CUR);

  return AVIFILE_AddFrame(This, ckid, size,
			  ck.dwDataOffset - 2 * sizeof(DWORD), flags);
}
