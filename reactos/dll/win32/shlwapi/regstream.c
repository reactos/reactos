/*
 * SHLWAPI Registry Stream functions
 *
 * Copyright 1999 Juergen Schmied
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

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
	const IStreamVtbl *lpVtbl;
	LONG   ref;
	HKEY   hKey;
	LPBYTE pbBuffer;
	DWORD  dwLength;
	DWORD  dwPos;
} ISHRegStream;

/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface, REFIID riid, LPVOID *ppvObj)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))	/*IUnknown*/
	  *ppvObj = This;
	else if(IsEqualIID(riid, &IID_IStream))	/*IStream*/
	  *ppvObj = This;

	if(*ppvObj)
	{
	  IStream_AddRef((IStream*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IStream_fnAddRef
*/
static ULONG WINAPI IStream_fnAddRef(IStream *iface)
{
	ISHRegStream *This = (ISHRegStream *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);
	
	TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
	ISHRegStream *This = (ISHRegStream *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying SHReg IStream (%p)\n",This);

          HeapFree(GetProcessHeap(),0,This->pbBuffer);

	  if (This->hKey)
	    RegCloseKey(This->hKey);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}

	return refCount;
}

/**************************************************************************
 * IStream_fnRead
 */
static HRESULT WINAPI IStream_fnRead (IStream * iface, void* pv, ULONG cb, ULONG* pcbRead)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	DWORD dwBytesToRead, dwBytesLeft;

	TRACE("(%p)->(%p,0x%08lx,%p)\n",This, pv, cb, pcbRead);

	if (!pv)
	  return STG_E_INVALIDPOINTER;

	dwBytesLeft = This->dwLength - This->dwPos;

	if ( 0 >= dwBytesLeft ) /* end of buffer */
	  return S_FALSE;

	dwBytesToRead = ( cb > dwBytesLeft) ? dwBytesLeft : cb;

	memmove ( pv, (This->pbBuffer) + (This->dwPos), dwBytesToRead);

	This->dwPos += dwBytesToRead; /* adjust pointer */

	if (pcbRead)
	  *pcbRead = dwBytesToRead;

	return S_OK;
}

/**************************************************************************
 * IStream_fnWrite
 */
static HRESULT WINAPI IStream_fnWrite (IStream * iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	if (pcbWritten)
	  *pcbWritten = 0;

	return E_NOTIMPL;
}

/**************************************************************************
 *  IStream_fnSeek
 */
static HRESULT WINAPI IStream_fnSeek (IStream * iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	if (plibNewPosition)
	  plibNewPosition->QuadPart = 0;
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnSetSize
 */
static HRESULT WINAPI IStream_fnSetSize (IStream * iface, ULARGE_INTEGER libNewSize)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnCopyTo
 */
static HRESULT WINAPI IStream_fnCopyTo (IStream * iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);
	if (pcbRead)
	  pcbRead->QuadPart = 0;
	if (pcbWritten)
	  pcbWritten->QuadPart = 0;
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnCommit
 */
static HRESULT WINAPI IStream_fnCommit (IStream * iface, DWORD grfCommitFlags)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnRevert
 */
static HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnLockUnlockRegion
 */
static HRESULT WINAPI IStream_fnLockUnlockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}

/*************************************************************************
 * IStream_fnStat
 */
static HRESULT WINAPI IStream_fnStat (IStream * iface, STATSTG*   pstatstg, DWORD grfStatFlag)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}

/*************************************************************************
 * IStream_fnClone
 */
static HRESULT WINAPI IStream_fnClone (IStream * iface, IStream** ppstm)
{
	ISHRegStream *This = (ISHRegStream *)iface;

	TRACE("(%p)\n",This);
	if (ppstm)
	  *ppstm = NULL;
	return E_NOTIMPL;
}

static const IStreamVtbl rstvt =
{
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

/* Methods overridden by the dummy stream */

/**************************************************************************
 *  IStream_fnAddRefDummy
 */
static ULONG WINAPI IStream_fnAddRefDummy(IStream *iface)
{
	ISHRegStream *This = (ISHRegStream *)iface;
	TRACE("(%p)\n", This);
	return 2;
}

/**************************************************************************
 *  IStream_fnReleaseDummy
 */
static ULONG WINAPI IStream_fnReleaseDummy(IStream *iface)
{
	ISHRegStream *This = (ISHRegStream *)iface;
	TRACE("(%p)\n", This);
	return 1;
}

/**************************************************************************
 * IStream_fnReadDummy
 */
static HRESULT WINAPI IStream_fnReadDummy(IStream *iface, LPVOID pv, ULONG cb, ULONG* pcbRead)
{
  if (pcbRead)
    *pcbRead = 0;
  return E_NOTIMPL;
}

static const IStreamVtbl DummyRegStreamVTable =
{
  IStream_fnQueryInterface,
  IStream_fnAddRefDummy,  /* Overridden */
  IStream_fnReleaseDummy, /* Overridden */
  IStream_fnReadDummy,    /* Overridden */
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

/* Dummy registry stream object */
static ISHRegStream rsDummyRegStream =
{
 &DummyRegStreamVTable,
 1,
 NULL,
 NULL,
 0,
 0
};

/**************************************************************************
 * IStream_Create
 *
 * Internal helper: Create and initialise a new registry stream object.
 */
static IStream *IStream_Create(HKEY hKey, LPBYTE pbBuffer, DWORD dwLength)
{
 ISHRegStream* regStream;

 regStream = HeapAlloc(GetProcessHeap(), 0, sizeof(ISHRegStream));

 if (regStream)
 {
   regStream->lpVtbl = &rstvt;
   regStream->ref = 1;
   regStream->hKey = hKey;
   regStream->pbBuffer = pbBuffer;
   regStream->dwLength = dwLength;
   regStream->dwPos = 0;
 }
 TRACE ("Returning %p\n", regStream);
 return (IStream *)regStream;
}

/*************************************************************************
 * SHOpenRegStream2A	[SHLWAPI.@]
 *
 * Create a stream to read binary registry data.
 *
 * PARAMS
 * hKey      [I] Registry handle
 * pszSubkey [I] The sub key name
 * pszValue  [I] The value name under the sub key
 * dwMode    [I] Unused
 *
 * RETURNS
 * Success: An IStream interface referring to the registry data
 * Failure: NULL, if the registry key could not be opened or is not binary.
 */
IStream * WINAPI SHOpenRegStream2A(HKEY hKey, LPCSTR pszSubkey,
                                   LPCSTR pszValue,DWORD dwMode)
{
  HKEY hStrKey = NULL;
  LPBYTE lpBuff = NULL;
  DWORD dwLength, dwType;

  TRACE("(%p,%s,%s,0x%08lx)\n", hKey, pszSubkey, pszValue, dwMode);

  /* Open the key, read in binary data and create stream */
  if (!RegOpenKeyExA (hKey, pszSubkey, 0, KEY_READ, &hStrKey) &&
      !RegQueryValueExA (hStrKey, pszValue, 0, 0, 0, &dwLength) &&
      (lpBuff = HeapAlloc (GetProcessHeap(), 0, dwLength)) &&
      !RegQueryValueExA (hStrKey, pszValue, 0, &dwType, lpBuff, &dwLength) &&
      dwType == REG_BINARY)
    return IStream_Create(hStrKey, lpBuff, dwLength);

  HeapFree (GetProcessHeap(), 0, lpBuff);
  if (hStrKey)
    RegCloseKey(hStrKey);
  return NULL;
}

/*************************************************************************
 * SHOpenRegStream2W	[SHLWAPI.@]
 *
 * See SHOpenRegStream2A.
 */
IStream * WINAPI SHOpenRegStream2W(HKEY hKey, LPCWSTR pszSubkey,
                                   LPCWSTR pszValue, DWORD dwMode)
{
  HKEY hStrKey = NULL;
  LPBYTE lpBuff = NULL;
  DWORD dwLength, dwType;

  TRACE("(%p,%s,%s,0x%08lx)\n", hKey, debugstr_w(pszSubkey),
        debugstr_w(pszValue), dwMode);

  /* Open the key, read in binary data and create stream */
  if (!RegOpenKeyExW (hKey, pszSubkey, 0, KEY_READ, &hStrKey) &&
      !RegQueryValueExW (hStrKey, pszValue, 0, 0, 0, &dwLength) &&
      (lpBuff = HeapAlloc (GetProcessHeap(), 0, dwLength)) &&
      !RegQueryValueExW (hStrKey, pszValue, 0, &dwType, lpBuff, &dwLength) &&
      dwType == REG_BINARY)
    return IStream_Create(hStrKey, lpBuff, dwLength);

  HeapFree (GetProcessHeap(), 0, lpBuff);
  if (hStrKey)
    RegCloseKey(hStrKey);
  return NULL;
}

/*************************************************************************
 * SHOpenRegStreamA     [SHLWAPI.@]
 *
 * Create a stream to read binary registry data.
 *
 * PARAMS
 * hKey      [I] Registry handle
 * pszSubkey [I] The sub key name
 * pszValue  [I] The value name under the sub key
 * dwMode    [I] STGM mode for opening the file
 *
 * RETURNS
 * Success: An IStream interface referring to the registry data
 * Failure: If the registry key could not be opened or is not binary,
 *          A dummy (empty) IStream object is returned.
 */
IStream * WINAPI SHOpenRegStreamA(HKEY hkey, LPCSTR pszSubkey,
                                  LPCSTR pszValue, DWORD dwMode)
{
  IStream *iStream;

  TRACE("(%p,%s,%s,0x%08lx)\n", hkey, pszSubkey, pszValue, dwMode);

  iStream = SHOpenRegStream2A(hkey, pszSubkey, pszValue, dwMode);
  return iStream ? iStream : (IStream *)&rsDummyRegStream;
}

/*************************************************************************
 * SHOpenRegStreamW	[SHLWAPI.@]
 *
 * See SHOpenRegStreamA.
 */
IStream * WINAPI SHOpenRegStreamW(HKEY hkey, LPCWSTR pszSubkey,
                                  LPCWSTR pszValue, DWORD dwMode)
{
  IStream *iStream;

  TRACE("(%p,%s,%s,0x%08lx)\n", hkey, debugstr_w(pszSubkey),
        debugstr_w(pszValue), dwMode);
  iStream = SHOpenRegStream2W(hkey, pszSubkey, pszValue, dwMode);
  return iStream ? iStream : (IStream *)&rsDummyRegStream;
}

/*************************************************************************
 * @   [SHLWAPI.12]
 *
 * Create an IStream object on a block of memory.
 *
 * PARAMS
 * lpbData   [I] Memory block to create the IStream object on
 * dwDataLen [I] Length of data block
 *
 * RETURNS
 * Success: A pointer to the IStream object.
 * Failure: NULL, if any parameters are invalid or an error occurs.
 *
 * NOTES
 *  A copy of the memory pointed to by lpbData is made, and is freed
 *  when the stream is released.
 */
IStream * WINAPI SHCreateMemStream(LPBYTE lpbData, DWORD dwDataLen)
{
  IStream *iStrmRet = NULL;

  TRACE("(%p,%ld)\n", lpbData, dwDataLen);

  if (lpbData)
  {
    LPBYTE lpbDup = HeapAlloc(GetProcessHeap(), 0, dwDataLen);

    if (lpbDup)
    {
      memcpy(lpbDup, lpbData, dwDataLen);
      iStrmRet = IStream_Create(NULL, lpbDup, dwDataLen);

      if (!iStrmRet)
        HeapFree(GetProcessHeap(), 0, lpbDup);
    }
  }
  return iStrmRet;
}

/*************************************************************************
 * SHCreateStreamWrapper   [SHLWAPI.@]
 *
 * Create an IStream object on a block of memory.
 *
 * PARAMS
 * lpbData    [I] Memory block to create the IStream object on
 * dwDataLen  [I] Length of data block
 * dwReserved [I] Reserved, Must be 0.
 * lppStream  [O] Destination for IStream object
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new IStream object.
 * Failure: E_INVALIDARG, if any parameters are invalid,
 *          E_OUTOFMEMORY if memory allocation fails.
 *
 * NOTES
 *  The stream assumes ownership of the memory passed to it.
 */
HRESULT WINAPI SHCreateStreamWrapper(LPBYTE lpbData, DWORD dwDataLen,
                                     DWORD dwReserved, IStream **lppStream)
{
  IStream* lpStream;

  if (lppStream)
    *lppStream = NULL;

  if(dwReserved || !lppStream)
    return E_INVALIDARG;

  lpStream = IStream_Create(NULL, lpbData, dwDataLen);

  if(!lpStream)
    return E_OUTOFMEMORY;

  IStream_QueryInterface(lpStream, &IID_IStream, (void**)lppStream);
  IStream_Release(lpStream);
  return S_OK;
}
