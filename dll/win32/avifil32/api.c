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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "shellapi.h"
#include "shlobj.h"
#include "vfw.h"
#include "msacm.h"

#include "avifile_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);


/***********************************************************************
 * for AVIBuildFilterW -- uses fixed size table
 */
#define MAX_FILTERS 30 /* 30 => 7kB */

typedef struct _AVIFilter {
  WCHAR szClsid[40];
  WCHAR szExtensions[MAX_FILTERS * 7];
} AVIFilter;

/***********************************************************************
 * for AVISaveOptions
 */
static struct {
  UINT                  uFlags;
  INT                   nStreams;
  PAVISTREAM           *ppavis;
  LPAVICOMPRESSOPTIONS *ppOptions;
  INT                   nCurrent;
} SaveOpts;

/***********************************************************************
 * copied from dlls/ole32/compobj.c
 */
static HRESULT AVIFILE_CLSIDFromString(LPCSTR idstr, LPCLSID id)
{
  BYTE const *s;
  BYTE *p;
  INT   i;
  BYTE table[256];

  if (!idstr) {
    memset(id, 0, sizeof(CLSID));
    return S_OK;
  }

  /* validate the CLSID string */
  if (lstrlenA(idstr) != 38)
    return CO_E_CLASSSTRING;

  s = (BYTE const*)idstr;
  if ((s[0]!='{') || (s[9]!='-') || (s[14]!='-') || (s[19]!='-') ||
      (s[24]!='-') || (s[37]!='}'))
    return CO_E_CLASSSTRING;

  for (i = 1; i < 37; i++) {
    if ((i == 9) || (i == 14) || (i == 19) || (i == 24))
      continue;
    if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
        ((s[i] >= 'a') && (s[i] <= 'f'))  ||
        ((s[i] >= 'A') && (s[i] <= 'F')))
       )
      return CO_E_CLASSSTRING;
  }

  TRACE("%s -> %p\n", s, id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++)
    table['0' + i] = i;

  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */
  p = (BYTE *) id;

  s++;	/* skip leading brace  */
  for (i = 0; i < 4; i++) {
    p[3 - i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 4;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  /* these are just sequential bytes */
  for (i = 0; i < 2; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  s++;	/* skip - */

  for (i = 0; i < 6; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }

  return S_OK;
}

static BOOL AVIFILE_GetFileHandlerByExtension(LPCWSTR szFile, LPCLSID lpclsid)
{
  CHAR   szRegKey[25];
  CHAR   szValue[100];
  LPWSTR szExt = wcsrchr(szFile, '.');
  LONG   len = ARRAY_SIZE(szValue);

  if (szExt == NULL)
    return FALSE;

  szExt++;

  wsprintfA(szRegKey, "AVIFile\\Extensions\\%.3ls", szExt);
  if (RegQueryValueA(HKEY_CLASSES_ROOT, szRegKey, szValue, &len) != ERROR_SUCCESS)
    return FALSE;

  return (AVIFILE_CLSIDFromString(szValue, lpclsid) == S_OK);
}

/***********************************************************************
 *		AVIFileInit		(AVIFIL32.@)
 */
void WINAPI AVIFileInit(void) {
  OleInitialize(NULL);
}

/***********************************************************************
 *		AVIFileExit		(AVIFIL32.@)
 */
void WINAPI AVIFileExit(void) {
  /* need to free ole32.dll if we are the last exit call */
  /* OleUninitialize() */
  FIXME("(): stub!\n");
}

/***********************************************************************
 *		AVIFileOpen		(AVIFIL32.@)
 *		AVIFileOpenA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileOpenA(PAVIFILE *ppfile, LPCSTR szFile, UINT uMode,
			    LPCLSID lpHandler)
{
  LPWSTR  wszFile = NULL;
  HRESULT hr;
  int     len;

  TRACE("(%p,%s,0x%08X,%s)\n", ppfile, debugstr_a(szFile), uMode,
	debugstr_guid(lpHandler));

  /* check parameters */
  if (ppfile == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  /* convert the ANSI string to Unicode and call the Unicode function */
  len = MultiByteToWideChar(CP_ACP, 0, szFile, -1, NULL, 0);
  if (len <= 0)
    return AVIERR_BADPARAM;

  wszFile = malloc(len * sizeof(WCHAR));
  if (wszFile == NULL)
    return AVIERR_MEMORY;

  MultiByteToWideChar(CP_ACP, 0, szFile, -1, wszFile, len);

  hr = AVIFileOpenW(ppfile, wszFile, uMode, lpHandler);

  free(wszFile);

  return hr;
}

/***********************************************************************
 *		AVIFileOpenW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileOpenW(PAVIFILE *ppfile, LPCWSTR szFile, UINT uMode,
			    LPCLSID lpHandler)
{
  IPersistFile *ppersist = NULL;
  CLSID         clsidHandler;
  HRESULT       hr;

  TRACE("(%p,%s,0x%X,%s)\n", ppfile, debugstr_w(szFile), uMode,
	debugstr_guid(lpHandler));

  /* check parameters */
  if (ppfile == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppfile = NULL;

  /* if no handler then try guessing it by extension */
  if (lpHandler == NULL) {
    if (! AVIFILE_GetFileHandlerByExtension(szFile, &clsidHandler))
      clsidHandler = CLSID_AVIFile;
  } else
    clsidHandler = *lpHandler;

  /* create instance of handler */
  hr = CoCreateInstance(&clsidHandler, NULL, CLSCTX_INPROC, &IID_IAVIFile, (LPVOID*)ppfile);
  if (FAILED(hr) || *ppfile == NULL)
    return hr;

  /* ask for IPersistFile interface for loading/creating the file */
  hr = IAVIFile_QueryInterface(*ppfile, &IID_IPersistFile, (LPVOID*)&ppersist);
  if (FAILED(hr) || ppersist == NULL) {
    IAVIFile_Release(*ppfile);
    *ppfile = NULL;
    return hr;
  }

  hr = IPersistFile_Load(ppersist, szFile, uMode);
  IPersistFile_Release(ppersist);
  if (FAILED(hr)) {
    IAVIFile_Release(*ppfile);
    *ppfile = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIFileAddRef		(AVIFIL32.@)
 */
ULONG WINAPI AVIFileAddRef(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIFile_AddRef(pfile);
}

/***********************************************************************
 *		AVIFileRelease		(AVIFIL32.@)
 */
ULONG WINAPI AVIFileRelease(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIFile_Release(pfile);
}

/***********************************************************************
 *		AVIFileInfo		(AVIFIL32.@)
 *		AVIFileInfoA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoA(PAVIFILE pfile, LPAVIFILEINFOA afi, LONG size)
{
  AVIFILEINFOW afiw;
  HRESULT      hres;

  TRACE("(%p,%p,%ld)\n", pfile, afi, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;
  if ((DWORD)size < sizeof(AVIFILEINFOA))
    return AVIERR_BADSIZE;

  hres = IAVIFile_Info(pfile, &afiw, sizeof(afiw));

  memcpy(afi, &afiw, sizeof(*afi) - sizeof(afi->szFileType));
  WideCharToMultiByte(CP_ACP, 0, afiw.szFileType, -1, afi->szFileType,
		      sizeof(afi->szFileType), NULL, NULL);
  afi->szFileType[sizeof(afi->szFileType) - 1] = 0;

  return hres;
}

/***********************************************************************
 *		AVIFileInfoW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoW(PAVIFILE pfile, LPAVIFILEINFOW afiw, LONG size)
{
  TRACE("(%p,%p,%ld)\n", pfile, afiw, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_Info(pfile, afiw, size);
}

/***********************************************************************
 *		AVIFileGetStream	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileGetStream(PAVIFILE pfile, PAVISTREAM *avis,
				DWORD fccType, LONG lParam)
{
  TRACE("(%p,%p,'%4.4s',%ld)\n", pfile, avis, (char*)&fccType, lParam);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_GetStream(pfile, avis, fccType, lParam);
}

/***********************************************************************
 *		AVIFileCreateStream	(AVIFIL32.@)
 *		AVIFileCreateStreamA	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamA(PAVIFILE pfile, PAVISTREAM *ppavi,
				    LPAVISTREAMINFOA psi)
{
  AVISTREAMINFOW	psiw;

  TRACE("(%p,%p,%p)\n", pfile, ppavi, psi);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  /* Only the szName at the end is different */
  memcpy(&psiw, psi, sizeof(*psi) - sizeof(psi->szName));
  MultiByteToWideChar(CP_ACP, 0, psi->szName, -1, psiw.szName,
		      ARRAY_SIZE(psiw.szName));

  return IAVIFile_CreateStream(pfile, ppavi, &psiw);
}

/***********************************************************************
 *		AVIFileCreateStreamW	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamW(PAVIFILE pfile, PAVISTREAM *avis,
				    LPAVISTREAMINFOW asi)
{
  TRACE("(%p,%p,%p)\n", pfile, avis, asi);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_CreateStream(pfile, avis, asi);
}

/***********************************************************************
 *		AVIFileWriteData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileWriteData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LONG size)
{
  TRACE("(%p,'%4.4s',%p,%ld)\n", pfile, (char*)&fcc, lp, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_WriteData(pfile, fcc, lp, size);
}

/***********************************************************************
 *		AVIFileReadData		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileReadData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LPLONG size)
{
  TRACE("(%p,'%4.4s',%p,%p)\n", pfile, (char*)&fcc, lp, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_ReadData(pfile, fcc, lp, size);
}

/***********************************************************************
 *		AVIFileEndRecord	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileEndRecord(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_EndRecord(pfile);
}

/***********************************************************************
 *		AVIStreamAddRef		(AVIFIL32.@)
 */
ULONG WINAPI AVIStreamAddRef(PAVISTREAM pstream)
{
  TRACE("(%p)\n", pstream);

  if (pstream == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIStream_AddRef(pstream);
}

/***********************************************************************
 *		AVIStreamRelease	(AVIFIL32.@)
 */
ULONG WINAPI AVIStreamRelease(PAVISTREAM pstream)
{
  TRACE("(%p)\n", pstream);

  if (pstream == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIStream_Release(pstream);
}

/***********************************************************************
 *		AVIStreamCreate		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamCreate(PAVISTREAM *ppavi, LONG lParam1, LONG lParam2,
			       LPCLSID pclsidHandler)
{
  HRESULT hr;

  TRACE("(%p,0x%08lX,0x%08lX,%s)\n", ppavi, lParam1, lParam2,
	debugstr_guid(pclsidHandler));

  if (ppavi == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;
  if (pclsidHandler == NULL)
    return AVIERR_UNSUPPORTED;

  hr = CoCreateInstance(pclsidHandler, NULL, CLSCTX_INPROC, &IID_IAVIStream, (LPVOID*)ppavi);
  if (FAILED(hr) || *ppavi == NULL)
    return hr;

  hr = IAVIStream_Create(*ppavi, lParam1, lParam2);
  if (FAILED(hr)) {
    IAVIStream_Release(*ppavi);
    *ppavi = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIStreamInfo		(AVIFIL32.@)
 *		AVIStreamInfoA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoA(PAVISTREAM pstream, LPAVISTREAMINFOA asi,
			      LONG size)
{
  AVISTREAMINFOW asiw;
  HRESULT	 hres;

  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;
  if ((DWORD)size < sizeof(AVISTREAMINFOA))
    return AVIERR_BADSIZE;

  hres = IAVIStream_Info(pstream, &asiw, sizeof(asiw));

  memcpy(asi, &asiw, sizeof(asiw) - sizeof(asiw.szName));
  WideCharToMultiByte(CP_ACP, 0, asiw.szName, -1, asi->szName,
		      sizeof(asi->szName), NULL, NULL);
  asi->szName[sizeof(asi->szName) - 1] = 0;

  return hres;
}

/***********************************************************************
 *		AVIStreamInfoW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoW(PAVISTREAM pstream, LPAVISTREAMINFOW asi,
			      LONG size)
{
  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Info(pstream, asi, size);
}

/***********************************************************************
 *		AVIStreamFindSample	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamFindSample(PAVISTREAM pstream, LONG pos, LONG flags)
{
  TRACE("(%p,%ld,0x%lX)\n", pstream, pos, flags);

  if (pstream == NULL)
    return -1;

  return IAVIStream_FindSample(pstream, pos, flags);
}

/***********************************************************************
 *		AVIStreamReadFormat	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadFormat(PAVISTREAM pstream, LONG pos,
				   LPVOID format, LPLONG formatsize)
{
  TRACE("(%p,%ld,%p,%p)\n", pstream, pos, format, formatsize);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_ReadFormat(pstream, pos, format, formatsize);
}

/***********************************************************************
 *		AVIStreamSetFormat	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamSetFormat(PAVISTREAM pstream, LONG pos,
				  LPVOID format, LONG formatsize)
{
  TRACE("(%p,%ld,%p,%ld)\n", pstream, pos, format, formatsize);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_SetFormat(pstream, pos, format, formatsize);
}

/***********************************************************************
 *		AVIStreamRead		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamRead(PAVISTREAM pstream, LONG start, LONG samples,
			     LPVOID buffer, LONG buffersize,
			     LPLONG bytesread, LPLONG samplesread)
{
  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", pstream, start, samples, buffer,
	buffersize, bytesread, samplesread);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Read(pstream, start, samples, buffer, buffersize,
			 bytesread, samplesread);
}

/***********************************************************************
 *		AVIStreamWrite		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWrite(PAVISTREAM pstream, LONG start, LONG samples,
			      LPVOID buffer, LONG buffersize, DWORD flags,
			      LPLONG sampwritten, LPLONG byteswritten)
{
  TRACE("(%p,%ld,%ld,%p,%ld,0x%lX,%p,%p)\n", pstream, start, samples, buffer,
	buffersize, flags, sampwritten, byteswritten);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Write(pstream, start, samples, buffer, buffersize,
			  flags, sampwritten, byteswritten);
}

/***********************************************************************
 *		AVIStreamReadData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadData(PAVISTREAM pstream, DWORD fcc, LPVOID lp,
				 LPLONG lpread)
{
  TRACE("(%p,'%4.4s',%p,%p)\n", pstream, (char*)&fcc, lp, lpread);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_ReadData(pstream, fcc, lp, lpread);
}

/***********************************************************************
 *		AVIStreamWriteData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWriteData(PAVISTREAM pstream, DWORD fcc, LPVOID lp,
				  LONG size)
{
  TRACE("(%p,'%4.4s',%p,%ld)\n", pstream, (char*)&fcc, lp, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_WriteData(pstream, fcc, lp, size);
}

/***********************************************************************
 *		AVIStreamGetFrameOpen	(AVIFIL32.@)
 */
PGETFRAME WINAPI AVIStreamGetFrameOpen(PAVISTREAM pstream,
				       LPBITMAPINFOHEADER lpbiWanted)
{
  PGETFRAME pg = NULL;

  TRACE("(%p,%p)\n", pstream, lpbiWanted);

  if (FAILED(IAVIStream_QueryInterface(pstream, &IID_IGetFrame, (LPVOID*)&pg)) ||
      pg == NULL) {
    pg = AVIFILE_CreateGetFrame(pstream);
    if (pg == NULL)
      return NULL;
  }

  if (FAILED(IGetFrame_SetFormat(pg, lpbiWanted, NULL, 0, 0, -1, -1))) {
    IGetFrame_Release(pg);
    return NULL;
  }

  return pg;
}

/***********************************************************************
 *		AVIStreamGetFrame	(AVIFIL32.@)
 */
LPVOID WINAPI AVIStreamGetFrame(PGETFRAME pg, LONG pos)
{
  TRACE("(%p,%ld)\n", pg, pos);

  if (pg == NULL)
    return NULL;

  return IGetFrame_GetFrame(pg, pos);
}

/***********************************************************************
 *		AVIStreamGetFrameClose	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamGetFrameClose(PGETFRAME pg)
{
  TRACE("(%p)\n", pg);

  if (pg != NULL)
    return IGetFrame_Release(pg);
  return 0;
}

/***********************************************************************
 *		AVIMakeCompressedStream	(AVIFIL32.@)
 */
HRESULT WINAPI AVIMakeCompressedStream(PAVISTREAM *ppsCompressed,
				       PAVISTREAM psSource,
				       LPAVICOMPRESSOPTIONS aco,
				       LPCLSID pclsidHandler)
{
  AVISTREAMINFOW asiw;
  CHAR           szRegKey[25];
  CHAR           szValue[100];
  CLSID          clsidHandler;
  HRESULT        hr;
  LONG           size = sizeof(szValue);

  TRACE("(%p,%p,%p,%s)\n", ppsCompressed, psSource, aco,
	debugstr_guid(pclsidHandler));

  if (ppsCompressed == NULL)
    return AVIERR_BADPARAM;
  if (psSource == NULL)
    return AVIERR_BADHANDLE;

  *ppsCompressed = NULL;

  /* if no handler given get default ones based on streamtype */
  if (pclsidHandler == NULL) {
    hr = IAVIStream_Info(psSource, &asiw, sizeof(asiw));
    if (FAILED(hr))
      return hr;

    wsprintfA(szRegKey, "AVIFile\\Compressors\\%4.4s", (char*)&asiw.fccType);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, szRegKey, szValue, &size) != ERROR_SUCCESS)
      return AVIERR_UNSUPPORTED;
    if (AVIFILE_CLSIDFromString(szValue, &clsidHandler) != S_OK)
      return AVIERR_UNSUPPORTED;
  } else
    clsidHandler = *pclsidHandler;

  hr = CoCreateInstance(&clsidHandler, NULL, CLSCTX_INPROC, &IID_IAVIStream, (LPVOID*)ppsCompressed);
  if (FAILED(hr) || *ppsCompressed == NULL)
    return hr;

  hr = IAVIStream_Create(*ppsCompressed, (LPARAM)psSource, (LPARAM)aco);
  if (FAILED(hr)) {
    IAVIStream_Release(*ppsCompressed);
    *ppsCompressed = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIMakeFileFromStreams	(AVIFIL32.@)
 */
HRESULT WINAPI AVIMakeFileFromStreams(PAVIFILE *ppfile, int nStreams,
				      PAVISTREAM *ppStreams)
{
  TRACE("(%p,%d,%p)\n", ppfile, nStreams, ppStreams);

  if (nStreams < 0 || ppfile == NULL || ppStreams == NULL)
    return AVIERR_BADPARAM;

  *ppfile = AVIFILE_CreateAVITempFile(nStreams, ppStreams);
  if (*ppfile == NULL)
    return AVIERR_MEMORY;

  return AVIERR_OK;
}

/***********************************************************************
 *		AVIStreamOpenFromFile	(AVIFIL32.@)
 *		AVIStreamOpenFromFileA	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileA(PAVISTREAM *ppavi, LPCSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, LPCLSID pclsidHandler)
{
  PAVIFILE pfile = NULL;
  HRESULT  hr;

  TRACE("(%p,%s,'%4.4s',%ld,0x%X,%s)\n", ppavi, debugstr_a(szFile),
	(char*)&fccType, lParam, mode, debugstr_guid(pclsidHandler));

  if (ppavi == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;

  hr = AVIFileOpenA(&pfile, szFile, mode, pclsidHandler);
  if (FAILED(hr) || pfile == NULL)
    return hr;

  hr = IAVIFile_GetStream(pfile, ppavi, fccType, lParam);
  IAVIFile_Release(pfile);

  return hr;
}

/***********************************************************************
 *		AVIStreamOpenFromFileW	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileW(PAVISTREAM *ppavi, LPCWSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, LPCLSID pclsidHandler)
{
  PAVIFILE pfile = NULL;
  HRESULT  hr;

  TRACE("(%p,%s,'%4.4s',%ld,0x%X,%s)\n", ppavi, debugstr_w(szFile),
	(char*)&fccType, lParam, mode, debugstr_guid(pclsidHandler));

  if (ppavi == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;

  hr = AVIFileOpenW(&pfile, szFile, mode, pclsidHandler);
  if (FAILED(hr) || pfile == NULL)
    return hr;

  hr = IAVIFile_GetStream(pfile, ppavi, fccType, lParam);
  IAVIFile_Release(pfile);

  return hr;
}

/***********************************************************************
 *		AVIStreamBeginStreaming	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamBeginStreaming(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate)
{
  IAVIStreaming* pstream = NULL;
  HRESULT hr;

  TRACE("(%p,%ld,%ld,%ld)\n", pavi, lStart, lEnd, lRate);

  if (pavi == NULL)
    return AVIERR_BADHANDLE;

  hr = IAVIStream_QueryInterface(pavi, &IID_IAVIStreaming, (LPVOID*)&pstream);
  if (SUCCEEDED(hr) && pstream != NULL) {
    hr = IAVIStreaming_Begin(pstream, lStart, lEnd, lRate);
    IAVIStreaming_Release(pstream);
  } else
    hr = AVIERR_OK;

  return hr;
}

/***********************************************************************
 *		AVIStreamEndStreaming	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamEndStreaming(PAVISTREAM pavi)
{
  IAVIStreaming* pstream = NULL;
  HRESULT hr;

  TRACE("(%p)\n", pavi);

  hr = IAVIStream_QueryInterface(pavi, &IID_IAVIStreaming, (LPVOID*)&pstream);
  if (SUCCEEDED(hr) && pstream != NULL) {
    IAVIStreaming_End(pstream);
    IAVIStreaming_Release(pstream);
  }

 return AVIERR_OK;
}

/***********************************************************************
 *		AVIStreamStart		(AVIFIL32.@)
 */
LONG WINAPI AVIStreamStart(PAVISTREAM pstream)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p)\n", pstream);

  if (pstream == NULL)
    return 0;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return 0;

  return asiw.dwStart;
}

/***********************************************************************
 *		AVIStreamLength		(AVIFIL32.@)
 */
LONG WINAPI AVIStreamLength(PAVISTREAM pstream)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p)\n", pstream);

  if (pstream == NULL)
    return 0;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return 0;

  return asiw.dwLength;
}

/***********************************************************************
 *		AVIStreamSampleToTime	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamSampleToTime(PAVISTREAM pstream, LONG lSample)
{
  AVISTREAMINFOW asiw;
  LONG time;

  TRACE("(%p,%ld)\n", pstream, lSample);

  if (pstream == NULL)
    return -1;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return -1;
  if (asiw.dwRate == 0)
    return -1;

  /* limit to stream bounds */
  if (lSample < asiw.dwStart)
    lSample = asiw.dwStart;
  if (lSample > asiw.dwStart + asiw.dwLength)
    lSample = asiw.dwStart + asiw.dwLength;

  if (asiw.dwRate / asiw.dwScale < 1000)
    time = (LONG)(((float)lSample * asiw.dwScale * 1000) / asiw.dwRate);
  else
    time = (LONG)(((float)lSample * asiw.dwScale * 1000 + (asiw.dwRate - 1)) / asiw.dwRate);

  TRACE(" -> %ld\n",time);
  return time;
}

/***********************************************************************
 *		AVIStreamTimeToSample	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamTimeToSample(PAVISTREAM pstream, LONG lTime)
{
  AVISTREAMINFOW asiw;
  ULONG sample;

  TRACE("(%p,%ld)\n", pstream, lTime);

  if (pstream == NULL || lTime < 0)
    return -1;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return -1;
  if (asiw.dwScale == 0)
    return -1;

  if (asiw.dwRate / asiw.dwScale < 1000)
    sample = (LONG)((((float)asiw.dwRate * lTime) / (asiw.dwScale * 1000)));
  else
    sample = (LONG)(((float)asiw.dwRate * lTime + (asiw.dwScale * 1000 - 1)) / (asiw.dwScale * 1000));

  /* limit to stream bounds */
  if (sample < asiw.dwStart)
    sample = asiw.dwStart;
  if (sample > asiw.dwStart + asiw.dwLength)
    sample = asiw.dwStart + asiw.dwLength;

  TRACE(" -> %ld\n", sample);
  return sample;
}

/***********************************************************************
 *		AVIBuildFilter		(AVIFIL32.@)
 *		AVIBuildFilterA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIBuildFilterA(LPSTR szFilter, LONG cbFilter, BOOL fSaving)
{
  LPWSTR  wszFilter;
  HRESULT hr;

  TRACE("(%p,%ld,%d)\n", szFilter, cbFilter, fSaving);

  /* check parameters */
  if (szFilter == NULL)
    return AVIERR_BADPARAM;
  if (cbFilter < 2)
    return AVIERR_BADSIZE;

  szFilter[0] = 0;
  szFilter[1] = 0;

  wszFilter = malloc(cbFilter * sizeof(WCHAR));
  if (wszFilter == NULL)
    return AVIERR_MEMORY;

  hr = AVIBuildFilterW(wszFilter, cbFilter, fSaving);
  if (SUCCEEDED(hr)) {
    WideCharToMultiByte(CP_ACP, 0, wszFilter, cbFilter,
			szFilter, cbFilter, NULL, NULL);
  }

  free(wszFilter);

  return hr;
}

/***********************************************************************
 *		AVIBuildFilterW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIBuildFilterW(LPWSTR szFilter, LONG cbFilter, BOOL fSaving)
{
  static const WCHAR all_files[] = L"*.*\0";

  AVIFilter *lp;
  WCHAR      szAllFiles[40];
  WCHAR      szFileExt[10];
  WCHAR      szValue[128];
  HKEY       hKey;
  DWORD      n, i;
  LONG       size;
  DWORD      count = 0;

  TRACE("(%p,%ld,%d)\n", szFilter, cbFilter, fSaving);

  /* check parameters */
  if (szFilter == NULL)
    return AVIERR_BADPARAM;
  if (cbFilter < 2)
    return AVIERR_BADSIZE;

  lp = calloc(MAX_FILTERS, sizeof(AVIFilter));
  if (lp == NULL)
    return AVIERR_MEMORY;

  /*
   * 1. iterate over HKEY_CLASSES_ROOT\\AVIFile\\Extensions and collect
   *    extensions and CLSIDs
   * 2. iterate over collected CLSIDs and copy its description and its
   *    extensions to szFilter if it fits
   *
   * First filter is named "All multimedia files" and its filter is a
   * collection of all possible extensions except "*.*".
   */
  if (RegOpenKeyW(HKEY_CLASSES_ROOT, L"AVIFile\\Extensions", &hKey) != ERROR_SUCCESS) {
    free(lp);
    return AVIERR_ERROR;
  }
  for (n = 0;RegEnumKeyW(hKey, n, szFileExt, ARRAY_SIZE(szFileExt)) == ERROR_SUCCESS;n++) {
    WCHAR clsidW[40];

    /* get CLSID to extension */
    size = sizeof(clsidW);
    if (RegQueryValueW(hKey, szFileExt, clsidW, &size) != ERROR_SUCCESS)
      break;

    /* search if the CLSID is already known */
    for (i = 1; i <= count; i++) {
      if (lstrcmpW(lp[i].szClsid, clsidW) == 0)
	break; /* a new one */
    }

    if (i == count + 1) {
      /* it's a new CLSID */

      /* FIXME: How do we get info's about read/write capabilities? */

      if (count >= MAX_FILTERS) {
	/* try to inform user of our full fixed size table */
	ERR(": More than %d filters found! Adjust MAX_FILTERS in dlls/avifil32/api.c\n", MAX_FILTERS);
	break;
      }

      lstrcpyW(lp[i].szClsid, clsidW);

      count++;
    }

    /* append extension to the filter */
    wsprintfW(szValue, L";*.%s", szFileExt);
    if (lp[i].szExtensions[0] == 0)
      lstrcatW(lp[i].szExtensions, szValue + 1);
    else
      lstrcatW(lp[i].szExtensions, szValue);

    /* also append to the "all multimedia"-filter */
    if (lp[0].szExtensions[0] == 0)
      lstrcatW(lp[0].szExtensions, szValue + 1);
    else
      lstrcatW(lp[0].szExtensions, szValue);
  }
  RegCloseKey(hKey);

  /* 2. get descriptions for the CLSIDs and fill out szFilter */
  if (RegOpenKeyW(HKEY_CLASSES_ROOT, L"CLSID", &hKey) != ERROR_SUCCESS) {
    free(lp);
    return AVIERR_ERROR;
  }
  for (n = 0; n <= count; n++) {
    /* first the description */
    if (n != 0) {
      size = sizeof(szValue);
      if (RegQueryValueW(hKey, lp[n].szClsid, szValue, &size) == ERROR_SUCCESS) {
	size = lstrlenW(szValue);
	lstrcpynW(szFilter, szValue, cbFilter);
      }
    } else
      size = LoadStringW(AVIFILE_hModule,IDS_ALLMULTIMEDIA,szFilter,cbFilter);

    /* check for enough space */
    size++;
    if (cbFilter < size + lstrlenW(lp[n].szExtensions) + 2) {
      szFilter[0] = 0;
      szFilter[1] = 0;
      free(lp);
      RegCloseKey(hKey);
      return AVIERR_BUFFERTOOSMALL;
    }
    cbFilter -= size;
    szFilter += size;

    /* and then the filter */
    lstrcpynW(szFilter, lp[n].szExtensions, cbFilter);
    size = lstrlenW(lp[n].szExtensions) + 1;
    cbFilter -= size;
    szFilter += size;
  }

  RegCloseKey(hKey);
  free(lp);

  /* add "All files" "*.*" filter if enough space left */
  size = LoadStringW(AVIFILE_hModule, IDS_ALLFILES, szAllFiles,
                     ARRAY_SIZE(szAllFiles) - ARRAY_SIZE(all_files)) + 1;
  memcpy( szAllFiles + size, all_files, sizeof(all_files) );
  size += ARRAY_SIZE(all_files);

  if (cbFilter > size) {
    memcpy(szFilter, szAllFiles, size * sizeof(szAllFiles[0]));
    return AVIERR_OK;
  } else {
    szFilter[0] = 0;
    return AVIERR_BUFFERTOOSMALL;
  }
}

static BOOL AVISaveOptionsFmtChoose(HWND hWnd)
{
  LPAVICOMPRESSOPTIONS pOptions = SaveOpts.ppOptions[SaveOpts.nCurrent];
  AVISTREAMINFOW       sInfo;

  TRACE("(%p)\n", hWnd);

  if (pOptions == NULL || SaveOpts.ppavis[SaveOpts.nCurrent] == NULL) {
    ERR(": bad state!\n");
    return FALSE;
  }

  if (FAILED(AVIStreamInfoW(SaveOpts.ppavis[SaveOpts.nCurrent],
			    &sInfo, sizeof(sInfo)))) {
    ERR(": AVIStreamInfoW failed!\n");
    return FALSE;
  }

  if (sInfo.fccType == streamtypeVIDEO) {
    COMPVARS cv;
    BOOL     ret;

    memset(&cv, 0, sizeof(cv));

    if ((pOptions->dwFlags & AVICOMPRESSF_VALID) == 0) {
      memset(pOptions, 0, sizeof(AVICOMPRESSOPTIONS));
      pOptions->fccType    = streamtypeVIDEO;
      pOptions->fccHandler = comptypeDIB;
      pOptions->dwQuality  = (DWORD)ICQUALITY_DEFAULT;
    }

    cv.cbSize     = sizeof(cv);
    cv.dwFlags    = ICMF_COMPVARS_VALID;
    /*cv.fccType    = pOptions->fccType; */
    cv.fccHandler = pOptions->fccHandler;
    cv.lQ         = pOptions->dwQuality;
    cv.lpState    = pOptions->lpParms;
    cv.cbState    = pOptions->cbParms;
    if (pOptions->dwFlags & AVICOMPRESSF_KEYFRAMES)
      cv.lKey = pOptions->dwKeyFrameEvery;
    else
      cv.lKey = 0;
    if (pOptions->dwFlags & AVICOMPRESSF_DATARATE)
      cv.lDataRate = pOptions->dwBytesPerSecond / 1024; /* need kBytes */
    else
      cv.lDataRate = 0;

    ret = ICCompressorChoose(hWnd, SaveOpts.uFlags, NULL,
			     SaveOpts.ppavis[SaveOpts.nCurrent], &cv, NULL);

    if (ret) {
      pOptions->fccHandler = cv.fccHandler;
      pOptions->lpParms   = cv.lpState;
      pOptions->cbParms   = cv.cbState;
      pOptions->dwQuality = cv.lQ;
      if (cv.lKey != 0) {
	pOptions->dwKeyFrameEvery = cv.lKey;
	pOptions->dwFlags |= AVICOMPRESSF_KEYFRAMES;
      } else
	pOptions->dwFlags &= ~AVICOMPRESSF_KEYFRAMES;
      if (cv.lDataRate != 0) {
	pOptions->dwBytesPerSecond = cv.lDataRate * 1024; /* need bytes */
	pOptions->dwFlags |= AVICOMPRESSF_DATARATE;
      } else
	pOptions->dwFlags &= ~AVICOMPRESSF_DATARATE;
      pOptions->dwFlags  |= AVICOMPRESSF_VALID;
    }
    ICCompressorFree(&cv);

    return ret;
  } else if (sInfo.fccType == streamtypeAUDIO) {
    ACMFORMATCHOOSEW afmtc;
    MMRESULT         ret;
    LONG             size;

    /* FIXME: check ACM version -- Which version is needed? */

    memset(&afmtc, 0, sizeof(afmtc));
    afmtc.cbStruct  = sizeof(afmtc);
    afmtc.fdwStyle  = 0;
    afmtc.hwndOwner = hWnd;

    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &size);
    if ((pOptions->cbFormat == 0 || pOptions->lpFormat == NULL) && size != 0) {
      pOptions->lpFormat = malloc(size);
      if (!pOptions->lpFormat) return FALSE;
      pOptions->cbFormat = size;
    } else if (pOptions->cbFormat < (DWORD)size) {
      void *new_buffer = realloc(pOptions->lpFormat, size);
      if (!new_buffer) return FALSE;
      pOptions->lpFormat = new_buffer;
      pOptions->cbFormat = size;
    }
    afmtc.pwfx  = pOptions->lpFormat;
    afmtc.cbwfx = pOptions->cbFormat;

    size = 0;
    AVIStreamFormatSize(SaveOpts.ppavis[SaveOpts.nCurrent],
			sInfo.dwStart, &size);
    if (size < (LONG)sizeof(PCMWAVEFORMAT))
      size = sizeof(PCMWAVEFORMAT);
    afmtc.pwfxEnum = malloc(size);
    if (afmtc.pwfxEnum != NULL) {
      AVIStreamReadFormat(SaveOpts.ppavis[SaveOpts.nCurrent],
			  sInfo.dwStart, afmtc.pwfxEnum, &size);
      afmtc.fdwEnum = ACM_FORMATENUMF_CONVERT;
    }

    ret = acmFormatChooseW(&afmtc);
    if (ret == S_OK)
      pOptions->dwFlags |= AVICOMPRESSF_VALID;

    free(afmtc.pwfxEnum);
    return ret == S_OK;
  } else {
    ERR(": unknown streamtype 0x%08lX\n", sInfo.fccType);
    return FALSE;
  }
}

static void AVISaveOptionsUpdate(HWND hWnd)
{
  WCHAR          szFormat[128];
  AVISTREAMINFOW sInfo;
  LPVOID         lpFormat;
  LONG           size;

  TRACE("(%p)\n", hWnd);

  SaveOpts.nCurrent = SendDlgItemMessageW(hWnd,IDC_STREAM,CB_GETCURSEL,0,0);
  if (SaveOpts.nCurrent < 0)
    return;

  if (FAILED(AVIStreamInfoW(SaveOpts.ppavis[SaveOpts.nCurrent], &sInfo, sizeof(sInfo))))
    return;

  AVIStreamFormatSize(SaveOpts.ppavis[SaveOpts.nCurrent],sInfo.dwStart,&size);
  if (size > 0) {
    szFormat[0] = 0;

    /* read format to build format description string */
    lpFormat = malloc(size);
    if (lpFormat != NULL) {
      if (SUCCEEDED(AVIStreamReadFormat(SaveOpts.ppavis[SaveOpts.nCurrent],sInfo.dwStart,lpFormat, &size))) {
	if (sInfo.fccType == streamtypeVIDEO) {
	  LPBITMAPINFOHEADER lpbi = lpFormat;
	  ICINFO icinfo;

          wsprintfW(szFormat, L"%ldx%ldx%d", lpbi->biWidth,
		    lpbi->biHeight, lpbi->biBitCount);

	  if (lpbi->biCompression != BI_RGB) {
	    HIC    hic;

	    hic = ICLocate(ICTYPE_VIDEO, sInfo.fccHandler, lpFormat,
			   NULL, ICMODE_DECOMPRESS);
	    if (hic != NULL) {
	      if (ICGetInfo(hic, &icinfo, sizeof(icinfo)) == S_OK)
		lstrcatW(szFormat, icinfo.szDescription);
	      ICClose(hic);
	    }
	  } else {
	    LoadStringW(AVIFILE_hModule, IDS_UNCOMPRESSED,
			icinfo.szDescription,
			ARRAY_SIZE(icinfo.szDescription));
	    lstrcatW(szFormat, icinfo.szDescription);
	  }
	} else if (sInfo.fccType == streamtypeAUDIO) {
	  ACMFORMATTAGDETAILSW aftd;
	  ACMFORMATDETAILSW    afd;

	  memset(&aftd, 0, sizeof(aftd));
	  memset(&afd, 0, sizeof(afd));

	  aftd.cbStruct     = sizeof(aftd);
	  aftd.dwFormatTag  = afd.dwFormatTag =
	    ((PWAVEFORMATEX)lpFormat)->wFormatTag;
	  aftd.cbFormatSize = afd.cbwfx = size;

	  afd.cbStruct      = sizeof(afd);
	  afd.pwfx          = lpFormat;

	  if (acmFormatTagDetailsW(NULL, &aftd,
				   ACM_FORMATTAGDETAILSF_FORMATTAG) == S_OK) {
	    if (acmFormatDetailsW(NULL,&afd,ACM_FORMATDETAILSF_FORMAT) == S_OK)
              wsprintfW(szFormat, L"%s %s", afd.szFormat, aftd.szFormatTag);
	  }
	}
      }
      free(lpFormat);
    }

    /* set text for format description */
    SetDlgItemTextW(hWnd, IDC_FORMATTEXT, szFormat);

    /* Disable option button for unsupported streamtypes */
    if (sInfo.fccType == streamtypeVIDEO ||
	sInfo.fccType == streamtypeAUDIO)
      EnableWindow(GetDlgItem(hWnd, IDC_OPTIONS), TRUE);
    else
      EnableWindow(GetDlgItem(hWnd, IDC_OPTIONS), FALSE);
  }

}

static INT_PTR CALLBACK AVISaveOptionsDlgProc(HWND hWnd, UINT uMsg,
                                              WPARAM wParam, LPARAM lParam)
{
  DWORD dwInterleave;
  BOOL  bIsInterleaved;
  INT   n;

  /*TRACE("(%p,%u,0x%04X,0x%08lX)\n", hWnd, uMsg, wParam, lParam);*/

  switch (uMsg) {
  case WM_INITDIALOG:
    SaveOpts.nCurrent = 0;
    if (SaveOpts.nStreams == 1) {
      EndDialog(hWnd, AVISaveOptionsFmtChoose(hWnd));
      return TRUE;
    }

    /* add streams */
    for (n = 0; n < SaveOpts.nStreams; n++) {
      AVISTREAMINFOW sInfo;

      AVIStreamInfoW(SaveOpts.ppavis[n], &sInfo, sizeof(sInfo));
      SendDlgItemMessageW(hWnd, IDC_STREAM, CB_ADDSTRING,
			  0L, (LPARAM)sInfo.szName);
    }

    /* select first stream */
    SendDlgItemMessageW(hWnd, IDC_STREAM, CB_SETCURSEL, 0, 0);
    SendMessageW(hWnd, WM_COMMAND, MAKELONG(IDC_STREAM, CBN_SELCHANGE), (LPARAM)hWnd);

    /* initialize interleave */
    if (SaveOpts.ppOptions[0] != NULL &&
	(SaveOpts.ppOptions[0]->dwFlags & AVICOMPRESSF_VALID)) {
      bIsInterleaved = (SaveOpts.ppOptions[0]->dwFlags & AVICOMPRESSF_INTERLEAVE);
      dwInterleave = SaveOpts.ppOptions[0]->dwInterleaveEvery;
    } else {
      bIsInterleaved = TRUE;
      dwInterleave   = 0;
    }
    CheckDlgButton(hWnd, IDC_INTERLEAVE, bIsInterleaved);
    SetDlgItemInt(hWnd, IDC_INTERLEAVEEVERY, dwInterleave, FALSE);
    EnableWindow(GetDlgItem(hWnd, IDC_INTERLEAVEEVERY), bIsInterleaved);
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      /* get data from controls and save them */
      dwInterleave   = GetDlgItemInt(hWnd, IDC_INTERLEAVEEVERY, NULL, 0);
      bIsInterleaved = IsDlgButtonChecked(hWnd, IDC_INTERLEAVE);
      for (n = 0; n < SaveOpts.nStreams; n++) {
	if (SaveOpts.ppOptions[n] != NULL) {
	  if (bIsInterleaved) {
	    SaveOpts.ppOptions[n]->dwFlags |= AVICOMPRESSF_INTERLEAVE;
	    SaveOpts.ppOptions[n]->dwInterleaveEvery = dwInterleave;
	  } else
	    SaveOpts.ppOptions[n]->dwFlags &= ~AVICOMPRESSF_INTERLEAVE;
	}
      }
      /* fall through */
    case IDCANCEL:
      EndDialog(hWnd, LOWORD(wParam) == IDOK);
      break;
    case IDC_INTERLEAVE:
      EnableWindow(GetDlgItem(hWnd, IDC_INTERLEAVEEVERY),
		   IsDlgButtonChecked(hWnd, IDC_INTERLEAVE));
      break;
    case IDC_STREAM:
      if (HIWORD(wParam) == CBN_SELCHANGE) {
	/* update control elements */
	AVISaveOptionsUpdate(hWnd);
      }
      break;
    case IDC_OPTIONS:
      AVISaveOptionsFmtChoose(hWnd);
      break;
    };
    return TRUE;
  };

  return FALSE;
}

/***********************************************************************
 *		AVISaveOptions		(AVIFIL32.@)
 */
BOOL WINAPI AVISaveOptions(HWND hWnd, UINT uFlags, INT nStreams,
			   PAVISTREAM *ppavi, LPAVICOMPRESSOPTIONS *ppOptions)
{
  LPAVICOMPRESSOPTIONS pSavedOptions = NULL;
  INT ret, n;

  TRACE("(%p,0x%X,%d,%p,%p)\n", hWnd, uFlags, nStreams,
	ppavi, ppOptions);

  /* check parameters */
  if (nStreams <= 0 || ppavi == NULL || ppOptions == NULL)
    return AVIERR_BADPARAM;

  /* save options in case the user presses cancel */
  if (nStreams > 1) {
    pSavedOptions = malloc(nStreams * sizeof(AVICOMPRESSOPTIONS));
    if (pSavedOptions == NULL)
      return FALSE;

    for (n = 0; n < nStreams; n++) {
      if (ppOptions[n] != NULL)
	memcpy(pSavedOptions + n, ppOptions[n], sizeof(AVICOMPRESSOPTIONS));
    }
  }

  SaveOpts.uFlags    = uFlags;
  SaveOpts.nStreams  = nStreams;
  SaveOpts.ppavis    = ppavi;
  SaveOpts.ppOptions = ppOptions;

  ret = DialogBoxW(AVIFILE_hModule, MAKEINTRESOURCEW(IDD_SAVEOPTIONS),
		   hWnd, AVISaveOptionsDlgProc);

  if (ret == -1)
    ret = FALSE;

  /* restore options when user pressed cancel */
  if (pSavedOptions != NULL) {
    if (ret == FALSE) {
      for (n = 0; n < nStreams; n++) {
	if (ppOptions[n] != NULL)
	  memcpy(ppOptions[n], pSavedOptions + n, sizeof(AVICOMPRESSOPTIONS));
      }
    }
    free(pSavedOptions);
  }

  return ret;
}

/***********************************************************************
 *		AVISaveOptionsFree	(AVIFIL32.@)
 */
HRESULT WINAPI AVISaveOptionsFree(INT nStreams,LPAVICOMPRESSOPTIONS*ppOptions)
{
  TRACE("(%d,%p)\n", nStreams, ppOptions);

  if (nStreams < 0 || ppOptions == NULL)
    return AVIERR_BADPARAM;

  for (nStreams--; nStreams >= 0; nStreams--) {
    if (ppOptions[nStreams] != NULL) {
      ppOptions[nStreams]->dwFlags &= ~AVICOMPRESSF_VALID;

      if (ppOptions[nStreams]->lpParms != NULL) {
	free(ppOptions[nStreams]->lpParms);
	ppOptions[nStreams]->lpParms = NULL;
	ppOptions[nStreams]->cbParms = 0;
      }
      if (ppOptions[nStreams]->lpFormat != NULL) {
	free(ppOptions[nStreams]->lpFormat);
	ppOptions[nStreams]->lpFormat = NULL;
	ppOptions[nStreams]->cbFormat = 0;
      }
    }
  }

  return AVIERR_OK;
}

/***********************************************************************
 *		AVISaveVA		(AVIFIL32.@)
 */
HRESULT WINAPI AVISaveVA(LPCSTR szFile, CLSID *pclsidHandler,
			 AVISAVECALLBACK lpfnCallback, int nStream,
			 PAVISTREAM *ppavi, LPAVICOMPRESSOPTIONS *plpOptions)
{
  LPWSTR  wszFile = NULL;
  HRESULT hr;
  int     len;

  TRACE("(%s,%p,%p,%d,%p,%p)\n", debugstr_a(szFile), pclsidHandler,
	lpfnCallback, nStream, ppavi, plpOptions);

  if (szFile == NULL || ppavi == NULL || plpOptions == NULL)
    return AVIERR_BADPARAM;

  /* convert the ANSI string to Unicode and call the Unicode function */
  len = MultiByteToWideChar(CP_ACP, 0, szFile, -1, NULL, 0);
  if (len <= 0)
    return AVIERR_BADPARAM;

  wszFile = malloc(len * sizeof(WCHAR));
  if (wszFile == NULL)
    return AVIERR_MEMORY;

  MultiByteToWideChar(CP_ACP, 0, szFile, -1, wszFile, len);

  hr = AVISaveVW(wszFile, pclsidHandler, lpfnCallback,
		 nStream, ppavi, plpOptions);

  free(wszFile);

  return hr;
}

/***********************************************************************
 *		AVIFILE_AVISaveDefaultCallback	(internal)
 */
static BOOL WINAPI AVIFILE_AVISaveDefaultCallback(INT progress)
{
  TRACE("(%d)\n", progress);

  return FALSE;
}

/***********************************************************************
 *		AVISaveVW		(AVIFIL32.@)
 */
HRESULT WINAPI AVISaveVW(LPCWSTR szFile, CLSID *pclsidHandler,
			 AVISAVECALLBACK lpfnCallback, int nStreams,
			 PAVISTREAM *ppavi, LPAVICOMPRESSOPTIONS *plpOptions)
{
  LONG           lStart[MAX_AVISTREAMS];
  PAVISTREAM     pOutStreams[MAX_AVISTREAMS];
  PAVISTREAM     pInStreams[MAX_AVISTREAMS];
  AVIFILEINFOW   fInfo;
  AVISTREAMINFOW sInfo;

  PAVIFILE       pfile = NULL; /* the output AVI file */
  LONG           lFirstVideo = -1;
  int            curStream;

  /* for interleaving ... */
  DWORD          dwInterleave = 0; /* interleave rate */
  DWORD          dwFileInitialFrames;
  LONG           lFileLength;
  LONG           lSampleInc;

  /* for reading/writing the data ... */
  LPVOID         lpBuffer = NULL;
  LONG           cbBuffer;        /* real size of lpBuffer */
  LONG           lBufferSize;     /* needed bytes for format(s), etc. */
  LONG           lReadBytes;
  LONG           lReadSamples;
  HRESULT        hres;

  TRACE("(%s,%p,%p,%d,%p,%p)\n", debugstr_w(szFile), pclsidHandler,
	lpfnCallback, nStreams, ppavi, plpOptions);

  if (szFile == NULL || ppavi == NULL || plpOptions == NULL)
    return AVIERR_BADPARAM;
  if (nStreams >= MAX_AVISTREAMS) {
    WARN("Can't write AVI with %d streams only supports %d -- change MAX_AVISTREAMS!\n", nStreams, MAX_AVISTREAMS);
    return AVIERR_INTERNAL;
  }

  if (lpfnCallback == NULL)
    lpfnCallback = AVIFILE_AVISaveDefaultCallback;

  /* clear local variable(s) */
  for (curStream = 0; curStream < nStreams; curStream++) {
    pInStreams[curStream]  = NULL;
    pOutStreams[curStream] = NULL;
  }

  /* open output AVI file (create it if it doesn't exist) */
  hres = AVIFileOpenW(&pfile, szFile, OF_CREATE|OF_SHARE_EXCLUSIVE|OF_WRITE,
		      pclsidHandler);
  if (FAILED(hres))
    return hres;
  AVIFileInfoW(pfile, &fInfo, sizeof(fInfo)); /* for dwCaps */

  /* initialize our data structures part 1 */
  for (curStream = 0; curStream < nStreams; curStream++) {
    PAVISTREAM pCurStream = ppavi[curStream];

    hres = AVIStreamInfoW(pCurStream, &sInfo, sizeof(sInfo));
    if (FAILED(hres))
      goto error;

    /* search first video stream and check for interleaving */
    if (sInfo.fccType == streamtypeVIDEO) {
      /* remember first video stream -- needed for interleaving */
      if (lFirstVideo < 0)
	lFirstVideo = curStream;
    } else if (!dwInterleave) {
      /* check if any non-video stream wants to be interleaved */
      WARN("options.flags=0x%lX options.dwInterleave=%lu\n",plpOptions[curStream]->dwFlags,plpOptions[curStream]->dwInterleaveEvery);
      if (plpOptions[curStream] != NULL &&
	  plpOptions[curStream]->dwFlags & AVICOMPRESSF_INTERLEAVE)
	dwInterleave = plpOptions[curStream]->dwInterleaveEvery;
    }

    /* create de-/compressed stream interface if needed */
    pInStreams[curStream] = NULL;
    if (plpOptions[curStream] != NULL) {
      if (plpOptions[curStream]->fccHandler ||
	  plpOptions[curStream]->lpFormat != NULL) {
	DWORD dwKeySave = plpOptions[curStream]->dwKeyFrameEvery;

	if (fInfo.dwCaps & AVIFILECAPS_ALLKEYFRAMES)
	  plpOptions[curStream]->dwKeyFrameEvery = 1;

	hres = AVIMakeCompressedStream(&pInStreams[curStream], pCurStream,
				       plpOptions[curStream], NULL);
	plpOptions[curStream]->dwKeyFrameEvery = dwKeySave;
	if (FAILED(hres) || pInStreams[curStream] == NULL) {
	  pInStreams[curStream] = NULL;
	  goto error;
	}

	/* test stream interface and update stream-info */
	hres = AVIStreamInfoW(pInStreams[curStream], &sInfo, sizeof(sInfo));
	if (FAILED(hres))
	  goto error;
      }
    }

    /* now handle streams which will only be copied */
    if (pInStreams[curStream] == NULL) {
      pCurStream = pInStreams[curStream] = ppavi[curStream];
      AVIStreamAddRef(pCurStream);
    } else
      pCurStream = pInStreams[curStream];

    lStart[curStream] = sInfo.dwStart;
  } /* for all streams */

  /* check that first video stream is the first stream */
  if (lFirstVideo > 0) {
    PAVISTREAM pTmp = pInStreams[lFirstVideo];
    LONG lTmp = lStart[lFirstVideo];

    pInStreams[lFirstVideo] = pInStreams[0];
    pInStreams[0] = pTmp;
    lStart[lFirstVideo] = lStart[0];
    lStart[0] = lTmp;
    lFirstVideo = 0;
  }

  /* allocate buffer for formats, data, etc. of an initial size of 64 kBytes*/
  cbBuffer = 0x00010000;
  lpBuffer = malloc(cbBuffer);
  if (lpBuffer == NULL) {
    hres = AVIERR_MEMORY;
    goto error;
  }

  AVIStreamInfoW(pInStreams[0], &sInfo, sizeof(sInfo));
  lFileLength = sInfo.dwLength;
  dwFileInitialFrames = 0;
  if (lFirstVideo >= 0) {
    /* check for correct version of the format
     *  -- need at least BITMAPINFOHEADER or newer
     */
    lSampleInc = 1;
    lBufferSize = cbBuffer;
    hres = AVIStreamReadFormat(pInStreams[lFirstVideo], AVIStreamStart(pInStreams[lFirstVideo]), lpBuffer, &lBufferSize);
    if (lBufferSize < (LONG)sizeof(BITMAPINFOHEADER))
      hres = AVIERR_INTERNAL;
    if (FAILED(hres))
      goto error;
  } else /* use one second blocks for interleaving if no video present */
    lSampleInc = AVIStreamTimeToSample(pInStreams[0], 1000000);

  /* create output streams */
  for (curStream = 0; curStream < nStreams; curStream++) {
    AVIStreamInfoW(pInStreams[curStream], &sInfo, sizeof(sInfo));

    sInfo.dwInitialFrames = 0;
    if (dwInterleave != 0 && curStream > 0 && sInfo.fccType != streamtypeVIDEO) {
      /* 750 ms initial frames for non-video streams */
      sInfo.dwInitialFrames = AVIStreamTimeToSample(pInStreams[0], 750);
    }

    hres = AVIFileCreateStreamW(pfile, &pOutStreams[curStream], &sInfo);
    if (pOutStreams[curStream] != NULL && SUCCEEDED(hres)) {
      /* copy initial format for this stream */
      lBufferSize = cbBuffer;
      hres = AVIStreamReadFormat(pInStreams[curStream], sInfo.dwStart,
				 lpBuffer, &lBufferSize);
      if (FAILED(hres))
	goto error;
      hres = AVIStreamSetFormat(pOutStreams[curStream], 0, lpBuffer, lBufferSize);
      if (FAILED(hres))
	goto error;

      /* try to copy stream handler data */
      lBufferSize = cbBuffer;
      hres = AVIStreamReadData(pInStreams[curStream], ckidSTREAMHANDLERDATA,
			       lpBuffer, &lBufferSize);
      if (SUCCEEDED(hres) && lBufferSize > 0) {
	hres = AVIStreamWriteData(pOutStreams[curStream],ckidSTREAMHANDLERDATA,
				  lpBuffer, lBufferSize);
	if (FAILED(hres))
	  goto error;
      }

      if (dwFileInitialFrames < sInfo.dwInitialFrames)
	dwFileInitialFrames = sInfo.dwInitialFrames;
      lReadBytes =
	AVIStreamSampleToSample(pOutStreams[0], pInStreams[curStream],
				sInfo.dwLength);
      if (lFileLength < lReadBytes)
	lFileLength = lReadBytes;
    } else {
      /* creation of de-/compression stream interface failed */
      WARN("creation of (de-)compression stream failed for stream %d\n",curStream);
      AVIStreamRelease(pInStreams[curStream]);
      if (curStream + 1 >= nStreams) {
	/* move the others one up */
	PAVISTREAM *ppas = &pInStreams[curStream];
	int            n = nStreams - (curStream + 1);

	do {
	  *ppas = pInStreams[curStream + 1];
	} while (--n);
      }
      nStreams--;
      curStream--;
    }
  } /* create output streams for all input streams */

  /* have we still something to write, or lost everything? */
  if (nStreams <= 0)
    goto error;

  if (dwInterleave) {
    LONG lCurFrame = -dwFileInitialFrames;

    /* interleaved file */
    if (dwInterleave == 1)
      AVIFileEndRecord(pfile);

    for (; lCurFrame < lFileLength; lCurFrame += lSampleInc) {
      for (curStream = 0; curStream < nStreams; curStream++) {
	LONG lLastSample;

	hres = AVIStreamInfoW(pOutStreams[curStream], &sInfo, sizeof(sInfo));
	if (FAILED(hres))
	  goto error;

	/* initial frames phase at the end for this stream? */
	if (-(LONG)sInfo.dwInitialFrames > lCurFrame)
	  continue;

	if ((lFileLength - lSampleInc) <= lCurFrame) {
	  lLastSample = AVIStreamLength(pInStreams[curStream]);
	  lFirstVideo = lLastSample + AVIStreamStart(pInStreams[curStream]);
	} else {
	  if (curStream != 0) {
	    lFirstVideo =
	      AVIStreamSampleToSample(pInStreams[curStream], pInStreams[0],
				      (sInfo.fccType == streamtypeVIDEO ? 
				       (LONG)dwInterleave : lSampleInc) +
				      sInfo.dwInitialFrames + lCurFrame);
	  } else
	    lFirstVideo = lSampleInc + (sInfo.dwInitialFrames + lCurFrame);

	  lLastSample = AVIStreamEnd(pInStreams[curStream]);
	  if (lLastSample <= lFirstVideo)
	    lFirstVideo = lLastSample;
	}

	/* copy needed samples now */
	WARN("copy from stream %d samples %ld to %ld...\n",curStream,
	      lStart[curStream],lFirstVideo);
	while (lFirstVideo > lStart[curStream]) {
	  DWORD flags = 0;

	  /* copy format in case it can change */
	  lBufferSize = cbBuffer;
	  hres = AVIStreamReadFormat(pInStreams[curStream], lStart[curStream],
				     lpBuffer, &lBufferSize);
	  if (FAILED(hres))
	    goto error;
	  AVIStreamSetFormat(pOutStreams[curStream], lStart[curStream],
			     lpBuffer, lBufferSize);

	  /* try to read data until we got it, or error */
	  do {
	    hres = AVIStreamRead(pInStreams[curStream], lStart[curStream],
				 lFirstVideo - lStart[curStream], lpBuffer,
				 cbBuffer, &lReadBytes, &lReadSamples);
	  } while ((hres == AVIERR_BUFFERTOOSMALL) &&
		   (lpBuffer = realloc(lpBuffer, cbBuffer *= 2)) != NULL);
	  if (lpBuffer == NULL)
	    hres = AVIERR_MEMORY;
	  if (FAILED(hres))
	    goto error;

	  if (AVIStreamIsKeyFrame(pInStreams[curStream], (LONG)sInfo.dwStart))
	    flags = AVIIF_KEYFRAME;
	  hres = AVIStreamWrite(pOutStreams[curStream], -1, lReadSamples,
				lpBuffer, lReadBytes, flags, NULL, NULL);
	  if (FAILED(hres))
	    goto error;

	  lStart[curStream] += lReadSamples;
	}
	lStart[curStream] = lFirstVideo;
      } /* stream by stream */

      /* need to close this block? */
      if (dwInterleave == 1) {
	hres = AVIFileEndRecord(pfile);
	if (FAILED(hres))
	  break;
      }

      /* show progress */
      if (lpfnCallback(MulDiv(dwFileInitialFrames + lCurFrame, 100,
			      dwFileInitialFrames + lFileLength))) {
	hres = AVIERR_USERABORT;
	break;
      }
    } /* copy frame by frame */
  } else {
    /* non-interleaved file */

    for (curStream = 0; curStream < nStreams; curStream++) {
      /* show progress */
      if (lpfnCallback(MulDiv(curStream, 100, nStreams))) {
	hres = AVIERR_USERABORT;
	goto error;
      }

      AVIStreamInfoW(pInStreams[curStream], &sInfo, sizeof(sInfo));

      if (sInfo.dwSampleSize != 0) {
	/* sample-based data like audio */
	while (sInfo.dwStart < sInfo.dwLength) {
	  LONG lSamples = cbBuffer / sInfo.dwSampleSize;

	  /* copy format in case it can change */
	  lBufferSize = cbBuffer;
	  hres = AVIStreamReadFormat(pInStreams[curStream], sInfo.dwStart,
				     lpBuffer, &lBufferSize);
	  if (FAILED(hres))
	    goto error;
	  AVIStreamSetFormat(pOutStreams[curStream], sInfo.dwStart,
			     lpBuffer, lBufferSize);

	  /* limit to stream boundaries */
	  if (lSamples != (LONG)(sInfo.dwLength - sInfo.dwStart))
	    lSamples = sInfo.dwLength - sInfo.dwStart;

	  /* now try to read until we get it, or an error occurs */
	  do {
	    lReadBytes   = cbBuffer;
	    lReadSamples = 0;
	    hres = AVIStreamRead(pInStreams[curStream],sInfo.dwStart,lSamples,
				 lpBuffer,cbBuffer,&lReadBytes,&lReadSamples);
	  } while ((hres == AVIERR_BUFFERTOOSMALL) &&
		   (lpBuffer = realloc(lpBuffer, cbBuffer *= 2)) != NULL);
	  if (lpBuffer == NULL)
	    hres = AVIERR_MEMORY;
	  if (FAILED(hres))
	    goto error;
	  if (lReadSamples != 0) {
	    sInfo.dwStart += lReadSamples;
	    hres = AVIStreamWrite(pOutStreams[curStream], -1, lReadSamples,
				  lpBuffer, lReadBytes, 0, NULL , NULL);
	    if (FAILED(hres))
	      goto error;

	    /* show progress */
	    if (lpfnCallback(MulDiv(sInfo.dwStart,100,nStreams*sInfo.dwLength)+
			     MulDiv(curStream, 100, nStreams))) {
	      hres = AVIERR_USERABORT;
	      goto error;
	    }
	  } else {
	    if ((sInfo.dwLength - sInfo.dwStart) != 1) {
	      hres = AVIERR_FILEREAD;
	      goto error;
	    }
	  }
	}
      } else {
	/* block-based data like video */
	for (; sInfo.dwStart < sInfo.dwLength; sInfo.dwStart++) {
	  DWORD flags = 0;

	  /* copy format in case it can change */
	  lBufferSize = cbBuffer;
	  hres = AVIStreamReadFormat(pInStreams[curStream], sInfo.dwStart,
				     lpBuffer, &lBufferSize);
	  if (FAILED(hres))
	    goto error;
	  AVIStreamSetFormat(pOutStreams[curStream], sInfo.dwStart,
			     lpBuffer, lBufferSize);

	  /* try to read block and resize buffer if necessary */
	  do {
	    lReadSamples = 0;
	    lReadBytes   = cbBuffer;
	    hres = AVIStreamRead(pInStreams[curStream], sInfo.dwStart, 1,
				 lpBuffer, cbBuffer,&lReadBytes,&lReadSamples);
	  } while ((hres == AVIERR_BUFFERTOOSMALL) &&
		   (lpBuffer = realloc(lpBuffer, cbBuffer *= 2)) != NULL);
	  if (lpBuffer == NULL)
	    hres = AVIERR_MEMORY;
	  if (FAILED(hres))
	    goto error;
	  if (lReadSamples != 1) {
	    hres = AVIERR_FILEREAD;
	    goto error;
	  }

	  if (AVIStreamIsKeyFrame(pInStreams[curStream], (LONG)sInfo.dwStart))
	    flags = AVIIF_KEYFRAME;
	  hres = AVIStreamWrite(pOutStreams[curStream], -1, lReadSamples,
				lpBuffer, lReadBytes, flags, NULL, NULL);
	  if (FAILED(hres))
	    goto error;

	  /* show progress */
	  if (lpfnCallback(MulDiv(sInfo.dwStart,100,nStreams*sInfo.dwLength)+
			   MulDiv(curStream, 100, nStreams))) {
	    hres = AVIERR_USERABORT;
	    goto error;
	  }
	} /* copy all blocks */
      }
    } /* copy data stream by stream */
  }

 error:
  free(lpBuffer);
  if (pfile != NULL) {
    for (curStream = 0; curStream < nStreams; curStream++) {
      if (pOutStreams[curStream] != NULL)
	AVIStreamRelease(pOutStreams[curStream]);
      if (pInStreams[curStream] != NULL)
	AVIStreamRelease(pInStreams[curStream]);
    }

    AVIFileRelease(pfile);
  }

  return hres;
}

/***********************************************************************
 *		EditStreamClone		(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamClone(PAVISTREAM pStream, PAVISTREAM *ppResult)
{
  PAVIEDITSTREAM pEdit = NULL;
  HRESULT        hr;

  TRACE("(%p,%p)\n", pStream, ppResult);

  if (pStream == NULL)
    return AVIERR_BADHANDLE;
  if (ppResult == NULL)
    return AVIERR_BADPARAM;

  *ppResult = NULL;

  hr = IAVIStream_QueryInterface(pStream, &IID_IAVIEditStream,(LPVOID*)&pEdit);
  if (SUCCEEDED(hr) && pEdit != NULL) {
    hr = IAVIEditStream_Clone(pEdit, ppResult);

    IAVIEditStream_Release(pEdit);
  } else
    hr = AVIERR_UNSUPPORTED;

  return hr;
}

/***********************************************************************
 *		EditStreamCopy		(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamCopy(PAVISTREAM pStream, LONG *plStart,
			      LONG *plLength, PAVISTREAM *ppResult)
{
  PAVIEDITSTREAM pEdit = NULL;
  HRESULT        hr;

  TRACE("(%p,%p,%p,%p)\n", pStream, plStart, plLength, ppResult);

  if (pStream == NULL)
    return AVIERR_BADHANDLE;
  if (plStart == NULL || plLength == NULL || ppResult == NULL)
    return AVIERR_BADPARAM;

  *ppResult = NULL;

  hr = IAVIStream_QueryInterface(pStream, &IID_IAVIEditStream,(LPVOID*)&pEdit);
  if (SUCCEEDED(hr) && pEdit != NULL) {
    hr = IAVIEditStream_Copy(pEdit, plStart, plLength, ppResult);

    IAVIEditStream_Release(pEdit);
  } else
    hr = AVIERR_UNSUPPORTED;

  return hr;
}

/***********************************************************************
 *		EditStreamCut		(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamCut(PAVISTREAM pStream, LONG *plStart,
			     LONG *plLength, PAVISTREAM *ppResult)
{
  PAVIEDITSTREAM pEdit = NULL;
  HRESULT        hr;

  TRACE("(%p,%p,%p,%p)\n", pStream, plStart, plLength, ppResult);

  if (ppResult != NULL)
    *ppResult = NULL;
  if (pStream == NULL)
    return AVIERR_BADHANDLE;
  if (plStart == NULL || plLength == NULL)
    return AVIERR_BADPARAM;

  hr = IAVIStream_QueryInterface(pStream, &IID_IAVIEditStream,(LPVOID*)&pEdit);
  if (SUCCEEDED(hr) && pEdit != NULL) {
    hr = IAVIEditStream_Cut(pEdit, plStart, plLength, ppResult);

    IAVIEditStream_Release(pEdit);
  } else
    hr = AVIERR_UNSUPPORTED;

  return hr;
}

/***********************************************************************
 *		EditStreamPaste		(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamPaste(PAVISTREAM pDest, LONG *plStart, LONG *plLength,
			       PAVISTREAM pSource, LONG lStart, LONG lEnd)
{
  PAVIEDITSTREAM pEdit = NULL;
  HRESULT        hr;

  TRACE("(%p,%p,%p,%p,%ld,%ld)\n", pDest, plStart, plLength,
	pSource, lStart, lEnd);

  if (pDest == NULL || pSource == NULL)
    return AVIERR_BADHANDLE;
  if (plStart == NULL || plLength == NULL || lStart < 0)
    return AVIERR_BADPARAM;

  hr = IAVIStream_QueryInterface(pDest, &IID_IAVIEditStream,(LPVOID*)&pEdit);
  if (SUCCEEDED(hr) && pEdit != NULL) {
    hr = IAVIEditStream_Paste(pEdit, plStart, plLength, pSource, lStart, lEnd);

    IAVIEditStream_Release(pEdit);
  } else
    hr = AVIERR_UNSUPPORTED;

  return hr;
}

/***********************************************************************
 *		EditStreamSetInfoA	(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamSetInfoA(PAVISTREAM pstream, LPAVISTREAMINFOA asi,
				  LONG size)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (size >= 0 && size < sizeof(AVISTREAMINFOA))
    return AVIERR_BADSIZE;

  memcpy(&asiw, asi, sizeof(asiw) - sizeof(asiw.szName));
  MultiByteToWideChar(CP_ACP, 0, asi->szName, -1, asiw.szName, ARRAY_SIZE(asiw.szName));

  return EditStreamSetInfoW(pstream, &asiw, sizeof(asiw));
}

/***********************************************************************
 *		EditStreamSetInfoW	(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamSetInfoW(PAVISTREAM pstream, LPAVISTREAMINFOW asi,
				  LONG size)
{
  PAVIEDITSTREAM pEdit = NULL;
  HRESULT        hr;

  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (size >= 0 && size < sizeof(AVISTREAMINFOA))
    return AVIERR_BADSIZE;

  hr = IAVIStream_QueryInterface(pstream, &IID_IAVIEditStream,(LPVOID*)&pEdit);
  if (SUCCEEDED(hr) && pEdit != NULL) {
    hr = IAVIEditStream_SetInfo(pEdit, asi, size);

    IAVIEditStream_Release(pEdit);
  } else
    hr = AVIERR_UNSUPPORTED;

  return hr;
}

/***********************************************************************
 *		EditStreamSetNameA	(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamSetNameA(PAVISTREAM pstream, LPCSTR szName)
{
  AVISTREAMINFOA asia;
  HRESULT        hres;

  TRACE("(%p,%s)\n", pstream, debugstr_a(szName));

  if (pstream == NULL)
    return AVIERR_BADHANDLE;
  if (szName == NULL)
    return AVIERR_BADPARAM;

  hres = AVIStreamInfoA(pstream, &asia, sizeof(asia));
  if (FAILED(hres))
    return hres;

  memset(asia.szName, 0, sizeof(asia.szName));
  lstrcpynA(asia.szName, szName, ARRAY_SIZE(asia.szName));

  return EditStreamSetInfoA(pstream, &asia, sizeof(asia));
}

/***********************************************************************
 *		EditStreamSetNameW	(AVIFIL32.@)
 */
HRESULT WINAPI EditStreamSetNameW(PAVISTREAM pstream, LPCWSTR szName)
{
  AVISTREAMINFOW asiw;
  HRESULT        hres;

  TRACE("(%p,%s)\n", pstream, debugstr_w(szName));

  if (pstream == NULL)
    return AVIERR_BADHANDLE;
  if (szName == NULL)
    return AVIERR_BADPARAM;

  hres = IAVIStream_Info(pstream, &asiw, sizeof(asiw));
  if (FAILED(hres))
    return hres;

  memset(asiw.szName, 0, sizeof(asiw.szName));
  lstrcpynW(asiw.szName, szName, ARRAY_SIZE(asiw.szName));

  return EditStreamSetInfoW(pstream, &asiw, sizeof(asiw));
}

/***********************************************************************
 *		AVIClearClipboard	(AVIFIL32.@)
 */
HRESULT WINAPI AVIClearClipboard(void)
{
  TRACE("()\n");

  return AVIERR_UNSUPPORTED; /* OleSetClipboard(NULL); */
}

/***********************************************************************
 *		AVIGetFromClipboard	(AVIFIL32.@)
 */
HRESULT WINAPI AVIGetFromClipboard(PAVIFILE *ppfile)
{
  FIXME("(%p), stub!\n", ppfile);

  *ppfile = NULL;

  return AVIERR_UNSUPPORTED;
}

/***********************************************************************
 *      AVIMakeStreamFromClipboard (AVIFIL32.@)
 */
HRESULT WINAPI AVIMakeStreamFromClipboard(UINT cfFormat, HANDLE hGlobal,
                                          PAVISTREAM * ppstream)
{
  FIXME("(0x%08x,%p,%p), stub!\n", cfFormat, hGlobal, ppstream);

  if (ppstream == NULL)
    return AVIERR_BADHANDLE;

  return AVIERR_UNSUPPORTED;
}

/***********************************************************************
 *		AVIPutFileOnClipboard	(AVIFIL32.@)
 */
HRESULT WINAPI AVIPutFileOnClipboard(PAVIFILE pfile)
{
  FIXME("(%p), stub!\n", pfile);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return AVIERR_UNSUPPORTED;
}

HRESULT WINAPIV AVISaveA(LPCSTR szFile, CLSID * pclsidHandler, AVISAVECALLBACK lpfnCallback,
                        int nStreams, PAVISTREAM pavi, LPAVICOMPRESSOPTIONS lpOptions, ...)
{
    va_list vl;
    int i;
    HRESULT ret;
    PAVISTREAM *streams;
    LPAVICOMPRESSOPTIONS *options;

    TRACE("(%s,%p,%p,%d,%p,%p)\n", debugstr_a(szFile), pclsidHandler, lpfnCallback,
          nStreams, pavi, lpOptions);

    if (nStreams <= 0) return AVIERR_BADPARAM;

    streams = malloc(nStreams * sizeof(*streams));
    options = malloc(nStreams * sizeof(*options));
    if (!streams || !options)
    {
        ret = AVIERR_MEMORY;
        goto error;
    }

    streams[0] = pavi;
    options[0] = lpOptions;

    va_start(vl, lpOptions);
    for (i = 1; i < nStreams; i++)
    {
        streams[i] = va_arg(vl, PAVISTREAM);
        options[i] = va_arg(vl, PAVICOMPRESSOPTIONS);
    }
    va_end(vl);

    for (i = 0; i < nStreams; i++)
        TRACE("Pair[%d] - Stream = %p, Options = %p\n", i, streams[i], options[i]);

    ret = AVISaveVA(szFile, pclsidHandler, lpfnCallback, nStreams, streams, options);
error:
    free(streams);
    free(options);
    return ret;
}

HRESULT WINAPIV AVISaveW(LPCWSTR szFile, CLSID * pclsidHandler, AVISAVECALLBACK lpfnCallback,
                        int nStreams, PAVISTREAM pavi, LPAVICOMPRESSOPTIONS lpOptions, ...)
{
    va_list vl;
    int i;
    HRESULT ret;
    PAVISTREAM *streams;
    LPAVICOMPRESSOPTIONS *options;

    TRACE("(%s,%p,%p,%d,%p,%p)\n", debugstr_w(szFile), pclsidHandler, lpfnCallback,
          nStreams, pavi, lpOptions);

    if (nStreams <= 0) return AVIERR_BADPARAM;

    streams = malloc(nStreams * sizeof(*streams));
    options = malloc(nStreams * sizeof(*options));
    if (!streams || !options)
    {
        ret = AVIERR_MEMORY;
        goto error;
    }

    streams[0] = pavi;
    options[0] = lpOptions;

    va_start(vl, lpOptions);
    for (i = 1; i < nStreams; i++)
    {
        streams[i] = va_arg(vl, PAVISTREAM);
        options[i] = va_arg(vl, PAVICOMPRESSOPTIONS);
    }
    va_end(vl);

    for (i = 0; i < nStreams; i++)
        TRACE("Pair[%d] - Stream = %p, Options = %p\n", i, streams[i], options[i]);

    ret = AVISaveVW(szFile, pclsidHandler, lpfnCallback, nStreams, streams, options);
error:
    free(streams);
    free(options);
    return ret;
}
