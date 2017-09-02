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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

typedef struct
{
	IStream IStream_iface;
	LONG   ref;
	HKEY   hKey;
	LPBYTE pbBuffer;
	DWORD  dwLength;
	DWORD  dwPos;
	DWORD  dwMode;
	union {
	    LPSTR keyNameA;
	    LPWSTR keyNameW;
	}u;
	BOOL   bUnicode;
} ISHRegStream;

static inline ISHRegStream *impl_from_IStream(IStream *iface)
{
	return CONTAINING_RECORD(iface, ISHRegStream, IStream_iface);
}

/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface, REFIID riid, LPVOID *ppvObj)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

       if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IStream))
         *ppvObj = &This->IStream_iface;

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
	ISHRegStream *This = impl_from_IStream(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);
	
	TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
	ISHRegStream *This = impl_from_IStream(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying SHReg IStream (%p)\n",This);

	  if (This->hKey)
	  {
	    /* write back data in REG_BINARY */
	    if (This->dwMode == STGM_READWRITE || This->dwMode == STGM_WRITE)
	    {
	      if (This->dwLength)
	      {
	        if (This->bUnicode)
	          RegSetValueExW(This->hKey, This->u.keyNameW, 0, REG_BINARY,
	                         (const BYTE *) This->pbBuffer, This->dwLength);
	        else
	          RegSetValueExA(This->hKey, This->u.keyNameA, 0, REG_BINARY,
	                        (const BYTE *) This->pbBuffer, This->dwLength);
	      }
	      else
	      {
	        if (This->bUnicode)
	          RegDeleteValueW(This->hKey, This->u.keyNameW);
	        else
	          RegDeleteValueA(This->hKey, This->u.keyNameA);
	      }
	    }

	    RegCloseKey(This->hKey);
	  }

	  HeapFree(GetProcessHeap(),0,This->u.keyNameA);
	  HeapFree(GetProcessHeap(),0,This->pbBuffer);
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
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD dwBytesToRead;

	TRACE("(%p)->(%p,0x%08x,%p)\n",This, pv, cb, pcbRead);

	if (This->dwPos >= This->dwLength)
	  dwBytesToRead = 0;
        else
	  dwBytesToRead = This->dwLength - This->dwPos;

	dwBytesToRead = (cb > dwBytesToRead) ? dwBytesToRead : cb;
	if (dwBytesToRead != 0) /* not at end of buffer and we want to read something */
	{
	  memmove(pv, This->pbBuffer + This->dwPos, dwBytesToRead);
	  This->dwPos += dwBytesToRead; /* adjust pointer */
	}

	if (pcbRead)
	  *pcbRead = dwBytesToRead;

	return S_OK;
}

/**************************************************************************
 * IStream_fnWrite
 */
static HRESULT WINAPI IStream_fnWrite (IStream * iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD newLen = This->dwPos + cb;

	TRACE("(%p, %p, %d, %p)\n",This, pv, cb, pcbWritten);

	if (newLen < This->dwPos) /* overflow */
	  return STG_E_INSUFFICIENTMEMORY;

	if (newLen > This->dwLength)
	{
	  LPBYTE newBuf = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->pbBuffer, newLen);
	  if (!newBuf)
	    return STG_E_INSUFFICIENTMEMORY;

	  This->dwLength = newLen;
	  This->pbBuffer = newBuf;
	}
	memmove(This->pbBuffer + This->dwPos, pv, cb);
	This->dwPos += cb; /* adjust pointer */

	if (pcbWritten)
	  *pcbWritten = cb;

	return S_OK;
}

/**************************************************************************
 *  IStream_fnSeek
 */
static HRESULT WINAPI IStream_fnSeek (IStream * iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ISHRegStream *This = impl_from_IStream(iface);
	LARGE_INTEGER tmp;
	TRACE("(%p, %s, %d %p)\n", This,
              wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

	if (dwOrigin == STREAM_SEEK_SET)
	  tmp = dlibMove;
        else if (dwOrigin == STREAM_SEEK_CUR)
	  tmp.QuadPart = This->dwPos + dlibMove.QuadPart;
	else if (dwOrigin == STREAM_SEEK_END)
	  tmp.QuadPart = This->dwLength + dlibMove.QuadPart;
        else
	  return STG_E_INVALIDPARAMETER;

	if (tmp.QuadPart < 0)
	  return STG_E_INVALIDFUNCTION;

	/* we cut off the high part here */
	This->dwPos = tmp.u.LowPart;

	if (plibNewPosition)
	  plibNewPosition->QuadPart = This->dwPos;
	return S_OK;
}

/**************************************************************************
 * IStream_fnSetSize
 */
static HRESULT WINAPI IStream_fnSetSize (IStream * iface, ULARGE_INTEGER libNewSize)
{
	ISHRegStream *This = impl_from_IStream(iface);
	DWORD newLen;
	LPBYTE newBuf;

	TRACE("(%p, %s)\n", This, wine_dbgstr_longlong(libNewSize.QuadPart));

	/* we cut off the high part here */
	newLen = libNewSize.u.LowPart;
	newBuf = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->pbBuffer, newLen);
	if (!newBuf)
	  return STG_E_INSUFFICIENTMEMORY;

	This->pbBuffer = newBuf;
	This->dwLength = newLen;

	return S_OK;
}

/**************************************************************************
 * IStream_fnCopyTo
 */
static HRESULT WINAPI IStream_fnCopyTo (IStream * iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);
	if (pcbRead)
	  pcbRead->QuadPart = 0;
	if (pcbWritten)
	  pcbWritten->QuadPart = 0;

	/* TODO implement */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnCommit
 */
static HRESULT WINAPI IStream_fnCommit (IStream * iface, DWORD grfCommitFlags)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* commit not supported by this stream */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnRevert
 */
static HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* revert not supported by this stream */
	return E_NOTIMPL;
}

/**************************************************************************
 * IStream_fnLockUnlockRegion
 */
static HRESULT WINAPI IStream_fnLockUnlockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);

	/* lock/unlock not supported by this stream */
	return E_NOTIMPL;
}

/*************************************************************************
 * IStream_fnStat
 */
static HRESULT WINAPI IStream_fnStat (IStream * iface, STATSTG* pstatstg, DWORD grfStatFlag)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p, %p, %d)\n",This,pstatstg,grfStatFlag);

	pstatstg->pwcsName = NULL;
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = This->dwLength;
	pstatstg->mtime.dwHighDateTime = 0;
	pstatstg->mtime.dwLowDateTime = 0;
	pstatstg->ctime.dwHighDateTime = 0;
	pstatstg->ctime.dwLowDateTime = 0;
	pstatstg->atime.dwHighDateTime = 0;
	pstatstg->atime.dwLowDateTime = 0;
	pstatstg->grfMode = This->dwMode;
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;

	return S_OK;
}

/*************************************************************************
 * IStream_fnClone
 */
static HRESULT WINAPI IStream_fnClone (IStream * iface, IStream** ppstm)
{
	ISHRegStream *This = impl_from_IStream(iface);

	TRACE("(%p)\n",This);
	*ppstm = NULL;

	/* clone not supported by this stream */
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
	ISHRegStream *This = impl_from_IStream(iface);
	TRACE("(%p)\n", This);
	return 2;
}

/**************************************************************************
 *  IStream_fnReleaseDummy
 */
static ULONG WINAPI IStream_fnReleaseDummy(IStream *iface)
{
	ISHRegStream *This = impl_from_IStream(iface);
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
 { &DummyRegStreamVTable },
 1,
 NULL,
 NULL,
 0,
 0,
 STGM_READWRITE,
 {NULL},
 FALSE
};

/**************************************************************************
 * IStream_Create
 *
 * Internal helper: Create and initialise a new registry stream object.
 */
static ISHRegStream *IStream_Create(HKEY hKey, LPBYTE pbBuffer, DWORD dwLength)
{
 ISHRegStream* regStream;

 regStream = HeapAlloc(GetProcessHeap(), 0, sizeof(ISHRegStream));

 if (regStream)
 {
   regStream->IStream_iface.lpVtbl = &rstvt;
   regStream->ref = 1;
   regStream->hKey = hKey;
   regStream->pbBuffer = pbBuffer;
   regStream->dwLength = dwLength;
   regStream->dwPos = 0;
   regStream->dwMode = STGM_READWRITE;
   regStream->u.keyNameA = NULL;
   regStream->bUnicode = FALSE;
 }
 TRACE ("Returning %p\n", regStream);
 return regStream;
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
  ISHRegStream *tmp;
  HKEY hStrKey = NULL;
  LPBYTE lpBuff = NULL;
  DWORD dwLength = 0;
  LONG ret;

  TRACE("(%p,%s,%s,0x%08x)\n", hKey, pszSubkey, pszValue, dwMode);

  if (dwMode == STGM_READ)
    ret = RegOpenKeyExA(hKey, pszSubkey, 0, KEY_READ, &hStrKey);
  else /* in write mode we make sure the subkey exits */
    ret = RegCreateKeyExA(hKey, pszSubkey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hStrKey, NULL);

  if (ret == ERROR_SUCCESS)
  {
    if (dwMode == STGM_READ || dwMode == STGM_READWRITE)
    {
      /* read initial data */
      ret = RegQueryValueExA(hStrKey, pszValue, 0, 0, 0, &dwLength);
      if (ret == ERROR_SUCCESS && dwLength)
      {
        lpBuff = HeapAlloc(GetProcessHeap(), 0, dwLength);
        RegQueryValueExA(hStrKey, pszValue, 0, 0, lpBuff, &dwLength);
      }
    }

    if (!dwLength)
      lpBuff = HeapAlloc(GetProcessHeap(), 0, dwLength);

    tmp = IStream_Create(hStrKey, lpBuff, dwLength);
    if(tmp)
    {
      if(pszValue)
      {
        int len = lstrlenA(pszValue) + 1;
        tmp->u.keyNameA = HeapAlloc(GetProcessHeap(), 0, len);
        memcpy(tmp->u.keyNameA, pszValue, len);
      }

      tmp->dwMode = dwMode;
      tmp->bUnicode = FALSE;
      return &tmp->IStream_iface;
    }
  }

  HeapFree(GetProcessHeap(), 0, lpBuff);
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
  ISHRegStream *tmp;
  HKEY hStrKey = NULL;
  LPBYTE lpBuff = NULL;
  DWORD dwLength = 0;
  LONG ret;

  TRACE("(%p,%s,%s,0x%08x)\n", hKey, debugstr_w(pszSubkey),
        debugstr_w(pszValue), dwMode);

  if (dwMode == STGM_READ)
    ret = RegOpenKeyExW(hKey, pszSubkey, 0, KEY_READ, &hStrKey);
  else /* in write mode we make sure the subkey exits */
    ret = RegCreateKeyExW(hKey, pszSubkey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hStrKey, NULL);

  if (ret == ERROR_SUCCESS)
  {
    if (dwMode == STGM_READ || dwMode == STGM_READWRITE)
    {
      /* read initial data */
      ret = RegQueryValueExW(hStrKey, pszValue, 0, 0, 0, &dwLength);
      if (ret == ERROR_SUCCESS && dwLength)
      {
        lpBuff = HeapAlloc(GetProcessHeap(), 0, dwLength);
        RegQueryValueExW(hStrKey, pszValue, 0, 0, lpBuff, &dwLength);
      }
    }

    if (!dwLength)
      lpBuff = HeapAlloc(GetProcessHeap(), 0, dwLength);

    tmp = IStream_Create(hStrKey, lpBuff, dwLength);
    if(tmp)
    {
      if(pszValue)
      {
        int len = lstrlenW(pszValue) + 1;
        tmp->u.keyNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        memcpy(tmp->u.keyNameW, pszValue, len * sizeof(WCHAR));
      }

      tmp->dwMode = dwMode;
      tmp->bUnicode = TRUE;
      return &tmp->IStream_iface;
    }
  }

  HeapFree(GetProcessHeap(), 0, lpBuff);
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

  TRACE("(%p,%s,%s,0x%08x)\n", hkey, pszSubkey, pszValue, dwMode);

  iStream = SHOpenRegStream2A(hkey, pszSubkey, pszValue, dwMode);
  return iStream ? iStream : &rsDummyRegStream.IStream_iface;
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

  TRACE("(%p,%s,%s,0x%08x)\n", hkey, debugstr_w(pszSubkey),
        debugstr_w(pszValue), dwMode);
  iStream = SHOpenRegStream2W(hkey, pszSubkey, pszValue, dwMode);
  return iStream ? iStream : &rsDummyRegStream.IStream_iface;
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
IStream * WINAPI SHCreateMemStream(const BYTE *lpbData, UINT dwDataLen)
{
  ISHRegStream *strm = NULL;
  LPBYTE lpbDup;

  TRACE("(%p,%d)\n", lpbData, dwDataLen);

  if (!lpbData)
    dwDataLen = 0;

  lpbDup = HeapAlloc(GetProcessHeap(), 0, dwDataLen);

  if (lpbDup)
  {
    memcpy(lpbDup, lpbData, dwDataLen);
    strm = IStream_Create(NULL, lpbDup, dwDataLen);

    if (!strm)
      HeapFree(GetProcessHeap(), 0, lpbDup);
  }
  return &strm->IStream_iface;
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
  ISHRegStream *strm;

  if (lppStream)
    *lppStream = NULL;

  if(dwReserved || !lppStream)
    return E_INVALIDARG;

  strm = IStream_Create(NULL, lpbData, dwDataLen);

  if(!strm)
    return E_OUTOFMEMORY;

  IStream_QueryInterface(&strm->IStream_iface, &IID_IStream, (void**)lppStream);
  IStream_Release(&strm->IStream_iface);
  return S_OK;
}
