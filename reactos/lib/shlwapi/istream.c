/*
 * SHLWAPI IStream functions
 *
 * Copyright 2002 Jon Griffiths
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Layout of ISHFileStream object */
typedef struct
{
  ICOM_VFIELD(IStream);
  ULONG    ref;
  HANDLE   hFile;
  DWORD    dwMode;
  LPOLESTR lpszPath;
  DWORD    type;
  DWORD    grfStateBits;
} ISHFileStream;

static HRESULT WINAPI IStream_fnCommit(IStream*,DWORD);


/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface, REFIID riid, LPVOID *ppvObj)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown) ||
     IsEqualIID(riid, &IID_IStream))
  {
    *ppvObj = This;

    IStream_AddRef((IStream*)*ppvObj);
    return S_OK;
  }
  return E_NOINTERFACE;
}

/**************************************************************************
*  IStream_fnAddRef
*/
static ULONG WINAPI IStream_fnAddRef(IStream *iface)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p)\n", This);
  return InterlockedIncrement(&This->ref);
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
  ICOM_THIS(ISHFileStream, iface);
  ULONG ulRet;

  TRACE("(%p)\n", This);

  if (!(ulRet = InterlockedDecrement(&This->ref)))
  {
    IStream_fnCommit(iface, 0); /* If ever buffered, this will be needed */
    LocalFree((HLOCAL)This->lpszPath);
    CloseHandle(This->hFile);
    HeapFree(GetProcessHeap(), 0, This);
  }
  return ulRet;
}

/**************************************************************************
 * IStream_fnRead
 */
static HRESULT WINAPI IStream_fnRead(IStream *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
  ICOM_THIS(ISHFileStream, iface);
  HRESULT hRet = S_OK;
  DWORD dwRead = 0;

  TRACE("(%p,%p,0x%08lx,%p)\n", This, pv, cb, pcbRead);

  if (!pv)
    hRet = STG_E_INVALIDPOINTER;
  else if (!ReadFile(This->hFile, pv, cb, &dwRead, NULL))
  {
    hRet = (HRESULT)GetLastError();
    if(hRet > 0)
      hRet = HRESULT_FROM_WIN32(hRet);
  }
  if (pcbRead)
    *pcbRead = dwRead;
  return hRet;
}

/**************************************************************************
 * IStream_fnWrite
 */
static HRESULT WINAPI IStream_fnWrite(IStream *iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
  ICOM_THIS(ISHFileStream, iface);
  HRESULT hRet = S_OK;
  DWORD dwWritten = 0;

  TRACE("(%p,%p,0x%08lx,%p)\n", This, pv, cb, pcbWritten);

  if (!pv)
    hRet = STG_E_INVALIDPOINTER;
  else if (!(This->dwMode & STGM_WRITE))
    hRet = E_FAIL;
  else if (!WriteFile(This->hFile, pv, cb, &dwWritten, NULL))
  {
    hRet = (HRESULT)GetLastError();
    if(hRet > 0)
      hRet = HRESULT_FROM_WIN32(hRet);
  }
  if (pcbWritten)
    *pcbWritten = dwWritten;
  return hRet;
}

/**************************************************************************
 *  IStream_fnSeek
 */
static HRESULT WINAPI IStream_fnSeek(IStream *iface, LARGE_INTEGER dlibMove,
                                     DWORD dwOrigin, ULARGE_INTEGER* pNewPos)
{
  ICOM_THIS(ISHFileStream, iface);
  DWORD dwPos;

  TRACE("(%p,%ld,%ld,%p)\n", This, dlibMove.u.LowPart, dwOrigin, pNewPos);

  IStream_fnCommit(iface, 0); /* If ever buffered, this will be needed */
  dwPos = SetFilePointer(This->hFile, dlibMove.u.LowPart, NULL, dwOrigin);

  if (pNewPos)
  {
    pNewPos->u.HighPart = 0;
    pNewPos->u.LowPart = dwPos;
  }
  return S_OK;
}

/**************************************************************************
 * IStream_fnSetSize
 */
static HRESULT WINAPI IStream_fnSetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p,%ld)\n", This, libNewSize.u.LowPart);
  IStream_fnCommit(iface, 0); /* If ever buffered, this will be needed */
  return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnCopyTo
 */
static HRESULT WINAPI IStream_fnCopyTo(IStream *iface, IStream* pstm, ULARGE_INTEGER cb,
                                       ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
  ICOM_THIS(ISHFileStream, iface);
  char copyBuff[1024];
  ULONGLONG ulSize;
  HRESULT hRet = S_OK;

  TRACE("(%p,%p,%ld,%p,%p)\n", This, pstm, cb.u.LowPart, pcbRead, pcbWritten);

  if (pcbRead)
    pcbRead->QuadPart = 0;
  if (pcbWritten)
    pcbWritten->QuadPart = 0;

  if (!pstm)
    return STG_E_INVALIDPOINTER;

  IStream_fnCommit(iface, 0); /* If ever buffered, this will be needed */

  /* Copy data */
  ulSize = cb.QuadPart;
  while (ulSize)
  {
    ULONG ulLeft, ulAmt;

    ulLeft = ulSize > sizeof(copyBuff) ? sizeof(copyBuff) : ulSize;

    /* Read */
    hRet = IStream_fnRead(iface, copyBuff, ulLeft, &ulAmt);
    if (pcbRead)
      pcbRead->QuadPart += ulAmt;
    if (FAILED(hRet) || ulAmt != ulLeft)
      break;

    /* Write */
    hRet = IStream_fnWrite(pstm, copyBuff, ulLeft, &ulAmt);
    if (pcbWritten)
      pcbWritten->QuadPart += ulAmt;
    if (FAILED(hRet) || ulAmt != ulLeft)
      break;

    ulSize -= ulLeft;
  }
  return hRet;
}

/**************************************************************************
 * IStream_fnCommit
 */
static HRESULT WINAPI IStream_fnCommit(IStream *iface, DWORD grfCommitFlags)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p,%ld)\n", This, grfCommitFlags);
  /* Currently unbuffered: This function is not needed */
  return S_OK;
}

/**************************************************************************
 * IStream_fnRevert
 */
static HRESULT WINAPI IStream_fnRevert(IStream *iface)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p)\n", This);
  return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnLockUnlockRegion
 */
static HRESULT WINAPI IStream_fnLockUnlockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                                 ULARGE_INTEGER cb, DWORD dwLockType)
{
  ICOM_THIS(ISHFileStream, iface);
  TRACE("(%p,%ld,%ld,%ld)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
  return E_NOTIMPL;
}

/*************************************************************************
 * IStream_fnStat
 */
static HRESULT WINAPI IStream_fnStat(IStream *iface, STATSTG* lpStat,
                                     DWORD grfStatFlag)
{
  ICOM_THIS(ISHFileStream, iface);
  BY_HANDLE_FILE_INFORMATION fi;
  HRESULT hRet = S_OK;

  TRACE("(%p,%p,%ld)\n", This, lpStat, grfStatFlag);

  if (!grfStatFlag)
    hRet = STG_E_INVALIDPOINTER;
  else
  {
    memset(&fi, 0, sizeof(fi));
    GetFileInformationByHandle(This->hFile, &fi);

    if (grfStatFlag & STATFLAG_NONAME)
      lpStat->pwcsName = NULL;
    else
      lpStat->pwcsName = StrDupW(This->lpszPath);
    lpStat->type = This->type;
    lpStat->cbSize.u.LowPart = fi.nFileSizeLow;
    lpStat->cbSize.u.HighPart = fi.nFileSizeHigh;
    lpStat->mtime = fi.ftLastWriteTime;
    lpStat->ctime = fi.ftCreationTime;
    lpStat->atime = fi.ftLastAccessTime;
    lpStat->grfMode = This->dwMode;
    lpStat->grfLocksSupported = 0;
    memcpy(&lpStat->clsid, &IID_IStream, sizeof(CLSID));
    lpStat->grfStateBits = This->grfStateBits;
    lpStat->reserved = 0;
  }
  return hRet;
}

/*************************************************************************
 * IStream_fnClone
 */
static HRESULT WINAPI IStream_fnClone(IStream *iface, IStream** ppstm)
{
  ICOM_THIS(ISHFileStream, iface);

  TRACE("(%p)\n",This);
  if (ppstm)
    *ppstm = NULL;
  return E_NOTIMPL;
}

static struct ICOM_VTABLE(IStream) SHLWAPI_fsVTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IStream_fnQueryInterface,
  IStream_fnAddRef,
  IStream_fnRelease,
  IStream_fnRead,
  IStream_fnWrite,
  IStream_fnSeek,
  IStream_fnSetSize,
  IStream_fnCopyTo,
  IStream_fnCommit,
  IStream_fnRevert,
  IStream_fnLockUnlockRegion,
  IStream_fnLockUnlockRegion,
  IStream_fnStat,
  IStream_fnClone
};

/**************************************************************************
 * IStream_Create
 *
 * Internal helper: Create and initialise a new file stream object.
 */
static IStream *IStream_Create(LPCWSTR lpszPath, HANDLE hFile, DWORD dwMode)
{
 ISHFileStream* fileStream;

 fileStream = (ISHFileStream*)HeapAlloc(GetProcessHeap(), 0, sizeof(ISHFileStream));

 if (fileStream)
 {
   fileStream->lpVtbl = &SHLWAPI_fsVTable;
   fileStream->ref = 1;
   fileStream->hFile = hFile;
   fileStream->dwMode = dwMode;
   fileStream->lpszPath = StrDupW(lpszPath);
   fileStream->type = 0; /* FIXME */
   fileStream->grfStateBits = 0; /* FIXME */
 }
 TRACE ("Returning %p\n", fileStream);
 return (IStream *)fileStream;
}

/*************************************************************************
 * SHCreateStreamOnFileEx   [SHLWAPI.@]
 *
 * Create a stream on a file.
 *
 * PARAMS
 *  lpszPath     [I] Path of file to create stream on
 *  dwMode       [I] Mode to create stream in
 *  dwAttributes [I] Attributes of the file
 *  bCreate      [I] Whether to create the file if it doesn't exist
 *  lpTemplate   [I] Reserved, must be NULL
 *  lppStream    [O] Destination for created stream
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new stream object
 * Failure: E_INVALIDARG if any parameter is invalid, or an HRESULT error code
 *
 * NOTES
 *  This function is available in Unicode only.
 */
HRESULT WINAPI SHCreateStreamOnFileEx(LPCWSTR lpszPath, DWORD dwMode,
                                      DWORD dwAttributes, BOOL bCreate,
                                      IStream *lpTemplate, IStream **lppStream)
{
  DWORD dwAccess, dwShare, dwCreate;
  HANDLE hFile;

  TRACE("(%s,%ld,0x%08lX,%d,%p,%p)\n", debugstr_w(lpszPath), dwMode,
        dwAttributes, bCreate, lpTemplate, lppStream);

  if (!lpszPath || !lppStream || lpTemplate)
    return E_INVALIDARG;

  *lppStream = NULL;

  if (dwMode & ~(STGM_WRITE|STGM_READWRITE|STGM_SHARE_DENY_NONE|STGM_SHARE_DENY_READ|STGM_CREATE))
  {
    WARN("Invalid mode 0x%08lX\n", dwMode);
    return E_INVALIDARG;
  }

  /* Access */
  switch (dwMode & (STGM_WRITE|STGM_READWRITE))
  {
  case STGM_READWRITE|STGM_WRITE:
  case STGM_READWRITE:
    dwAccess = GENERIC_READ|GENERIC_WRITE;
    break;
  case STGM_WRITE:
    dwAccess = GENERIC_WRITE;
    break;
  default:
    dwAccess = GENERIC_READ;
    break;
  }

  /* Sharing */
  switch (dwMode & STGM_SHARE_DENY_READ)
  {
  case STGM_SHARE_DENY_READ:
    dwShare = FILE_SHARE_WRITE;
    break;
  case STGM_SHARE_DENY_WRITE:
    dwShare = FILE_SHARE_READ;
    break;
  case STGM_SHARE_EXCLUSIVE:
    dwShare = 0;
    break;
  default:
    dwShare = FILE_SHARE_READ|FILE_SHARE_WRITE;
  }

  /* FIXME: Creation Flags, MSDN is fuzzy on the mapping... */
  if (dwMode == STGM_FAILIFTHERE)
    dwCreate = bCreate ? CREATE_NEW : OPEN_EXISTING;
  else if (dwMode & STGM_CREATE)
    dwCreate = CREATE_ALWAYS;
  else
    dwCreate = OPEN_ALWAYS;

  /* Open HANDLE to file */
  hFile = CreateFileW(lpszPath, dwAccess, dwShare, NULL, dwCreate,
                      dwAttributes, 0);

  if(hFile == INVALID_HANDLE_VALUE)
  {
    HRESULT hRet = (HRESULT)GetLastError();
    if(hRet > 0)
      hRet = HRESULT_FROM_WIN32(hRet);
    return hRet;
  }

  *lppStream = IStream_Create(lpszPath, hFile, dwMode);

  if(!*lppStream)
  {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }
  return S_OK;
}

/*************************************************************************
 * SHCreateStreamOnFileW   [SHLWAPI.@]
 *
 * See SHCreateStreamOnFileA.
 */
HRESULT WINAPI SHCreateStreamOnFileW(LPCWSTR lpszPath, DWORD dwMode,
                                   IStream **lppStream)
{
  DWORD dwAttr;

  TRACE("(%s,%ld,%p)\n", debugstr_w(lpszPath), dwMode, lppStream);

  if (!lpszPath || !lppStream)
    return E_INVALIDARG;

  dwAttr = GetFileAttributesW(lpszPath);
  if (dwAttr == INVALID_FILE_ATTRIBUTES)
    dwAttr = 0;

  return SHCreateStreamOnFileEx(lpszPath, dwMode|STGM_WRITE, dwAttr,
                                TRUE, NULL, lppStream);
}

/*************************************************************************
 * SHCreateStreamOnFileA   [SHLWAPI.@]
 *
 * Create a stream on a file.
 *
 * PARAMS
 *  lpszPath  [I] Path of file to create stream on
 *  dwMode    [I] Mode to create stream in
 *  lppStream [O] Destination for created IStream object
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new IStream object
 * Failure: E_INVALIDARG if any parameter is invalid, or an HRESULT error code
 */
HRESULT WINAPI SHCreateStreamOnFileA(LPCSTR lpszPath, DWORD dwMode,
                                     IStream **lppStream)
{
  WCHAR szPath[MAX_PATH];

  TRACE("(%s,%ld,%p)\n", debugstr_a(lpszPath), dwMode, lppStream);

  if (!lpszPath)
    return E_INVALIDARG;
  MultiByteToWideChar(0, 0, lpszPath, -1, szPath, MAX_PATH);
  return SHCreateStreamOnFileW(szPath, dwMode, lppStream);
}

/*************************************************************************
 * @       [SHLWAPI.184]
 *
 * Call IStream_Read() on a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpvDest  [O] Destination for data read
 *  ulSize   [I] Size of data to read
 *
 * RETURNS
 *  Success: S_OK. ulSize bytes have been read from the stream into lpvDest.
 *  Failure: An HRESULT error code, or E_FAIL if the read succeeded but the
 *           number of bytes read does not match.
 */
HRESULT WINAPI SHLWAPI_184(IStream *lpStream, LPVOID lpvDest, ULONG ulSize)
{
  ULONG ulRead;
  HRESULT hRet;

  TRACE("(%p,%p,%ld)\n", lpStream, lpvDest, ulSize);

  hRet = IStream_Read(lpStream, lpvDest, ulSize, &ulRead);

  if (SUCCEEDED(hRet) && ulRead != ulSize)
    hRet = E_FAIL;
  return hRet;
}

/*************************************************************************
 * @       [SHLWAPI.166]
 *
 * Determine if a stream has 0 length.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *
 * RETURNS
 *  TRUE:  If the stream has 0 length
 *  FALSE: Otherwise.
 */
BOOL WINAPI SHIsEmptyStream(IStream *lpStream)
{
  STATSTG statstg;
  BOOL bRet = TRUE;

  TRACE("(%p)\n", lpStream);

  memset(&statstg, 0, sizeof(statstg));

  if(SUCCEEDED(IStream_Stat(lpStream, &statstg, 1)))
  {
    if(statstg.cbSize.QuadPart)
      bRet = FALSE; /* Non-Zero */
  }
  else
  {
    DWORD dwDummy;

    /* Try to read from the stream */
    if(SUCCEEDED(SHLWAPI_184(lpStream, &dwDummy, sizeof(dwDummy))))
    {
      LARGE_INTEGER zero;
      zero.QuadPart = 0;

      IStream_Seek(lpStream, zero, 0, NULL);
      bRet = FALSE; /* Non-Zero */
    }
  }
  return bRet;
}

/*************************************************************************
 * @       [SHLWAPI.212]
 *
 * Call IStream_Write() on a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpvSrc   [I] Source for data to write
 *  ulSize   [I] Size of data
 *
 * RETURNS
 *  Success: S_OK. ulSize bytes have been written to the stream from lpvSrc.
 *  Failure: An HRESULT error code, or E_FAIL if the write succeeded but the
 *           number of bytes written does not match.
 */
HRESULT WINAPI SHLWAPI_212(IStream *lpStream, LPCVOID lpvSrc, ULONG ulSize)
{
  ULONG ulWritten;
  HRESULT hRet;

  TRACE("(%p,%p,%ld)\n", lpStream, lpvSrc, ulSize);

  hRet = IStream_Write(lpStream, lpvSrc, ulSize, &ulWritten);

  if (SUCCEEDED(hRet) && ulWritten != ulSize)
    hRet = E_FAIL;

  return hRet;
}

/*************************************************************************
 * @       [SHLWAPI.213]
 *
 * Seek to the start of a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *
 * RETURNS
 *  Success: S_OK. The current position within the stream is updated
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI IStream_Reset(IStream *lpStream)
{
  LARGE_INTEGER zero;
  TRACE("(%p)\n", lpStream);
  zero.QuadPart = 0;
  return IStream_Seek(lpStream, zero, 0, NULL);
}

/*************************************************************************
 * @       [SHLWAPI.214]
 *
 * Get the size of a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpulSize [O] Destination for size
 *
 * RETURNS
 *  Success: S_OK. lpulSize contains the size of the stream.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI IStream_Size(IStream *lpStream, ULARGE_INTEGER* lpulSize)
{
  STATSTG statstg;
  HRESULT hRet;

  TRACE("(%p,%p)\n", lpStream, lpulSize);

  memset(&statstg, 0, sizeof(statstg));

  hRet = IStream_Stat(lpStream, &statstg, 1);

  if (SUCCEEDED(hRet) && lpulSize)
    *lpulSize = statstg.cbSize;
  return hRet;
}
