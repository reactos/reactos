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

#include "d3d9_private.h"

static inline struct d3d9_swapchain *impl_from_IDirect3DSwapChain9Ex(IDirect3DSwapChain9Ex *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_swapchain, IDirect3DSwapChain9Ex_iface);
}

static HRESULT WINAPI d3d9_swapchain_QueryInterface(IDirect3DSwapChain9Ex *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DSwapChain9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DSwapChain9Ex_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirect3DSwapChain9Ex))
    {
        struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
        struct d3d9_device *device       = impl_from_IDirect3DDevice9Ex(swapchain->parent_device);

        /* Find out if the creating d3d9 interface was created with Direct3DCreate9Ex.
         * It doesn't matter with which function the device was created. */
        if (!device->d3d_parent->extended)
        {
            WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE.\n");
            *out = NULL;
            return E_NOINTERFACE;
        }

        IDirect3DSwapChain9Ex_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_swapchain_AddRef(IDirect3DSwapChain9Ex *iface)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    ULONG refcount = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        if (swapchain->parent_device)
            IDirect3DDevice9Ex_AddRef(swapchain->parent_device);

        wined3d_mutex_lock();
        wined3d_swapchain_incref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d9_swapchain_Release(IDirect3DSwapChain9Ex *iface)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    ULONG refcount = InterlockedDecrement(&swapchain->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *parent_device = swapchain->parent_device;

        wined3d_mutex_lock();
        wined3d_swapchain_decref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        if (parent_device)
            IDirect3DDevice9Ex_Release(parent_device);
    }

    return refcount;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_swapchain_Present(IDirect3DSwapChain9Ex *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct d3d9_device *device = impl_from_IDirect3DDevice9Ex(swapchain->parent_device);
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#x.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    if (device->device_state != D3D9_DEVICE_STATE_OK)
        return device->d3d_parent->extended ? S_PRESENT_OCCLUDED : D3DERR_DEVICELOST;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(swapchain->wined3d_swapchain, src_rect,
            dst_rect, dst_window_override, dirty_region, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_swapchain_GetFrontBufferData(IDirect3DSwapChain9Ex *iface, IDirect3DSurface9 *surface)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct d3d9_surface *dst = unsafe_impl_from_IDirect3DSurface9(surface);
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_front_buffer_data(swapchain->wined3d_swapchain, dst->wined3d_texture, dst->sub_resource_idx);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_swapchain_GetBackBuffer(IDirect3DSwapChain9Ex *iface,
        UINT backbuffer_idx, D3DBACKBUFFER_TYPE backbuffer_type, IDirect3DSurface9 **backbuffer)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_texture *wined3d_texture;
    struct d3d9_surface *surface_impl;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, backbuffer_idx, backbuffer_type, backbuffer);

    /* backbuffer_type is ignored by native. */

    if (!backbuffer)
    {
        WARN("The output pointer is NULL, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if ((wined3d_texture = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain, backbuffer_idx)))
    {
        wined3d_resource = wined3d_texture_get_sub_resource(wined3d_texture, 0);
        surface_impl = wined3d_resource_get_parent(wined3d_resource);
        *backbuffer = &surface_impl->IDirect3DSurface9_iface;
        IDirect3DSurface9_AddRef(*backbuffer);
    }
    else
    {
        /* Do not set *backbuffer = NULL, see tests/device.c, test_swapchain(). */
        hr = D3DERR_INVALIDCALL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_swapchain_GetRasterStatus(IDirect3DSwapChain9Ex *iface, D3DRASTER_STATUS *raster_status)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, raster_status);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_raster_status(swapchain->wined3d_swapchain,
            (struct wined3d_raster_status *)raster_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_swapchain_GetDisplayMode(IDirect3DSwapChain9Ex *iface, D3DDISPLAYMODE *mode)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, mode);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_display_mode(swapchain->wined3d_swapchain, &wined3d_mode, NULL);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
    }

    return hr;
}

static HRESULT WINAPI d3d9_swapchain_GetDevice(IDirect3DSwapChain9Ex *iface, IDirect3DDevice9 **device)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)swapchain->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_swapchain_GetPresentParameters(IDirect3DSwapChain9Ex *iface,
        D3DPRESENT_PARAMETERS *parameters)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct wined3d_swapchain_desc desc;

    TRACE("iface %p, parameters %p.\n", iface, parameters);

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &desc);
    wined3d_mutex_unlock();
    present_parameters_from_wined3d_swapchain_desc(parameters, &desc);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_swapchain_GetLastPresentCount(IDirect3DSwapChain9Ex *iface,
        UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p, stub!\n", iface, last_present_count);

    if (last_present_count)
        *last_present_count = 0;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_swapchain_GetPresentStatistics(IDirect3DSwapChain9Ex *iface,
        D3DPRESENTSTATS *present_stats)
{
    FIXME("iface %p, present_stats %p, stub!\n", iface, present_stats);

    if (present_stats)
        memset(present_stats, 0, sizeof(*present_stats));

    return D3D_OK;
}

static HRESULT WINAPI d3d9_swapchain_GetDisplayModeEx(IDirect3DSwapChain9Ex *iface,
        D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation)
{
    struct d3d9_swapchain *swapchain = impl_from_IDirect3DSwapChain9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, mode %p, rotation %p.\n", iface, mode, rotation);

    if (mode->Size != sizeof(*mode))
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_display_mode(swapchain->wined3d_swapchain, &wined3d_mode,
            (enum wined3d_display_rotation *)rotation);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
        mode->ScanLineOrdering = wined3d_mode.scanline_ordering;
    }

    return hr;
}

static const struct IDirect3DSwapChain9ExVtbl d3d9_swapchain_vtbl =
{
    /* IUnknown */
    d3d9_swapchain_QueryInterface,
    d3d9_swapchain_AddRef,
    d3d9_swapchain_Release,
    /* IDirect3DSwapChain9 */
    d3d9_swapchain_Present,
    d3d9_swapchain_GetFrontBufferData,
    d3d9_swapchain_GetBackBuffer,
    d3d9_swapchain_GetRasterStatus,
    d3d9_swapchain_GetDisplayMode,
    d3d9_swapchain_GetDevice,
    d3d9_swapchain_GetPresentParameters,
    /* IDirect3DSwapChain9Ex */
    d3d9_swapchain_GetLastPresentCount,
    d3d9_swapchain_GetPresentStatistics,
    d3d9_swapchain_GetDisplayModeEx
};

static void STDMETHODCALLTYPE d3d9_swapchain_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_swapchain_wined3d_parent_ops =
{
    d3d9_swapchain_wined3d_object_released,
};

static HRESULT swapchain_init(struct d3d9_swapchain *swapchain, struct d3d9_device *device,
        struct wined3d_swapchain_desc *desc)
{
    HRESULT hr;

    swapchain->refcount = 1;
    swapchain->IDirect3DSwapChain9Ex_iface.lpVtbl = &d3d9_swapchain_vtbl;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, desc, swapchain,
            &d3d9_swapchain_wined3d_parent_ops, &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(swapchain->parent_device);

    return D3D_OK;
}

HRESULT d3d9_swapchain_create(struct d3d9_device *device, struct wined3d_swapchain_desc *desc,
        struct d3d9_swapchain **swapchain)
{
    struct d3d9_swapchain *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = swapchain_init(object, device, desc)))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = object;

    return D3D_OK;
}
