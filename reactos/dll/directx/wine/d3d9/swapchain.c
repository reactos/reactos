/*
 * IDirect3DSwapChain9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DSwapChain IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSwapChain9Impl_QueryInterface(LPDIRECT3DSWAPCHAIN9 iface, REFIID riid, LPVOID* ppobj)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSwapChain9)) {
        IDirect3DSwapChain9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSwapChain9Impl_AddRef(LPDIRECT3DSWAPCHAIN9 iface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    if(ref == 1 && This->parentDevice) IDirect3DDevice9Ex_AddRef(This->parentDevice);

    return ref;
}

static ULONG WINAPI IDirect3DSwapChain9Impl_Release(LPDIRECT3DSWAPCHAIN9 iface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        if (This->parentDevice) IDirect3DDevice9Ex_Release(This->parentDevice);
        if (!This->isImplicit) {
            EnterCriticalSection(&d3d9_cs);
            IWineD3DSwapChain_Destroy(This->wineD3DSwapChain, D3D9CB_DestroyRenderTarget);
            LeaveCriticalSection(&d3d9_cs);
            HeapFree(GetProcessHeap(), 0, This);
        }
    }
    return ref;
}

/* IDirect3DSwapChain9 parts follow: */
static HRESULT WINAPI IDirect3DSwapChain9Impl_Present(LPDIRECT3DSWAPCHAIN9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSwapChain_Present(This->wineD3DSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetFrontBufferData(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DSurface9* pDestSurface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSwapChain_GetFrontBufferData(This->wineD3DSwapChain,  ((IDirect3DSurface9Impl *)pDestSurface)->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetBackBuffer(LPDIRECT3DSWAPCHAIN9 iface, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DSwapChain_GetBackBuffer(This->wineD3DSwapChain, iBackBuffer, (WINED3DBACKBUFFER_TYPE) Type, &mySurface);
    if (hrc == D3D_OK && NULL != mySurface) {
       IWineD3DSurface_GetParent(mySurface, (IUnknown **)ppBackBuffer);
       IWineD3DSurface_Release(mySurface);
    }
    LeaveCriticalSection(&d3d9_cs);
    /* Do not touch the **ppBackBuffer pointer otherwise! (see device test) */
    return hrc;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetRasterStatus(LPDIRECT3DSWAPCHAIN9 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSwapChain_GetRasterStatus(This->wineD3DSwapChain, (WINED3DRASTER_STATUS *) pRasterStatus);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDisplayMode(LPDIRECT3DSWAPCHAIN9 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSwapChain_GetDisplayMode(This->wineD3DSwapChain, (WINED3DDISPLAYMODE *) pMode);
    LeaveCriticalSection(&d3d9_cs);

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDevice(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DDevice *device = NULL;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DSwapChain_GetDevice(This->wineD3DSwapChain, &device);
    if (hrc == D3D_OK && NULL != device) {
       IWineD3DDevice_GetParent(device, (IUnknown **)ppDevice);
       IWineD3DDevice_Release(device);
    }
    LeaveCriticalSection(&d3d9_cs);
    return hrc;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetPresentParameters(LPDIRECT3DSWAPCHAIN9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    WINED3DPRESENT_PARAMETERS winePresentParameters;
    HRESULT hr;

    TRACE("(%p)->(%p): Relay\n", This, pPresentationParameters);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSwapChain_GetPresentParameters(This->wineD3DSwapChain, &winePresentParameters);
    LeaveCriticalSection(&d3d9_cs);

    pPresentationParameters->BackBufferWidth            = winePresentParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight           = winePresentParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat           = d3dformat_from_wined3dformat(winePresentParameters.BackBufferFormat);
    pPresentationParameters->BackBufferCount            = winePresentParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType            = winePresentParameters.MultiSampleType;
    pPresentationParameters->MultiSampleQuality         = winePresentParameters.MultiSampleQuality;
    pPresentationParameters->SwapEffect                 = winePresentParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow              = winePresentParameters.hDeviceWindow;
    pPresentationParameters->Windowed                   = winePresentParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil     = winePresentParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat     = d3dformat_from_wined3dformat(winePresentParameters.AutoDepthStencilFormat);
    pPresentationParameters->Flags                      = winePresentParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz = winePresentParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->PresentationInterval       = winePresentParameters.PresentationInterval;

    return hr;
}


static const IDirect3DSwapChain9Vtbl Direct3DSwapChain9_Vtbl =
{
    IDirect3DSwapChain9Impl_QueryInterface,
    IDirect3DSwapChain9Impl_AddRef,
    IDirect3DSwapChain9Impl_Release,
    IDirect3DSwapChain9Impl_Present,
    IDirect3DSwapChain9Impl_GetFrontBufferData,
    IDirect3DSwapChain9Impl_GetBackBuffer,
    IDirect3DSwapChain9Impl_GetRasterStatus,
    IDirect3DSwapChain9Impl_GetDisplayMode,
    IDirect3DSwapChain9Impl_GetDevice,
    IDirect3DSwapChain9Impl_GetPresentParameters
};


/* IDirect3DDevice9 IDirect3DSwapChain9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9EX iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSwapChain9Impl* object;
    HRESULT hrc = D3D_OK;
    WINED3DPRESENT_PARAMETERS localParameters;

    TRACE("(%p) Relay\n", This);

    object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->ref = 1;
    object->lpVtbl = &Direct3DSwapChain9_Vtbl;

    /* The back buffer count is set to one if it's 0 */
    if(pPresentationParameters->BackBufferCount == 0) {
        pPresentationParameters->BackBufferCount = 1;
    }

    /* Allocate an associated WineD3DDevice object */
    localParameters.BackBufferWidth                     = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                    = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                    = wined3dformat_from_d3dformat(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                     = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                     = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                  = pPresentationParameters->MultiSampleQuality;
    localParameters.SwapEffect                          = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                       = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                            = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil              = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat              = wined3dformat_from_d3dformat(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                               = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz          = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                = pPresentationParameters->PresentationInterval;
    localParameters.AutoRestoreDisplayMode              = TRUE;

    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_CreateSwapChain(This->WineD3DDevice, &localParameters,
            &object->wineD3DSwapChain, (IUnknown*)object, SURFACE_OPENGL);
    LeaveCriticalSection(&d3d9_cs);

    pPresentationParameters->BackBufferWidth            = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight           = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat           = d3dformat_from_wined3dformat(localParameters.BackBufferFormat);
    pPresentationParameters->BackBufferCount            = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType            = localParameters.MultiSampleType;
    pPresentationParameters->MultiSampleQuality         = localParameters.MultiSampleQuality;
    pPresentationParameters->SwapEffect                 = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow              = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                   = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil     = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat     = d3dformat_from_wined3dformat(localParameters.AutoDepthStencilFormat);
    pPresentationParameters->Flags                      = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->PresentationInterval       = localParameters.PresentationInterval;

    if (hrc != D3D_OK) {
        FIXME("(%p) call to IWineD3DDevice_CreateSwapChain failed\n", This);
        HeapFree(GetProcessHeap(), 0 , object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *pSwapChain = (IDirect3DSwapChain9 *)object;
        TRACE("(%p) : Created swapchain %p\n", This, *pSwapChain);
    }
    TRACE("(%p) returning %p\n", This, *pSwapChain);
    return hrc;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetSwapChain(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSwapChain *swapchain = NULL;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_GetSwapChain(This->WineD3DDevice, iSwapChain, &swapchain);
    if (hrc == D3D_OK && NULL != swapchain) {
       IWineD3DSwapChain_GetParent(swapchain, (IUnknown **)pSwapChain);
       IWineD3DSwapChain_Release(swapchain);
    } else {
        *pSwapChain = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);
    return hrc;
}

UINT     WINAPI  IDirect3DDevice9Impl_GetNumberOfSwapChains(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    UINT ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DDevice_GetNumberOfSwapChains(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}
