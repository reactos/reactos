/*
 * Based on ../shell32/memorystream.c
 *
 * Copyright 1999 Juergen Schmied
 * Copyright 2003 Mike McCormack for CodeWeavers
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
#include "winreg.h"
#include "winternl.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"
#include "shlwapi.h"
#include "urlmon_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static const IStreamVtbl stvt;

HRESULT UMCreateStreamOnCacheFile(LPCWSTR pszURL,
                                  DWORD dwSize,
                                  LPWSTR pszFileName,
                                  HANDLE *phfile,
                                  IUMCacheStream **ppstr)
{
    IUMCacheStream* ucstr;
    HANDLE handle;
    DWORD size;
    LPWSTR url, c, ext = NULL;
    HRESULT hr;

    size = (strlenW(pszURL)+1)*sizeof(WCHAR);
    url = HeapAlloc(GetProcessHeap(), 0, size);
    memcpy(url, pszURL, size);

    for (c = url; *c && *c != '#' && *c != '?'; ++c)
    {
        if (*c == '.')
            ext = c+1;
        else if(*c == '/')
            ext = NULL;
    }

    *c = 0;

    if(!CreateUrlCacheEntryW(url, dwSize, ext, pszFileName, 0))
       hr = HRESULT_FROM_WIN32(GetLastError());
    else
       hr = 0;

    HeapFree(GetProcessHeap(), 0, url);

    if (hr)
       return hr;

    TRACE("Opening %s\n", debugstr_w(pszFileName) );

    handle = CreateFileW( pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL );
    if( handle == INVALID_HANDLE_VALUE )
       return HRESULT_FROM_WIN32(GetLastError());

    if (phfile)
    {
       /* Call CreateFileW again because we need a handle with its own file pointer, and DuplicateHandle will return
        * a handle that shares its file pointer with the original.
        */
           *phfile = CreateFileW( pszFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

       if (*phfile == (HANDLE) HFILE_ERROR)
       {
           DWORD dwError = GetLastError();

           CloseHandle(handle);
           return HRESULT_FROM_WIN32(dwError);
       }
    }

    ucstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(IUMCacheStream));
    if(ucstr )
    {
       ucstr->pszURL = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(WCHAR) * (lstrlenW(pszURL) + 1));
       if (ucstr->pszURL)
       {
            ucstr->pszFileName = HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(WCHAR) * (lstrlenW(pszFileName) + 1));
           if (ucstr->pszFileName)
           {
              ucstr->lpVtbl=&stvt;
              ucstr->ref = 1;
              ucstr->handle = handle;
              ucstr->closed = 0;
              lstrcpyW(ucstr->pszURL, pszURL);
              lstrcpyW(ucstr->pszFileName, pszFileName);

              *ppstr = ucstr;

              return S_OK;
           }
           HeapFree(GetProcessHeap(), 0, ucstr->pszURL);
       }
       HeapFree(GetProcessHeap(), 0, ucstr);
    }
    CloseHandle(handle);
    if (phfile)
       CloseHandle(*phfile);
    return E_OUTOFMEMORY;
}

void UMCloseCacheFileStream(IUMCacheStream *This)
{
    if (!This->closed)
    {
       FILETIME ftZero;

       ftZero.dwLowDateTime = ftZero.dwHighDateTime = 0;

       This->closed = 1;
       CommitUrlCacheEntryW(This->pszURL,
                            This->pszFileName,
                            ftZero,
                            ftZero,
                            NORMAL_CACHE_ENTRY,
                            0,
                            0,
                            0,
                            0);
    }
}

/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface,
                                               REFIID riid,
                                               LPVOID *ppvObj)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IStream))
    {
      *ppvObj = This;
    }

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
    IUMCacheStream *This = (IUMCacheStream *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount)
    {
       TRACE(" destroying UMCacheStream (%p)\n",This);
       UMCloseCacheFileStream(This);
       CloseHandle(This->handle);
       HeapFree(GetProcessHeap(), 0, This->pszFileName);
       HeapFree(GetProcessHeap(), 0, This->pszURL);
       HeapFree(GetProcessHeap(),0,This);
    }
    return refCount;
}

static HRESULT WINAPI IStream_fnRead (IStream * iface,
                                      void* pv,
                                      ULONG cb,
                                      ULONG* pcbRead)
{
    ULONG dwBytesRead;
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)->(%p,0x%08x,%p)\n",This, pv, cb, pcbRead);

    if ( !pv )
       return STG_E_INVALIDPOINTER;

    if ( !pcbRead)
        pcbRead = &dwBytesRead;

    if ( ! ReadFile( This->handle, pv, cb, (LPDWORD)pcbRead, NULL ) )
       return S_FALSE;

    if (!*pcbRead)
        return This->closed ? S_FALSE : E_PENDING;
    return S_OK;
}

static HRESULT WINAPI IStream_fnWrite (IStream * iface,
                                       const void* pv,
                                       ULONG cb,
                                       ULONG* pcbWritten)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IStream_fnSeek (IStream * iface,
                                   LARGE_INTEGER dlibMove,
                                   DWORD dwOrigin,
                                   ULARGE_INTEGER* plibNewPosition)
{
    LARGE_INTEGER newpos;
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    if (!SetFilePointerEx( This->handle, dlibMove, &newpos, dwOrigin ))
       return E_FAIL;

    if (plibNewPosition)
        plibNewPosition->QuadPart = newpos.QuadPart;

    return S_OK;
}

static HRESULT WINAPI IStream_fnSetSize (IStream * iface,
                                         ULARGE_INTEGER libNewSize)
{
    LARGE_INTEGER newpos;
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    newpos.QuadPart = libNewSize.QuadPart;
    if( ! SetFilePointerEx( This->handle, newpos, NULL, FILE_BEGIN ) )
       return E_FAIL;

    if( ! SetEndOfFile( This->handle ) )
       return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI IStream_fnCopyTo (IStream * iface,
                                   IStream* pstm,
                                   ULARGE_INTEGER cb,
                                   ULARGE_INTEGER* pcbRead,
                                   ULARGE_INTEGER* pcbWritten)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IStream_fnCommit (IStream * iface,
                                   DWORD grfCommitFlags)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnLockRegion (IStream * iface,
                                            ULARGE_INTEGER libOffset,
                                            ULARGE_INTEGER cb,
                                            DWORD dwLockType)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnUnlockRegion (IStream * iface,
                                              ULARGE_INTEGER libOffset,
                                              ULARGE_INTEGER cb,
                                              DWORD dwLockType)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnStat (IStream * iface,
                                      STATSTG*   pstatstg,
                                      DWORD grfStatFlag)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnClone (IStream * iface,
                                       IStream** ppstm)
{
    IUMCacheStream *This = (IUMCacheStream *)iface;

    TRACE("(%p)\n",This);

    return E_NOTIMPL;
}

static const IStreamVtbl stvt =
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
    IStream_fnLockRegion,
    IStream_fnUnlockRegion,
    IStream_fnStat,
    IStream_fnClone

};
