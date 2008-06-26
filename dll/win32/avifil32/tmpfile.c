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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "vfw.h"

#include "avifile_private.h"
#include "extrachunk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************/

static HRESULT WINAPI ITmpFile_fnQueryInterface(IAVIFile* iface,REFIID refiid,LPVOID *obj);
static ULONG   WINAPI ITmpFile_fnAddRef(IAVIFile* iface);
static ULONG   WINAPI ITmpFile_fnRelease(IAVIFile* iface);
static HRESULT WINAPI ITmpFile_fnInfo(IAVIFile*iface,AVIFILEINFOW*afi,LONG size);
static HRESULT WINAPI ITmpFile_fnGetStream(IAVIFile*iface,PAVISTREAM*avis,DWORD fccType,LONG lParam);
static HRESULT WINAPI ITmpFile_fnCreateStream(IAVIFile*iface,PAVISTREAM*avis,AVISTREAMINFOW*asi);
static HRESULT WINAPI ITmpFile_fnWriteData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG size);
static HRESULT WINAPI ITmpFile_fnReadData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG *size);
static HRESULT WINAPI ITmpFile_fnEndRecord(IAVIFile*iface);
static HRESULT WINAPI ITmpFile_fnDeleteStream(IAVIFile*iface,DWORD fccType,LONG lParam);

static const struct IAVIFileVtbl itmpft = {
  ITmpFile_fnQueryInterface,
  ITmpFile_fnAddRef,
  ITmpFile_fnRelease,
  ITmpFile_fnInfo,
  ITmpFile_fnGetStream,
  ITmpFile_fnCreateStream,
  ITmpFile_fnWriteData,
  ITmpFile_fnReadData,
  ITmpFile_fnEndRecord,
  ITmpFile_fnDeleteStream
};

typedef struct _ITmpFileImpl {
  /* IUnknown stuff */
  const IAVIFileVtbl *lpVtbl;
  LONG         ref;

  /* IAVIFile stuff */
  AVIFILEINFOW  fInfo;
  PAVISTREAM   *ppStreams;
} ITmpFileImpl;

PAVIFILE AVIFILE_CreateAVITempFile(int nStreams, const PAVISTREAM *ppStreams) {
  ITmpFileImpl *tmpFile;
  int           i;

  tmpFile = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ITmpFileImpl));
  if (tmpFile == NULL)
    return NULL;

  tmpFile->lpVtbl = &itmpft;
  tmpFile->ref    = 1;
  memset(&tmpFile->fInfo, 0, sizeof(tmpFile->fInfo));

  tmpFile->fInfo.dwStreams = nStreams;
  tmpFile->ppStreams = HeapAlloc(GetProcessHeap(), 0, nStreams * sizeof(PAVISTREAM));
  if (tmpFile->ppStreams == NULL) {
    HeapFree(GetProcessHeap(), 0, tmpFile);
    return NULL;
  }

  for (i = 0; i < nStreams; i++) {
    AVISTREAMINFOW sInfo;

    tmpFile->ppStreams[i] = ppStreams[i];

    AVIStreamAddRef(ppStreams[i]);
    AVIStreamInfoW(ppStreams[i], &sInfo, sizeof(sInfo));
    if (i == 0) {
      tmpFile->fInfo.dwScale = sInfo.dwScale;
      tmpFile->fInfo.dwRate  = sInfo.dwRate;
      if (!sInfo.dwScale || !sInfo.dwRate) {
	tmpFile->fInfo.dwScale = 1;
	tmpFile->fInfo.dwRate  = 100;
      }
    }

    if (tmpFile->fInfo.dwSuggestedBufferSize < sInfo.dwSuggestedBufferSize)
      tmpFile->fInfo.dwSuggestedBufferSize = sInfo.dwSuggestedBufferSize;

    {
      register DWORD tmp;

      tmp = MulDiv(AVIStreamSampleToTime(ppStreams[i], sInfo.dwLength),
		   tmpFile->fInfo.dwScale, tmpFile->fInfo.dwRate * 1000);
      if (tmpFile->fInfo.dwLength < tmp)
	tmpFile->fInfo.dwLength = tmp;

      tmp = sInfo.rcFrame.right - sInfo.rcFrame.left;
      if (tmpFile->fInfo.dwWidth < tmp)
	tmpFile->fInfo.dwWidth = tmp;
      tmp = sInfo.rcFrame.bottom - sInfo.rcFrame.top;
      if (tmpFile->fInfo.dwHeight < tmp)
	tmpFile->fInfo.dwHeight = tmp;
    }
  }

  return (PAVIFILE)tmpFile;
}

static HRESULT WINAPI ITmpFile_fnQueryInterface(IAVIFile *iface, REFIID refiid,
						LPVOID *obj)
{
  ITmpFileImpl *This = (ITmpFileImpl *)iface;

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(refiid), obj);

  if (IsEqualGUID(&IID_IUnknown, refiid) ||
      IsEqualGUID(&IID_IAVIFile, refiid)) {
    *obj = iface;
    IAVIFile_AddRef(iface);

    return S_OK;
  }

  return OLE_E_ENUM_NOMORE;
}

static ULONG   WINAPI ITmpFile_fnAddRef(IAVIFile *iface)
{
  ITmpFileImpl *This = (ITmpFileImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) -> %d\n", iface, ref);

  return ref;
}

static ULONG   WINAPI ITmpFile_fnRelease(IAVIFile *iface)
{
  ITmpFileImpl *This = (ITmpFileImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) -> %d\n", iface, ref);

  if (!ref) {
    unsigned int i;

    for (i = 0; i < This->fInfo.dwStreams; i++) {
      if (This->ppStreams[i] != NULL) {
	AVIStreamRelease(This->ppStreams[i]);

	This->ppStreams[i] = NULL;
      }
    }

    HeapFree(GetProcessHeap(), 0, This);
    return 0;
  }

  return ref;
}

static HRESULT WINAPI ITmpFile_fnInfo(IAVIFile *iface,
				      AVIFILEINFOW *afi, LONG size)
{
  ITmpFileImpl *This = (ITmpFileImpl *)iface;

  TRACE("(%p,%p,%d)\n",iface,afi,size);

  if (afi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  memcpy(afi, &This->fInfo, min((DWORD)size, sizeof(This->fInfo)));

  if ((DWORD)size < sizeof(This->fInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static HRESULT WINAPI ITmpFile_fnGetStream(IAVIFile *iface, PAVISTREAM *avis,
					   DWORD fccType, LONG lParam)
{
  ITmpFileImpl *This = (ITmpFileImpl *)iface;

  ULONG nStream = (ULONG)-1;

  TRACE("(%p,%p,0x%08X,%d)\n", iface, avis, fccType, lParam);

  if (avis == NULL || lParam < 0)
    return AVIERR_BADPARAM;

  if (fccType != streamtypeANY) {
    /* search the number of the specified stream */
    ULONG i;

    for (i = 0; i < This->fInfo.dwStreams; i++) {
      AVISTREAMINFOW sInfo;
      HRESULT        hr;

      hr = AVIStreamInfoW(This->ppStreams[i], &sInfo, sizeof(sInfo));
      if (FAILED(hr))
	return hr;

      if (sInfo.fccType == fccType) {
	if (lParam == 0) {
	  nStream = i;
	  break;
	} else
	  lParam--;
      }
    }
  } else
    nStream = lParam;

  /* Does the requested stream exist ? */
  if (nStream < This->fInfo.dwStreams && This->ppStreams[nStream] != NULL) {
    *avis = This->ppStreams[nStream];
    AVIStreamAddRef(*avis);

    return AVIERR_OK;
  }

  /* Sorry, but the specified stream doesn't exist */
  return AVIERR_NODATA;
}

static HRESULT WINAPI ITmpFile_fnCreateStream(IAVIFile *iface,PAVISTREAM *avis,
					      AVISTREAMINFOW *asi)
{
  TRACE("(%p,%p,%p)\n",iface,avis,asi);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI ITmpFile_fnWriteData(IAVIFile *iface, DWORD ckid,
					   LPVOID lpData, LONG size)
{
  TRACE("(%p,0x%08X,%p,%d)\n", iface, ckid, lpData, size);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI ITmpFile_fnReadData(IAVIFile *iface, DWORD ckid,
					  LPVOID lpData, LONG *size)
{
  TRACE("(%p,0x%08X,%p,%p)\n", iface, ckid, lpData, size);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI ITmpFile_fnEndRecord(IAVIFile *iface)
{
  TRACE("(%p)\n",iface);

  return AVIERR_OK;
}

static HRESULT WINAPI ITmpFile_fnDeleteStream(IAVIFile *iface, DWORD fccType,
					      LONG lParam)
{
  TRACE("(%p,0x%08X,%d)\n", iface, fccType, lParam);

  return AVIERR_UNSUPPORTED;
}
