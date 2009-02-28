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

#include "urlmon_main.h"

#include "winreg.h"
#include "winternl.h"
#include "wininet.h"
#include "shlwapi.h"

#include "wine/debug.h"

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
    url = heap_alloc(size);
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
       hr = S_OK;

    heap_free(url);

    if (hr != S_OK)
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

    ucstr = heap_alloc_zero(sizeof(IUMCacheStream));
    if(ucstr)
    {
       ucstr->pszURL = heap_alloc_zero(sizeof(WCHAR) * (lstrlenW(pszURL) + 1));
       if (ucstr->pszURL)
       {
            ucstr->pszFileName = heap_alloc_zero(sizeof(WCHAR) * (lstrlenW(pszFileName) + 1));
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
           heap_free(ucstr->pszURL);
       }
       heap_free(ucstr);
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
       heap_free(This->pszFileName);
       heap_free(This->pszURL);
       heap_free(This);
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

    if ( ! ReadFile( This->handle, pv, cb, pcbRead, NULL ) )
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

typedef struct ProxyBindStatusCallback
{
    const IBindStatusCallbackVtbl *lpVtbl;

    IBindStatusCallback *pBSC;
} ProxyBindStatusCallback;

static HRESULT WINAPI ProxyBindStatusCallback_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&IID_IBindStatusCallback, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ProxyBindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI ProxyBindStatusCallback_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved,
                                               IBinding *pib)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnStartBinding(This->pBSC, dwReserved, pib);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_GetPriority(This->pBSC, pnPriority);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnLowResource(This->pBSC, reserved);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
                                           ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnProgress(This->pBSC, ulProgress,
                                          ulProgressMax, ulStatusCode,
                                          szStatusText);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnStopBinding(This->pBSC, hresult, szError);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_GetBindInfo(This->pBSC, grfBINDF, pbindinfo);

    return E_INVALIDARG;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
                                                              DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnDataAvailable(This->pBSC, grfBSCF, dwSize,
                                               pformatetc, pstgmed);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;

    if(This->pBSC)
        return IBindStatusCallback_OnObjectAvailable(This->pBSC, riid, punk);

    return S_OK;
}

static HRESULT WINAPI BlockingBindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
                                                                 DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    return S_OK;
}

static const IBindStatusCallbackVtbl BlockingBindStatusCallbackVtbl =
{
    ProxyBindStatusCallback_QueryInterface,
    ProxyBindStatusCallback_AddRef,
    ProxyBindStatusCallback_Release,
    ProxyBindStatusCallback_OnStartBinding,
    ProxyBindStatusCallback_GetPriority,
    ProxyBindStatusCallback_OnLowResource,
    ProxyBindStatusCallback_OnProgress,
    ProxyBindStatusCallback_OnStopBinding,
    ProxyBindStatusCallback_GetBindInfo,
    BlockingBindStatusCallback_OnDataAvailable,
    ProxyBindStatusCallback_OnObjectAvailable
};

static HRESULT WINAPI AsyncBindStatusCallback_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    ProxyBindStatusCallback *This = (ProxyBindStatusCallback *)iface;
    HRESULT hr = IBindStatusCallback_GetBindInfo(This->pBSC, grfBINDF, pbindinfo);
    *grfBINDF |= BINDF_PULLDATA | BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    return hr;
}

static const IBindStatusCallbackVtbl AsyncBindStatusCallbackVtbl =
{
    ProxyBindStatusCallback_QueryInterface,
    ProxyBindStatusCallback_AddRef,
    ProxyBindStatusCallback_Release,
    ProxyBindStatusCallback_OnStartBinding,
    ProxyBindStatusCallback_GetPriority,
    ProxyBindStatusCallback_OnLowResource,
    ProxyBindStatusCallback_OnProgress,
    ProxyBindStatusCallback_OnStopBinding,
    AsyncBindStatusCallback_GetBindInfo,
    ProxyBindStatusCallback_OnDataAvailable,
    ProxyBindStatusCallback_OnObjectAvailable
};

static HRESULT URLStartDownload(LPCWSTR szURL, LPSTREAM *ppStream, IBindStatusCallback *pBSC)
{
    HRESULT hr;
    IMoniker *pMoniker;
    IBindCtx *pbc;

    *ppStream = NULL;

    hr = CreateURLMoniker(NULL, szURL, &pMoniker);
    if (FAILED(hr))
        return hr;

    hr = CreateBindCtx(0, &pbc);
    if (FAILED(hr))
    {
        IMoniker_Release(pMoniker);
        return hr;
    }

    hr = RegisterBindStatusCallback(pbc, pBSC, NULL, 0);
    if (FAILED(hr))
    {
        IBindCtx_Release(pbc);
        IMoniker_Release(pMoniker);
        return hr;
    }

    hr = IMoniker_BindToStorage(pMoniker, pbc, NULL, &IID_IStream, (void **)ppStream);

    /* BindToStorage returning E_PENDING because it's asynchronous is not an error */
    if (hr == E_PENDING) hr = S_OK;

    IBindCtx_Release(pbc);
    IMoniker_Release(pMoniker);

    return hr;
}

/***********************************************************************
 *		URLOpenBlockingStreamA (URLMON.@)
 */
HRESULT WINAPI URLOpenBlockingStreamA(LPUNKNOWN pCaller, LPCSTR szURL,
                                      LPSTREAM *ppStream, DWORD dwReserved,
                                      LPBINDSTATUSCALLBACK lpfnCB)
{
    LPWSTR szURLW;
    int len;
    HRESULT hr;

    TRACE("(%p, %s, %p, 0x%x, %p)\n", pCaller, szURL, ppStream, dwReserved, lpfnCB);

    if (!szURL || !ppStream)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
    szURLW = heap_alloc(len * sizeof(WCHAR));
    if (!szURLW)
    {
        *ppStream = NULL;
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar(CP_ACP, 0, szURL, -1, szURLW, len);

    hr = URLOpenBlockingStreamW(pCaller, szURLW, ppStream, dwReserved, lpfnCB);

    heap_free(szURLW);

    return hr;
}

/***********************************************************************
 *		URLOpenBlockingStreamW (URLMON.@)
 */
HRESULT WINAPI URLOpenBlockingStreamW(LPUNKNOWN pCaller, LPCWSTR szURL,
                                      LPSTREAM *ppStream, DWORD dwReserved,
                                      LPBINDSTATUSCALLBACK lpfnCB)
{
    ProxyBindStatusCallback blocking_bsc;

    TRACE("(%p, %s, %p, 0x%x, %p)\n", pCaller, debugstr_w(szURL), ppStream,
          dwReserved, lpfnCB);

    if (!szURL || !ppStream)
        return E_INVALIDARG;

    blocking_bsc.lpVtbl = &BlockingBindStatusCallbackVtbl;
    blocking_bsc.pBSC = lpfnCB;

    return URLStartDownload(szURL, ppStream, (IBindStatusCallback *)&blocking_bsc);
}

/***********************************************************************
 *		URLOpenStreamA (URLMON.@)
 */
HRESULT WINAPI URLOpenStreamA(LPUNKNOWN pCaller, LPCSTR szURL, DWORD dwReserved,
                              LPBINDSTATUSCALLBACK lpfnCB)
{
    LPWSTR szURLW;
    int len;
    HRESULT hr;

    TRACE("(%p, %s, 0x%x, %p)\n", pCaller, szURL, dwReserved, lpfnCB);

    if (!szURL)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
    szURLW = heap_alloc(len * sizeof(WCHAR));
    if (!szURLW)
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, szURL, -1, szURLW, len);

    hr = URLOpenStreamW(pCaller, szURLW, dwReserved, lpfnCB);

    heap_free(szURLW);

    return hr;
}

/***********************************************************************
 *		URLOpenStreamW (URLMON.@)
 */
HRESULT WINAPI URLOpenStreamW(LPUNKNOWN pCaller, LPCWSTR szURL, DWORD dwReserved,
                              LPBINDSTATUSCALLBACK lpfnCB)
{
    HRESULT hr;
    ProxyBindStatusCallback async_bsc;
    IStream *pStream;

    TRACE("(%p, %s, 0x%x, %p)\n", pCaller, debugstr_w(szURL), dwReserved,
          lpfnCB);

    if (!szURL)
        return E_INVALIDARG;

    async_bsc.lpVtbl = &AsyncBindStatusCallbackVtbl;
    async_bsc.pBSC = lpfnCB;

    hr = URLStartDownload(szURL, &pStream, (IBindStatusCallback *)&async_bsc);
    if (SUCCEEDED(hr) && pStream)
        IStream_Release(pStream);

    return hr;
}
