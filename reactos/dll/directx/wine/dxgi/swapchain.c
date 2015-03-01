/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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
 *
 */

#include "dxgi_private.h"

static inline struct dxgi_swapchain *impl_from_IDXGISwapChain(IDXGISwapChain *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_swapchain, IDXGISwapChain_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_QueryInterface(IDXGISwapChain *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGISwapChain))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_AddRef(IDXGISwapChain *iface)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    if (refcount == 1)
        wined3d_swapchain_incref(This->wined3d_swapchain);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_Release(IDXGISwapChain *iface)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
        wined3d_swapchain_decref(This->wined3d_swapchain);

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateDataInterface(IDXGISwapChain *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetParent(IDXGISwapChain *iface, REFIID riid, void **parent)
{
    FIXME("iface %p, riid %s, parent %p stub!\n", iface, debugstr_guid(riid), parent);

    return E_NOTIMPL;
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDevice(IDXGISwapChain *iface, REFIID riid, void **device)
{
    FIXME("iface %p, riid %s, device %p stub!\n", iface, debugstr_guid(riid), device);

    return E_NOTIMPL;
}

/* IDXGISwapChain methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_Present(IDXGISwapChain *iface, UINT sync_interval, UINT flags)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, sync_interval %u, flags %#x\n", iface, sync_interval, flags);

    if (sync_interval) FIXME("Unimplemented sync interval %u\n", sync_interval);
    if (flags) FIXME("Unimplemented flags %#x\n", flags);

    return wined3d_swapchain_present(This->wined3d_swapchain, NULL, NULL, NULL, NULL, 0);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetBuffer(IDXGISwapChain *iface,
        UINT buffer_idx, REFIID riid, void **surface)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    struct wined3d_surface *backbuffer;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, buffer_idx %u, riid %s, surface %p\n",
            iface, buffer_idx, debugstr_guid(riid), surface);

    EnterCriticalSection(&dxgi_cs);

    if (!(backbuffer = wined3d_swapchain_get_back_buffer(This->wined3d_swapchain,
            buffer_idx, WINED3D_BACKBUFFER_TYPE_MONO)))
    {
        LeaveCriticalSection(&dxgi_cs);
        return DXGI_ERROR_INVALID_CALL;
    }

    parent = wined3d_surface_get_parent(backbuffer);
    hr = IUnknown_QueryInterface(parent, riid, surface);
    LeaveCriticalSection(&dxgi_cs);

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetFullscreenState(IDXGISwapChain *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    FIXME("iface %p, fullscreen %u, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetFullscreenState(IDXGISwapChain *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    FIXME("iface %p, fullscreen %p, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDesc(IDXGISwapChain *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    FIXME("iface %p, desc %p partial stub!\n", iface, desc);

    if (desc == NULL)
        return E_INVALIDARG;

    EnterCriticalSection(&dxgi_cs);
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    LeaveCriticalSection(&dxgi_cs);

    FIXME("Ignoring ScanlineOrdering, Scaling, SwapEffect and Flags\n");

    desc->BufferDesc.Width = wined3d_desc.backbuffer_width;
    desc->BufferDesc.Height = wined3d_desc.backbuffer_height;
    desc->BufferDesc.RefreshRate.Numerator = wined3d_desc.refresh_rate;
    desc->BufferDesc.RefreshRate.Denominator = 1;
    desc->BufferDesc.Format = dxgi_format_from_wined3dformat(wined3d_desc.backbuffer_format);
    desc->BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc->BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc->SampleDesc.Count = wined3d_desc.multisample_type;
    desc->SampleDesc.Quality = wined3d_desc.multisample_quality;
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->OutputWindow = wined3d_desc.device_window;
    desc->Windowed = wined3d_desc.windowed;
    desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc->Flags = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeBuffers(IDXGISwapChain *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    FIXME("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x stub!\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeTarget(IDXGISwapChain *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    FIXME("iface %p, target_mode_desc %p stub!\n", iface, target_mode_desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetContainingOutput(IDXGISwapChain *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetFrameStatistics(IDXGISwapChain *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetLastPresentCount(IDXGISwapChain *iface, UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

static const struct IDXGISwapChainVtbl dxgi_swapchain_vtbl =
{
    /* IUnknown methods */
    dxgi_swapchain_QueryInterface,
    dxgi_swapchain_AddRef,
    dxgi_swapchain_Release,
    /* IDXGIObject methods */
    dxgi_swapchain_SetPrivateData,
    dxgi_swapchain_SetPrivateDataInterface,
    dxgi_swapchain_GetPrivateData,
    dxgi_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    dxgi_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    dxgi_swapchain_Present,
    dxgi_swapchain_GetBuffer,
    dxgi_swapchain_SetFullscreenState,
    dxgi_swapchain_GetFullscreenState,
    dxgi_swapchain_GetDesc,
    dxgi_swapchain_ResizeBuffers,
    dxgi_swapchain_ResizeTarget,
    dxgi_swapchain_GetContainingOutput,
    dxgi_swapchain_GetFrameStatistics,
    dxgi_swapchain_GetLastPresentCount,
};

static void STDMETHODCALLTYPE dxgi_swapchain_wined3d_object_released(void *parent)
{
    struct dxgi_swapchain *swapchain = parent;

    wined3d_private_store_cleanup(&swapchain->private_store);
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops dxgi_swapchain_wined3d_parent_ops =
{
    dxgi_swapchain_wined3d_object_released,
};

HRESULT dxgi_swapchain_init(struct dxgi_swapchain *swapchain, struct dxgi_device *device,
        struct wined3d_swapchain_desc *desc)
{
    HRESULT hr;

    swapchain->IDXGISwapChain_iface.lpVtbl = &dxgi_swapchain_vtbl;
    swapchain->refcount = 1;
    wined3d_private_store_init(&swapchain->private_store);

    if (FAILED(hr = wined3d_swapchain_create(device->wined3d_device, desc, swapchain,
            &dxgi_swapchain_wined3d_parent_ops, &swapchain->wined3d_swapchain)))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&swapchain->private_store);
        return hr;
    }

    return S_OK;
}
