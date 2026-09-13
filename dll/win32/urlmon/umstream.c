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

typedef struct ProxyBindStatusCallback
{
    IBindStatusCallback IBindStatusCallback_iface;

    IBindStatusCallback *pBSC;
} ProxyBindStatusCallback;

static inline ProxyBindStatusCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, ProxyBindStatusCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI ProxyBindStatusCallback_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&IID_IBindStatusCallback, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
        IBindStatusCallback_AddRef(iface);
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
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_OnStartBinding(This->pBSC, dwReserved, pib);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_GetPriority(This->pBSC, pnPriority);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_OnLowResource(This->pBSC, reserved);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
                                           ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_OnProgress(This->pBSC, ulProgress,
                                          ulProgressMax, ulStatusCode,
                                          szStatusText);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_OnStopBinding(This->pBSC, hresult, szError);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD size = pbindinfo->cbSize;
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_GetBindInfo(This->pBSC, grfBINDF, pbindinfo);

    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;

    *grfBINDF = 0;

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
                                                              DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    if(This->pBSC)
        return IBindStatusCallback_OnDataAvailable(This->pBSC, grfBSCF, dwSize,
                                               pformatetc, pstgmed);

    return S_OK;
}

static HRESULT WINAPI ProxyBindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

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
    ProxyBindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    HRESULT hr = S_OK;

    if(This->pBSC)
        hr = IBindStatusCallback_GetBindInfo(This->pBSC, grfBINDF, pbindinfo);
    else{
        DWORD size = pbindinfo->cbSize;
        memset(pbindinfo, 0, size);
        pbindinfo->cbSize = size;

        *grfBINDF = 0;
    }

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

    TRACE("(%p, %s, %p, 0x%lx, %p)\n", pCaller, szURL, ppStream, dwReserved, lpfnCB);

    if (!szURL || !ppStream)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
    szURLW = malloc(len * sizeof(WCHAR));
    if (!szURLW)
    {
        *ppStream = NULL;
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar(CP_ACP, 0, szURL, -1, szURLW, len);

    hr = URLOpenBlockingStreamW(pCaller, szURLW, ppStream, dwReserved, lpfnCB);

    free(szURLW);

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

    TRACE("(%p, %s, %p, 0x%lx, %p)\n", pCaller, debugstr_w(szURL), ppStream,
          dwReserved, lpfnCB);

    if (!szURL || !ppStream)
        return E_INVALIDARG;

    blocking_bsc.IBindStatusCallback_iface.lpVtbl = &BlockingBindStatusCallbackVtbl;
    blocking_bsc.pBSC = lpfnCB;

    return URLStartDownload(szURL, ppStream, &blocking_bsc.IBindStatusCallback_iface);
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

    TRACE("(%p, %s, 0x%lx, %p)\n", pCaller, szURL, dwReserved, lpfnCB);

    if (!szURL)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
    szURLW = malloc(len * sizeof(WCHAR));
    if (!szURLW)
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, szURL, -1, szURLW, len);

    hr = URLOpenStreamW(pCaller, szURLW, dwReserved, lpfnCB);

    free(szURLW);

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

    TRACE("(%p, %s, 0x%lx, %p)\n", pCaller, debugstr_w(szURL), dwReserved,
          lpfnCB);

    if (!szURL)
        return E_INVALIDARG;

    async_bsc.IBindStatusCallback_iface.lpVtbl = &AsyncBindStatusCallbackVtbl;
    async_bsc.pBSC = lpfnCB;

    hr = URLStartDownload(szURL, &pStream, &async_bsc.IBindStatusCallback_iface);
    if (SUCCEEDED(hr) && pStream)
        IStream_Release(pStream);

    return hr;
}

/***********************************************************************
 *		URLOpenPullStreamW (URLMON.@)
 */
HRESULT WINAPI URLOpenPullStreamW(IUnknown *caller, const WCHAR *url, DWORD reserved,
                                  IBindStatusCallback *callback)
{
    FIXME("%p %s %lu %p, stub!\n", caller, debugstr_w(url), reserved, callback);
    return E_NOTIMPL;
}
