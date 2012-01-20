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

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

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

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        if (This->parentDevice)
            IDirect3DDevice9Ex_AddRef(This->parentDevice);

        wined3d_mutex_lock();
        wined3d_swapchain_incref(This->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DSwapChain9Impl_Release(LPDIRECT3DSWAPCHAIN9 iface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        wined3d_swapchain_decref(This->wined3d_swapchain);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        if (parentDevice) IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DSwapChain9 parts follow: */
static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DSwapChain9Impl_Present(LPDIRECT3DSWAPCHAIN9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p, flags %#x.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(This->wined3d_swapchain, pSourceRect,
            pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetFrontBufferData(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DSurface9* pDestSurface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, pDestSurface);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_front_buffer_data(This->wined3d_swapchain,
            ((IDirect3DSurface9Impl *)pDestSurface)->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetBackBuffer(IDirect3DSwapChain9 *iface,
        UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    struct wined3d_surface *wined3d_surface = NULL;
    HRESULT hr;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, iBackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_back_buffer(This->wined3d_swapchain,
            iBackBuffer, (WINED3DBACKBUFFER_TYPE)Type, &wined3d_surface);
    if (SUCCEEDED(hr) && wined3d_surface)
    {
       *ppBackBuffer = wined3d_surface_get_parent(wined3d_surface);
       IDirect3DSurface9_AddRef(*ppBackBuffer);
       wined3d_surface_decref(wined3d_surface);
    }
    wined3d_mutex_unlock();

    /* Do not touch the **ppBackBuffer pointer otherwise! (see device test) */
    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetRasterStatus(LPDIRECT3DSWAPCHAIN9 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, pRasterStatus);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_raster_status(This->wined3d_swapchain, (WINED3DRASTER_STATUS *)pRasterStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDisplayMode(LPDIRECT3DSWAPCHAIN9 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, pMode);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_display_mode(This->wined3d_swapchain, (WINED3DDISPLAYMODE *)pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDevice(IDirect3DSwapChain9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetPresentParameters(LPDIRECT3DSWAPCHAIN9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    WINED3DPRESENT_PARAMETERS winePresentParameters;
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pPresentationParameters);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_present_parameters(This->wined3d_swapchain, &winePresentParameters);
    wined3d_mutex_unlock();

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

static void STDMETHODCALLTYPE d3d9_swapchain_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_swapchain_wined3d_parent_ops =
{
    d3d9_swapchain_wined3d_object_released,
};

HRESULT swapchain_init(IDirect3DSwapChain9Impl *swapchain, IDirect3DDevice9Impl *device,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    WINED3DPRESENT_PARAMETERS wined3d_parameters;
    HRESULT hr;

    swapchain->ref = 1;
    swapchain->lpVtbl = &Direct3DSwapChain9_Vtbl;

    wined3d_parameters.BackBufferWidth = present_parameters->BackBufferWidth;
    wined3d_parameters.BackBufferHeight = present_parameters->BackBufferHeight;
    wined3d_parameters.BackBufferFormat = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    wined3d_parameters.BackBufferCount = max(1, present_parameters->BackBufferCount);
    wined3d_parameters.MultiSampleType = present_parameters->MultiSampleType;
    wined3d_parameters.MultiSampleQuality = present_parameters->MultiSampleQuality;
    wined3d_parameters.SwapEffect = present_parameters->SwapEffect;
    wined3d_parameters.hDeviceWindow = present_parameters->hDeviceWindow;
    wined3d_parameters.Windowed = present_parameters->Windowed;
    wined3d_parameters.EnableAutoDepthStencil = present_parameters->EnableAutoDepthStencil;
    wined3d_parameters.AutoDepthStencilFormat = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    wined3d_parameters.Flags = present_parameters->Flags;
    wined3d_parameters.FullScreen_RefreshRateInHz = present_parameters->FullScreen_RefreshRateInHz;
    wined3d_parameters.PresentationInterval = present_parameters->PresentationInterval;
    wined3d_parameters.AutoRestoreDisplayMode = TRUE;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, &wined3d_parameters,
            SURFACE_OPENGL, swapchain, &d3d9_swapchain_wined3d_parent_ops,
            &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    present_parameters->BackBufferWidth = wined3d_parameters.BackBufferWidth;
    present_parameters->BackBufferHeight = wined3d_parameters.BackBufferHeight;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(wined3d_parameters.BackBufferFormat);
    present_parameters->BackBufferCount = wined3d_parameters.BackBufferCount;
    present_parameters->MultiSampleType = wined3d_parameters.MultiSampleType;
    present_parameters->MultiSampleQuality = wined3d_parameters.MultiSampleQuality;
    present_parameters->SwapEffect = wined3d_parameters.SwapEffect;
    present_parameters->hDeviceWindow = wined3d_parameters.hDeviceWindow;
    present_parameters->Windowed = wined3d_parameters.Windowed;
    present_parameters->EnableAutoDepthStencil = wined3d_parameters.EnableAutoDepthStencil;
    present_parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(wined3d_parameters.AutoDepthStencilFormat);
    present_parameters->Flags = wined3d_parameters.Flags;
    present_parameters->FullScreen_RefreshRateInHz = wined3d_parameters.FullScreen_RefreshRateInHz;
    present_parameters->PresentationInterval = wined3d_parameters.PresentationInterval;

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(swapchain->parentDevice);

    return D3D_OK;
}
