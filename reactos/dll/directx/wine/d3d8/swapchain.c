/*
 * IDirect3DSwapChain8 implementation
 *
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline IDirect3DSwapChain8Impl *impl_from_IDirect3DSwapChain8(IDirect3DSwapChain8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DSwapChain8Impl, IDirect3DSwapChain8_iface);
}

static HRESULT WINAPI IDirect3DSwapChain8Impl_QueryInterface(IDirect3DSwapChain8 *iface,
        REFIID riid, void **ppobj)
{
    IDirect3DSwapChain8Impl *This = impl_from_IDirect3DSwapChain8(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSwapChain8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSwapChain8Impl_AddRef(IDirect3DSwapChain8 *iface)
{
    IDirect3DSwapChain8Impl *This = impl_from_IDirect3DSwapChain8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        if (This->parentDevice)
            IDirect3DDevice8_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        wined3d_swapchain_incref(This->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DSwapChain8Impl_Release(IDirect3DSwapChain8 *iface)
{
    IDirect3DSwapChain8Impl *This = impl_from_IDirect3DSwapChain8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice8 *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        wined3d_swapchain_decref(This->wined3d_swapchain);
        wined3d_mutex_unlock();

        if (parentDevice)
            IDirect3DDevice8_Release(parentDevice);
    }
    return ref;
}

static HRESULT WINAPI IDirect3DSwapChain8Impl_Present(IDirect3DSwapChain8 *iface,
        const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride,
        const RGNDATA *pDirtyRegion)
{
    IDirect3DSwapChain8Impl *This = impl_from_IDirect3DSwapChain8(iface);
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(This->wined3d_swapchain, pSourceRect,
            pDestRect, hDestWindowOverride, pDirtyRegion, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain8Impl_GetBackBuffer(IDirect3DSwapChain8 *iface,
        UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer)
{
    IDirect3DSwapChain8Impl *This = impl_from_IDirect3DSwapChain8(iface);
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
        IDirect3DSurface8_AddRef(*ppBackBuffer);
        wined3d_surface_decref(wined3d_surface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DSwapChain8Vtbl Direct3DSwapChain8_Vtbl =
{
    IDirect3DSwapChain8Impl_QueryInterface,
    IDirect3DSwapChain8Impl_AddRef,
    IDirect3DSwapChain8Impl_Release,
    IDirect3DSwapChain8Impl_Present,
    IDirect3DSwapChain8Impl_GetBackBuffer
};

static void STDMETHODCALLTYPE d3d8_swapchain_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_swapchain_wined3d_parent_ops =
{
    d3d8_swapchain_wined3d_object_released,
};

HRESULT swapchain_init(IDirect3DSwapChain8Impl *swapchain, IDirect3DDevice8Impl *device,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    struct wined3d_swapchain_desc desc;
    HRESULT hr;

    swapchain->ref = 1;
    swapchain->IDirect3DSwapChain8_iface.lpVtbl = &Direct3DSwapChain8_Vtbl;

    desc.backbuffer_width = present_parameters->BackBufferWidth;
    desc.backbuffer_height = present_parameters->BackBufferHeight;
    desc.backbuffer_format = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    desc.backbuffer_count = max(1, present_parameters->BackBufferCount);
    desc.multisample_type = present_parameters->MultiSampleType;
    desc.multisample_quality = 0; /* d3d9 only */
    desc.swap_effect = present_parameters->SwapEffect;
    desc.device_window = present_parameters->hDeviceWindow;
    desc.windowed = present_parameters->Windowed;
    desc.enable_auto_depth_stencil = present_parameters->EnableAutoDepthStencil;
    desc.auto_depth_stencil_format = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    desc.flags = present_parameters->Flags;
    desc.refresh_rate = present_parameters->FullScreen_RefreshRateInHz;
    desc.swap_interval = present_parameters->FullScreen_PresentationInterval;
    desc.auto_restore_display_mode = TRUE;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, &desc,
            SURFACE_OPENGL, swapchain, &d3d8_swapchain_wined3d_parent_ops,
            &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    present_parameters->BackBufferWidth = desc.backbuffer_width;
    present_parameters->BackBufferHeight = desc.backbuffer_height;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(desc.backbuffer_format);
    present_parameters->BackBufferCount = desc.backbuffer_count;
    present_parameters->MultiSampleType = desc.multisample_type;
    present_parameters->SwapEffect = desc.swap_effect;
    present_parameters->hDeviceWindow = desc.device_window;
    present_parameters->Windowed = desc.windowed;
    present_parameters->EnableAutoDepthStencil = desc.enable_auto_depth_stencil;
    present_parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc.auto_depth_stencil_format);
    present_parameters->Flags = desc.flags;
    present_parameters->FullScreen_RefreshRateInHz = desc.refresh_rate;
    present_parameters->FullScreen_PresentationInterval = desc.swap_interval;

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parentDevice = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(swapchain->parentDevice);

    return D3D_OK;
}
