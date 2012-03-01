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

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetFrontBufferData(IDirect3DSwapChain9 *iface,
        IDirect3DSurface9 *pDestSurface)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    IDirect3DSurface9Impl *dst = unsafe_impl_from_IDirect3DSurface9(pDestSurface);
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, pDestSurface);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_front_buffer_data(This->wined3d_swapchain, dst->wined3d_surface);
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
            iBackBuffer, (enum wined3d_backbuffer_type)Type, &wined3d_surface);
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
    hr = wined3d_swapchain_get_raster_status(This->wined3d_swapchain, (struct wined3d_raster_status *)pRasterStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDisplayMode(LPDIRECT3DSWAPCHAIN9 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, pMode);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_display_mode(This->wined3d_swapchain, (struct wined3d_display_mode *)pMode);
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

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetPresentParameters(IDirect3DSwapChain9 *iface,
        D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    struct wined3d_swapchain_desc desc;
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pPresentationParameters);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_desc(This->wined3d_swapchain, &desc);
    wined3d_mutex_unlock();

    pPresentationParameters->BackBufferWidth = desc.backbuffer_width;
    pPresentationParameters->BackBufferHeight = desc.backbuffer_height;
    pPresentationParameters->BackBufferFormat = d3dformat_from_wined3dformat(desc.backbuffer_format);
    pPresentationParameters->BackBufferCount = desc.backbuffer_count;
    pPresentationParameters->MultiSampleType = desc.multisample_type;
    pPresentationParameters->MultiSampleQuality = desc.multisample_quality;
    pPresentationParameters->SwapEffect = desc.swap_effect;
    pPresentationParameters->hDeviceWindow = desc.device_window;
    pPresentationParameters->Windowed = desc.windowed;
    pPresentationParameters->EnableAutoDepthStencil = desc.enable_auto_depth_stencil;
    pPresentationParameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc.auto_depth_stencil_format);
    pPresentationParameters->Flags = desc.flags;
    pPresentationParameters->FullScreen_RefreshRateInHz = desc.refresh_rate;
    pPresentationParameters->PresentationInterval = desc.swap_interval;

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
    struct wined3d_swapchain_desc desc;
    HRESULT hr;

    swapchain->ref = 1;
    swapchain->lpVtbl = &Direct3DSwapChain9_Vtbl;

    desc.backbuffer_width = present_parameters->BackBufferWidth;
    desc.backbuffer_height = present_parameters->BackBufferHeight;
    desc.backbuffer_format = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    desc.backbuffer_count = max(1, present_parameters->BackBufferCount);
    desc.multisample_type = present_parameters->MultiSampleType;
    desc.multisample_quality = present_parameters->MultiSampleQuality;
    desc.swap_effect = present_parameters->SwapEffect;
    desc.device_window = present_parameters->hDeviceWindow;
    desc.windowed = present_parameters->Windowed;
    desc.enable_auto_depth_stencil = present_parameters->EnableAutoDepthStencil;
    desc.auto_depth_stencil_format = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    desc.flags = present_parameters->Flags;
    desc.refresh_rate = present_parameters->FullScreen_RefreshRateInHz;
    desc.swap_interval = present_parameters->PresentationInterval;
    desc.auto_restore_display_mode = TRUE;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, &desc,
            SURFACE_OPENGL, swapchain, &d3d9_swapchain_wined3d_parent_ops,
            &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    present_parameters->BackBufferWidth = desc.backbuffer_width;
    present_parameters->BackBufferHeight = desc.backbuffer_height;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(desc.backbuffer_format);
    present_parameters->BackBufferCount = desc.backbuffer_count;
    present_parameters->MultiSampleType = desc.multisample_type;
    present_parameters->MultiSampleQuality = desc.multisample_quality;
    present_parameters->SwapEffect = desc.swap_effect;
    present_parameters->hDeviceWindow = desc.device_window;
    present_parameters->Windowed = desc.windowed;
    present_parameters->EnableAutoDepthStencil = desc.enable_auto_depth_stencil;
    present_parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc.auto_depth_stencil_format);
    present_parameters->Flags = desc.flags;
    present_parameters->FullScreen_RefreshRateInHz = desc.refresh_rate;
    present_parameters->PresentationInterval = desc.swap_interval;

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(swapchain->parentDevice);

    return D3D_OK;
}
