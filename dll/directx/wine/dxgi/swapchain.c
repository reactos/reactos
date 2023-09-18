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

#include "config.h"
#include "wine/port.h"

#include "dxgi_private.h"

#ifdef SONAME_LIBVKD3D
#define VK_NO_PROTOTYPES
#define VKD3D_NO_PROTOTYPES
#define VKD3D_NO_VULKAN_H
#define VKD3D_NO_WIN32_TYPES
#ifndef USE_WIN32_VULKAN
#define WINE_VK_HOST
#endif
#include "wine/library.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include <vkd3d.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static DXGI_SWAP_EFFECT dxgi_swap_effect_from_wined3d(enum wined3d_swap_effect swap_effect)
{
    switch (swap_effect)
    {
        case WINED3D_SWAP_EFFECT_DISCARD:
            return DXGI_SWAP_EFFECT_DISCARD;
        case WINED3D_SWAP_EFFECT_SEQUENTIAL:
            return DXGI_SWAP_EFFECT_SEQUENTIAL;
        case WINED3D_SWAP_EFFECT_FLIP_DISCARD:
            return DXGI_SWAP_EFFECT_FLIP_DISCARD;
        case WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL:
            return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        default:
            FIXME("Invalid swap effect %#x.\n", swap_effect);
            return DXGI_SWAP_EFFECT_DISCARD;
    }
}

static BOOL dxgi_validate_flip_swap_effect_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return TRUE;
        default:
            WARN("Invalid swapchain format %#x for flip presentation model.\n", format);
            return FALSE;
    }
}

BOOL dxgi_validate_swapchain_desc(const DXGI_SWAP_CHAIN_DESC1 *desc)
{
    unsigned int min_buffer_count;

    switch (desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_DISCARD:
        case DXGI_SWAP_EFFECT_SEQUENTIAL:
            min_buffer_count = 1;
            break;

        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
            min_buffer_count = 2;

            if (desc->Format && !dxgi_validate_flip_swap_effect_format(desc->Format))
                return FALSE;

            if (desc->SampleDesc.Count != 1 || desc->SampleDesc.Quality)
            {
                WARN("Invalid sample desc %u, %u for swap effect %#x.\n",
                        desc->SampleDesc.Count, desc->SampleDesc.Quality, desc->SwapEffect);
                return FALSE;
            }
            break;

        default:
            WARN("Invalid swap effect %u used.\n", desc->SwapEffect);
            return FALSE;
    }

    if (desc->BufferCount < min_buffer_count || desc->BufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS)
    {
        WARN("BufferCount is %u.\n", desc->BufferCount);
        return FALSE;
    }

    return TRUE;
}

static HRESULT dxgi_get_output_from_window(IDXGIAdapter *adapter, HWND window, IDXGIOutput **dxgi_output)
{
    DXGI_OUTPUT_DESC desc;
    IDXGIOutput *output;
    unsigned int index;
    HMONITOR monitor;
    HRESULT hr;

    if (!(monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)))
    {
        WARN("Failed to get monitor from window.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    index = 0;
    while ((hr = IDXGIAdapter_EnumOutputs(adapter, index, &output)) == S_OK)
    {
        if (FAILED(hr = IDXGIOutput_GetDesc(output, &desc)))
        {
            WARN("Failed to get output desc %u, hr %#x.\n", index, hr);
            ++index;
            continue;
        }

        if (desc.Monitor == monitor)
        {
            *dxgi_output = output;
            return S_OK;
        }

        IDXGIOutput_Release(output);
        ++index;
    }
    if (hr != DXGI_ERROR_NOT_FOUND)
        WARN("Failed to enumerate outputs, hr %#x.\n", hr);

    WARN("Output could not be found.\n");
    return DXGI_ERROR_NOT_FOUND;
}

static HRESULT dxgi_swapchain_set_fullscreen_state(struct wined3d_swapchain_state *state,
        const struct wined3d_swapchain_desc *swapchain_desc, IDXGIOutput *output)
{
    struct dxgi_output *dxgi_output;
    struct dxgi_adapter *adapter;
    HRESULT hr;

    dxgi_output = unsafe_impl_from_IDXGIOutput(output);
    adapter = dxgi_output->adapter;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_state_set_fullscreen(state, swapchain_desc,
            adapter->factory->wined3d, adapter->ordinal, NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT dxgi_swapchain_resize_target(IDXGISwapChain1 *swapchain,
        struct wined3d_swapchain_state *state, const DXGI_MODE_DESC *target_mode_desc)
{
    struct wined3d_display_mode mode;
    struct dxgi_output *dxgi_output;
    struct dxgi_adapter *adapter;
    IDXGIOutput *output;
    HRESULT hr;

    if (!target_mode_desc)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    if (FAILED(hr = IDXGISwapChain1_GetContainingOutput(swapchain, &output)))
        return hr;
    dxgi_output = unsafe_impl_from_IDXGIOutput(output);
    adapter = dxgi_output->adapter;
    IDXGIOutput_Release(output);

    TRACE("Mode: %s.\n", debug_dxgi_mode(target_mode_desc));

    if (target_mode_desc->Scaling)
        FIXME("Ignoring scaling %#x.\n", target_mode_desc->Scaling);

    wined3d_display_mode_from_dxgi(&mode, target_mode_desc);

    return wined3d_swapchain_state_resize_target(state, adapter->factory->wined3d, adapter->ordinal, &mode);
}

static HWND d3d11_swapchain_get_hwnd(struct d3d11_swapchain *swapchain)
{
    struct wined3d_swapchain_desc wined3d_desc;

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    return wined3d_desc.device_window;
}

static inline struct d3d11_swapchain *d3d11_swapchain_from_IDXGISwapChain1(IDXGISwapChain1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_swapchain, IDXGISwapChain1_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_QueryInterface(IDXGISwapChain1 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGISwapChain)
            || IsEqualGUID(riid, &IID_IDXGISwapChain1))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_swapchain_AddRef(IDXGISwapChain1 *iface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    ULONG refcount = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u.\n", swapchain, refcount);

    if (refcount == 1)
        wined3d_swapchain_incref(swapchain->wined3d_swapchain);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_swapchain_Release(IDXGISwapChain1 *iface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    ULONG refcount = InterlockedDecrement(&swapchain->refcount);

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        IWineDXGIDevice *device = swapchain->device;
        if (swapchain->target)
        {
            WARN("Releasing fullscreen swapchain.\n");
            IDXGIOutput_Release(swapchain->target);
        }
        if (swapchain->factory)
            IDXGIFactory_Release(swapchain->factory);
        wined3d_swapchain_decref(swapchain->wined3d_swapchain);
        if (device)
            IWineDXGIDevice_Release(device);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetPrivateData(IDXGISwapChain1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetPrivateDataInterface(IDXGISwapChain1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetPrivateData(IDXGISwapChain1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetParent(IDXGISwapChain1 *iface, REFIID riid, void **parent)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    if (!swapchain->factory)
    {
        ERR("Implicit swapchain does not store reference to parent.\n");
        *parent = NULL;
        return E_NOINTERFACE;
    }

    return IDXGIFactory_QueryInterface(swapchain->factory, riid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDevice(IDXGISwapChain1 *iface, REFIID riid, void **device)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    if (!swapchain->device)
    {
        ERR("Implicit swapchain does not store reference to device.\n");
        *device = NULL;
        return E_NOINTERFACE;
    }

    return IWineDXGIDevice_QueryInterface(swapchain->device, riid, device);
}

/* IDXGISwapChain1 methods */

static HRESULT d3d11_swapchain_present(struct d3d11_swapchain *swapchain,
        unsigned int sync_interval, unsigned int flags)
{
    if (sync_interval > 4)
    {
        WARN("Invalid sync interval %u.\n", sync_interval);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (IsIconic(d3d11_swapchain_get_hwnd(swapchain)))
        return DXGI_STATUS_OCCLUDED;

    if (flags & ~DXGI_PRESENT_TEST)
        FIXME("Unimplemented flags %#x.\n", flags);
    if (flags & DXGI_PRESENT_TEST)
    {
        WARN("Returning S_OK for DXGI_PRESENT_TEST.\n");
        return S_OK;
    }

    return wined3d_swapchain_present(swapchain->wined3d_swapchain, NULL, NULL, NULL, sync_interval, 0);
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH d3d11_swapchain_Present(IDXGISwapChain1 *iface, UINT sync_interval, UINT flags)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, sync_interval %u, flags %#x.\n", iface, sync_interval, flags);

    return d3d11_swapchain_present(swapchain, sync_interval, flags);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetBuffer(IDXGISwapChain1 *iface,
        UINT buffer_idx, REFIID riid, void **surface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_texture *texture;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, buffer_idx %u, riid %s, surface %p\n",
            iface, buffer_idx, debugstr_guid(riid), surface);

    wined3d_mutex_lock();

    if (!(texture = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain, buffer_idx)))
    {
        wined3d_mutex_unlock();
        return DXGI_ERROR_INVALID_CALL;
    }

    parent = wined3d_texture_get_parent(texture);
    hr = IUnknown_QueryInterface(parent, riid, surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH d3d11_swapchain_SetFullscreenState(IDXGISwapChain1 *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc swapchain_desc;
    struct wined3d_swapchain_state *state;
    HRESULT hr;

    TRACE("iface %p, fullscreen %#x, target %p.\n", iface, fullscreen, target);

    if (!fullscreen && target)
    {
        WARN("Invalid call.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    if (target)
    {
        IDXGIOutput_AddRef(target);
    }
    else if (FAILED(hr = IDXGISwapChain1_GetContainingOutput(iface, &target)))
    {
        WARN("Failed to get target output for swapchain, hr %#x.\n", hr);
        return hr;
    }

    wined3d_mutex_lock();
    state = wined3d_swapchain_get_state(swapchain->wined3d_swapchain);
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &swapchain_desc);
    swapchain_desc.windowed = !fullscreen;
    hr = dxgi_swapchain_set_fullscreen_state(state, &swapchain_desc, target);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        IDXGIOutput_Release(target);

        return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
    }

    if (!fullscreen)
    {
        IDXGIOutput_Release(target);
        target = NULL;
    }

    if (swapchain->target)
        IDXGIOutput_Release(swapchain->target);
    swapchain->target = target;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFullscreenState(IDXGISwapChain1 *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc swapchain_desc;
    HRESULT hr;

    TRACE("iface %p, fullscreen %p, target %p.\n", iface, fullscreen, target);

    if (fullscreen || target)
    {
        wined3d_mutex_lock();
        wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &swapchain_desc);
        wined3d_mutex_unlock();
    }

    if (fullscreen)
        *fullscreen = !swapchain_desc.windowed;

    if (target)
    {
        if (!swapchain_desc.windowed)
        {
            if (!swapchain->target && FAILED(hr = IDXGISwapChain1_GetContainingOutput(iface, &swapchain->target)))
                return hr;

            *target = swapchain->target;
            IDXGIOutput_AddRef(*target);
        }
        else
        {
            *target = NULL;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDesc(IDXGISwapChain1 *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring ScanlineOrdering and Scaling.\n");

    desc->BufferDesc.Width = wined3d_desc.backbuffer_width;
    desc->BufferDesc.Height = wined3d_desc.backbuffer_height;
    desc->BufferDesc.RefreshRate.Numerator = wined3d_desc.refresh_rate;
    desc->BufferDesc.RefreshRate.Denominator = 1;
    desc->BufferDesc.Format = dxgi_format_from_wined3dformat(wined3d_desc.backbuffer_format);
    desc->BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc->BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc,
            wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    desc->BufferUsage = dxgi_usage_from_wined3d_bind_flags(wined3d_desc.backbuffer_bind_flags);
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->OutputWindow = wined3d_desc.device_window;
    desc->Windowed = wined3d_desc.windowed;
    desc->SwapEffect = dxgi_swap_effect_from_wined3d(wined3d_desc.swap_effect);
    desc->Flags = dxgi_swapchain_flags_from_wined3d(wined3d_desc.flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_ResizeBuffers(IDXGISwapChain1 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;
    struct wined3d_texture *texture;
    IUnknown *parent;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x.\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    for (i = 0; i < wined3d_desc.backbuffer_count; ++i)
    {
        texture = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain, i);
        parent = wined3d_texture_get_parent(texture);
        IUnknown_AddRef(parent);
        if (IUnknown_Release(parent))
        {
            wined3d_mutex_unlock();
            return DXGI_ERROR_INVALID_CALL;
        }
    }
    if (format != DXGI_FORMAT_UNKNOWN)
        wined3d_desc.backbuffer_format = wined3dformat_from_dxgi_format(format);
    hr = wined3d_swapchain_resize_buffers(swapchain->wined3d_swapchain, buffer_count, width, height,
            wined3d_desc.backbuffer_format, wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_ResizeTarget(IDXGISwapChain1 *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_state *state;

    TRACE("iface %p, target_mode_desc %p.\n", iface, target_mode_desc);

    state = wined3d_swapchain_get_state(swapchain->wined3d_swapchain);

    return dxgi_swapchain_resize_target(iface, state, target_mode_desc);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetContainingOutput(IDXGISwapChain1 *iface, IDXGIOutput **output)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;

    TRACE("iface %p, output %p.\n", iface, output);

    if (swapchain->target)
    {
        IDXGIOutput_AddRef(*output = swapchain->target);
        return S_OK;
    }

    if (SUCCEEDED(hr = d3d11_swapchain_GetDevice(iface, &IID_IDXGIDevice, (void **)&device)))
    {
        hr = IDXGIDevice_GetAdapter(device, &adapter);
        IDXGIDevice_Release(device);
    }

    if (SUCCEEDED(hr))
    {
        HWND hwnd = d3d11_swapchain_get_hwnd(swapchain);
        hr = dxgi_get_output_from_window(adapter, hwnd, output);
        IDXGIAdapter_Release(adapter);
    }
    else
    {
        WARN("Failed to get adapter, hr %#x.\n", hr);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFrameStatistics(IDXGISwapChain1 *iface,
        DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetLastPresentCount(IDXGISwapChain1 *iface,
        UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

/* IDXGISwapChain1 methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDesc1(IDXGISwapChain1 *iface, DXGI_SWAP_CHAIN_DESC1 *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring Stereo, Scaling and AlphaMode.\n");

    desc->Width = wined3d_desc.backbuffer_width;
    desc->Height = wined3d_desc.backbuffer_height;
    desc->Format = dxgi_format_from_wined3dformat(wined3d_desc.backbuffer_format);
    desc->Stereo = FALSE;
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc,
            wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    desc->BufferUsage = dxgi_usage_from_wined3d_bind_flags(wined3d_desc.backbuffer_bind_flags);
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->Scaling = DXGI_SCALING_STRETCH;
    desc->SwapEffect = dxgi_swap_effect_from_wined3d(wined3d_desc.swap_effect);
    desc->AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc->Flags = dxgi_swapchain_flags_from_wined3d(wined3d_desc.flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFullscreenDesc(IDXGISwapChain1 *iface,
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring ScanlineOrdering and Scaling.\n");

    desc->RefreshRate.Numerator = wined3d_desc.refresh_rate;
    desc->RefreshRate.Denominator = 1;
    desc->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc->Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc->Windowed = wined3d_desc.windowed;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetHwnd(IDXGISwapChain1 *iface, HWND *hwnd)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, hwnd %p.\n", iface, hwnd);

    if (!hwnd)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    *hwnd = d3d11_swapchain_get_hwnd(swapchain);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetCoreWindow(IDXGISwapChain1 *iface,
        REFIID iid, void **core_window)
{
    FIXME("iface %p, iid %s, core_window %p stub!\n", iface, debugstr_guid(iid), core_window);

    if (core_window)
        *core_window = NULL;

    return DXGI_ERROR_INVALID_CALL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_Present1(IDXGISwapChain1 *iface,
        UINT sync_interval, UINT flags, const DXGI_PRESENT_PARAMETERS *present_parameters)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, sync_interval %u, flags %#x, present_parameters %p.\n",
            iface, sync_interval, flags, present_parameters);

    if (present_parameters)
        FIXME("Ignored present parameters %p.\n", present_parameters);

    return d3d11_swapchain_present(swapchain, sync_interval, flags);
}

static BOOL STDMETHODCALLTYPE d3d11_swapchain_IsTemporaryMonoSupported(IDXGISwapChain1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetRestrictToOutput(IDXGISwapChain1 *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    if (!output)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *output = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetBackgroundColor(IDXGISwapChain1 *iface, const DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetBackgroundColor(IDXGISwapChain1 *iface, DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetRotation(IDXGISwapChain1 *iface, DXGI_MODE_ROTATION rotation)
{
    FIXME("iface %p, rotation %#x stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetRotation(IDXGISwapChain1 *iface, DXGI_MODE_ROTATION *rotation)
{
    FIXME("iface %p, rotation %p stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static const struct IDXGISwapChain1Vtbl d3d11_swapchain_vtbl =
{
    /* IUnknown methods */
    d3d11_swapchain_QueryInterface,
    d3d11_swapchain_AddRef,
    d3d11_swapchain_Release,
    /* IDXGIObject methods */
    d3d11_swapchain_SetPrivateData,
    d3d11_swapchain_SetPrivateDataInterface,
    d3d11_swapchain_GetPrivateData,
    d3d11_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    d3d11_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    d3d11_swapchain_Present,
    d3d11_swapchain_GetBuffer,
    d3d11_swapchain_SetFullscreenState,
    d3d11_swapchain_GetFullscreenState,
    d3d11_swapchain_GetDesc,
    d3d11_swapchain_ResizeBuffers,
    d3d11_swapchain_ResizeTarget,
    d3d11_swapchain_GetContainingOutput,
    d3d11_swapchain_GetFrameStatistics,
    d3d11_swapchain_GetLastPresentCount,
    /* IDXGISwapChain1 methods */
    d3d11_swapchain_GetDesc1,
    d3d11_swapchain_GetFullscreenDesc,
    d3d11_swapchain_GetHwnd,
    d3d11_swapchain_GetCoreWindow,
    d3d11_swapchain_Present1,
    d3d11_swapchain_IsTemporaryMonoSupported,
    d3d11_swapchain_GetRestrictToOutput,
    d3d11_swapchain_SetBackgroundColor,
    d3d11_swapchain_GetBackgroundColor,
    d3d11_swapchain_SetRotation,
    d3d11_swapchain_GetRotation,
};

static void STDMETHODCALLTYPE d3d11_swapchain_wined3d_object_released(void *parent)
{
    struct d3d11_swapchain *swapchain = parent;

    wined3d_private_store_cleanup(&swapchain->private_store);
    heap_free(parent);
}

static const struct wined3d_parent_ops d3d11_swapchain_wined3d_parent_ops =
{
    d3d11_swapchain_wined3d_object_released,
};

HRESULT d3d11_swapchain_init(struct d3d11_swapchain *swapchain, struct dxgi_device *device,
        struct wined3d_swapchain_desc *desc)
{
    BOOL fullscreen;
    HRESULT hr;

    /* A reference to the implicit swapchain is held by the wined3d device. In
     * order to avoid circular references we do not keep a reference to the
     * device in the implicit swapchain. */
    if (!(desc->flags & WINED3D_SWAPCHAIN_IMPLICIT))
    {
        if (desc->backbuffer_format == WINED3DFMT_UNKNOWN)
            return E_INVALIDARG;

        if (FAILED(hr = IWineDXGIAdapter_GetParent(device->adapter, &IID_IDXGIFactory,
                (void **)&swapchain->factory)))
        {
            WARN("Failed to get adapter parent, hr %#x.\n", hr);
            return hr;
        }
        IWineDXGIDevice_AddRef(swapchain->device = &device->IWineDXGIDevice_iface);
    }
    else
    {
        swapchain->device = NULL;
        swapchain->factory = NULL;
    }

    swapchain->IDXGISwapChain1_iface.lpVtbl = &d3d11_swapchain_vtbl;
    swapchain->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&swapchain->private_store);

    if (!desc->windowed && (!desc->backbuffer_width || !desc->backbuffer_height))
        FIXME("Fullscreen swapchain with back buffer width/height equal to 0 not supported properly.\n");

    fullscreen = !desc->windowed;
    desc->windowed = TRUE;
    if (FAILED(hr = wined3d_swapchain_create(device->wined3d_device, desc, swapchain,
            &d3d11_swapchain_wined3d_parent_ops, &swapchain->wined3d_swapchain)))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        goto cleanup;
    }

    swapchain->target = NULL;
    if (fullscreen)
    {
        struct wined3d_swapchain_state *state;

        desc->windowed = FALSE;
        state = wined3d_swapchain_get_state(swapchain->wined3d_swapchain);

        if (FAILED(hr = IDXGISwapChain1_GetContainingOutput(&swapchain->IDXGISwapChain1_iface,
                &swapchain->target)))
        {
            WARN("Failed to get target output for fullscreen swapchain, hr %#x.\n", hr);
            wined3d_swapchain_decref(swapchain->wined3d_swapchain);
            goto cleanup;
        }

        if (FAILED(hr = dxgi_swapchain_set_fullscreen_state(state, desc, swapchain->target)))
        {
            WARN("Failed to set fullscreen state, hr %#x.\n", hr);
            IDXGIOutput_Release(swapchain->target);
            wined3d_swapchain_decref(swapchain->wined3d_swapchain);
            goto cleanup;
        }

    }
    wined3d_mutex_unlock();

    return S_OK;

cleanup:
    wined3d_private_store_cleanup(&swapchain->private_store);
    wined3d_mutex_unlock();
    if (swapchain->factory)
        IDXGIFactory_Release(swapchain->factory);
    if (swapchain->device)
        IWineDXGIDevice_Release(swapchain->device);
    return hr;
}

#ifdef SONAME_LIBVKD3D

#ifdef USE_WIN32_VULKAN

static void *load_library(const char *name)
{
    return LoadLibraryA(name);
}

static void *get_library_proc(void *handle, const char *name)
{
    return (void *)GetProcAddress(handle, name);
}

static void close_library(void *handle)
{
    if (handle)
        FreeLibrary(handle);
}

static PFN_vkGetInstanceProcAddr load_vulkan(void **vulkan_handle)
{
    *vulkan_handle = LoadLibraryA("vulkan-1.dll");
    return (void *)GetProcAddress(*vulkan_handle, "vkGetInstanceProcAddr");
}

#else

static void *load_library(const char *name)
{
    return wine_dlopen(name, RTLD_NOW, NULL, 0);
}

static void *get_library_proc(void *handle, const char *name)
{
    return wine_dlsym(handle, name, NULL, 0);
}

static void close_library(void *handle)
{
    if (handle)
        wine_dlclose(handle, NULL, 0);
}

static PFN_vkGetInstanceProcAddr load_vulkan(void **vulkan_handle)
{
    const struct vulkan_funcs *vk_funcs;
    HDC hdc;

    *vulkan_handle = NULL;

    hdc = GetDC(0);
    vk_funcs = __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
    ReleaseDC(0, hdc);

    if (vk_funcs)
        return (PFN_vkGetInstanceProcAddr)vk_funcs->p_vkGetInstanceProcAddr;

    return NULL;
}

#endif  /* USE_WIN32_VULKAN */

static PFN_vkd3d_acquire_vk_queue vkd3d_acquire_vk_queue;
static PFN_vkd3d_create_image_resource vkd3d_create_image_resource;
static PFN_vkd3d_get_device_parent vkd3d_get_device_parent;
static PFN_vkd3d_get_vk_device vkd3d_get_vk_device;
static PFN_vkd3d_get_vk_format vkd3d_get_vk_format;
static PFN_vkd3d_get_vk_physical_device vkd3d_get_vk_physical_device;
static PFN_vkd3d_get_vk_queue_family_index vkd3d_get_vk_queue_family_index;
static PFN_vkd3d_instance_from_device vkd3d_instance_from_device;
static PFN_vkd3d_instance_get_vk_instance vkd3d_instance_get_vk_instance;
static PFN_vkd3d_release_vk_queue vkd3d_release_vk_queue;
static PFN_vkd3d_resource_decref vkd3d_resource_decref;
static PFN_vkd3d_resource_incref vkd3d_resource_incref;

struct dxgi_vk_funcs
{
    PFN_vkAcquireNextImageKHR p_vkAcquireNextImageKHR;
    PFN_vkAllocateCommandBuffers p_vkAllocateCommandBuffers;
    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkBeginCommandBuffer p_vkBeginCommandBuffer;
    PFN_vkBindImageMemory p_vkBindImageMemory;
    PFN_vkCmdBlitImage p_vkCmdBlitImage;
    PFN_vkCmdPipelineBarrier p_vkCmdPipelineBarrier;
    PFN_vkCreateCommandPool p_vkCreateCommandPool;
    PFN_vkCreateFence p_vkCreateFence;
    PFN_vkCreateImage p_vkCreateImage;
    PFN_vkCreateSemaphore p_vkCreateSemaphore;
    PFN_vkCreateSwapchainKHR p_vkCreateSwapchainKHR;
    PFN_vkCreateWin32SurfaceKHR p_vkCreateWin32SurfaceKHR;
    PFN_vkDestroyCommandPool p_vkDestroyCommandPool;
    PFN_vkDestroyFence p_vkDestroyFence;
    PFN_vkDestroyImage p_vkDestroyImage;
    PFN_vkDestroySemaphore p_vkDestroySemaphore;
    PFN_vkDestroySurfaceKHR p_vkDestroySurfaceKHR;
    PFN_vkResetCommandBuffer p_vkResetCommandBuffer;
    PFN_vkDestroySwapchainKHR p_vkDestroySwapchainKHR;
    PFN_vkEndCommandBuffer p_vkEndCommandBuffer;
    PFN_vkFreeMemory p_vkFreeMemory;
    PFN_vkGetImageMemoryRequirements p_vkGetImageMemoryRequirements;
    PFN_vkGetInstanceProcAddr p_vkGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceMemoryProperties p_vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR p_vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR p_vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR p_vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR p_vkGetPhysicalDeviceWin32PresentationSupportKHR;
    PFN_vkGetSwapchainImagesKHR p_vkGetSwapchainImagesKHR;
    PFN_vkQueuePresentKHR p_vkQueuePresentKHR;
    PFN_vkQueueSubmit p_vkQueueSubmit;
    PFN_vkQueueWaitIdle p_vkQueueWaitIdle;
    PFN_vkResetFences p_vkResetFences;
    PFN_vkWaitForFences p_vkWaitForFences;

    void *vulkan_module;
};

static HRESULT hresult_from_vk_result(VkResult vr)
{
    switch (vr)
    {
        case VK_SUCCESS:
            return S_OK;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return E_OUTOFMEMORY;
        default:
            FIXME("Unhandled VkResult %d.\n", vr);
            return E_FAIL;
    }
}

#define INVALID_VK_IMAGE_INDEX (~(uint32_t)0)

struct d3d12_swapchain
{
    IDXGISwapChain3 IDXGISwapChain3_iface;
    LONG refcount;
    struct wined3d_private_store private_store;

    struct wined3d_swapchain_state *state;

    VkSwapchainKHR vk_swapchain;
    VkSurfaceKHR vk_surface;
    VkFence vk_fence;
    VkInstance vk_instance;
    VkDevice vk_device;
    VkPhysicalDevice vk_physical_device;
    VkDeviceMemory vk_memory;
    VkCommandPool vk_cmd_pool;
    VkImage vk_images[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    VkImage vk_swapchain_images[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    VkCommandBuffer vk_cmd_buffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    VkSemaphore vk_semaphores[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    ID3D12Resource *buffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    unsigned int buffer_count;
    unsigned int vk_swapchain_width;
    unsigned int vk_swapchain_height;
    VkPresentModeKHR present_mode;

    uint32_t vk_image_index;
    unsigned int current_buffer_index;
    struct dxgi_vk_funcs vk_funcs;

    ID3D12CommandQueue *command_queue;
    ID3D12Device *device;
    IWineDXGIFactory *factory;

    HWND window;
    IDXGIOutput *target;
    DXGI_SWAP_CHAIN_DESC1 desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
};

static DXGI_FORMAT dxgi_format_from_vk_format(VkFormat vk_format)
{
    switch (vk_format)
    {
        case VK_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        default:
            WARN("Unhandled format %#x.\n", vk_format);
            return DXGI_FORMAT_UNKNOWN;
    }
}

static VkFormat get_swapchain_fallback_format(VkFormat vk_format)
{
    switch (vk_format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return VK_FORMAT_B8G8R8A8_UNORM;
        default:
            WARN("Unhandled format %#x.\n", vk_format);
            return VK_FORMAT_UNDEFINED;
    }
}

static HRESULT select_vk_format(const struct dxgi_vk_funcs *vk_funcs,
        VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, VkFormat *vk_format)
{
    VkSurfaceFormatKHR *formats;
    uint32_t format_count;
    VkFormat format;
    unsigned int i;
    VkResult vr;

    *vk_format = VK_FORMAT_UNDEFINED;

    format = vkd3d_get_vk_format(swapchain_desc->Format);

    vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &format_count, NULL);
    if (vr < 0 || !format_count)
    {
        WARN("Failed to get supported surface formats, vr %d.\n", vr);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (!(formats = heap_calloc(format_count, sizeof(*formats))))
        return E_OUTOFMEMORY;

    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,
            vk_surface, &format_count, formats)) < 0)
    {
        WARN("Failed to enumerate supported surface formats, vr %d.\n", vr);
        heap_free(formats);
        return hresult_from_vk_result(vr);
    }

    for (i = 0; i < format_count; ++i)
    {
        if (formats[i].format == format && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            break;
    }
    if (i == format_count)
    {
        /* Try to create a swapchain with format conversion. */
        format = get_swapchain_fallback_format(format);
        WARN("Failed to find Vulkan swapchain format for %s.\n", debug_dxgi_format(swapchain_desc->Format));
        for (i = 0; i < format_count; ++i)
        {
            if (formats[i].format == format && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                format = formats[i].format;
                break;
            }
        }
    }
    heap_free(formats);
    if (i == format_count)
    {
        FIXME("Failed to find Vulkan swapchain format for %s.\n", debug_dxgi_format(swapchain_desc->Format));
        return DXGI_ERROR_UNSUPPORTED;
    }

    TRACE("Using Vulkan swapchain format %#x.\n", format);

    *vk_format = format;
    return S_OK;
}

static HRESULT vk_select_memory_type(const struct dxgi_vk_funcs *vk_funcs,
        VkPhysicalDevice vk_physical_device, uint32_t memory_type_mask,
        VkMemoryPropertyFlags flags, uint32_t *memory_type_index)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    unsigned int i;

    vk_funcs->p_vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &memory_properties);
    for (i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if (!(memory_type_mask & (1u << i)))
            continue;

        if ((memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
        {
            *memory_type_index = i;
            return S_OK;
        }
    }

    FIXME("Failed to find memory type (allowed types %#x).\n", memory_type_mask);
    return E_FAIL;
}

static BOOL d3d12_swapchain_is_present_mode_supported(struct d3d12_swapchain *swapchain,
        VkPresentModeKHR present_mode)
{
    VkPhysicalDevice vk_physical_device = swapchain->vk_physical_device;
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkPresentModeKHR *modes;
    uint32_t count, i;
    BOOL supported;
    VkResult vr;

    if (present_mode == VK_PRESENT_MODE_FIFO_KHR)
        return TRUE;

    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,
            swapchain->vk_surface, &count, NULL)) < 0)
    {
        WARN("Failed to get count of available present modes, vr %d.\n", vr);
        return FALSE;
    }

    supported = FALSE;

    if (!(modes = heap_calloc(count, sizeof(*modes))))
        return FALSE;
    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,
            swapchain->vk_surface, &count, modes)) >= 0)
    {
        for (i = 0; i < count; ++i)
        {
            if (modes[i] == present_mode)
            {
                supported = TRUE;
                break;
            }
        }
    }
    else
    {
        WARN("Failed to get available present modes, vr %d.\n", vr);
    }
    heap_free(modes);

    return supported;
}

static BOOL d3d12_swapchain_has_user_images(struct d3d12_swapchain *swapchain)
{
    return !!swapchain->vk_images[0];
}

static HRESULT d3d12_swapchain_create_user_buffers(struct d3d12_swapchain *swapchain, VkFormat vk_format)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkDeviceSize image_offset[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    VkDevice vk_device = swapchain->vk_device;
    VkMemoryAllocateInfo allocate_info;
    VkMemoryRequirements requirements;
    VkImageCreateInfo image_info;
    uint32_t memory_type_mask;
    VkDeviceSize memory_size;
    unsigned int i;
    VkResult vr;
    HRESULT hr;

    if (d3d12_swapchain_has_user_images(swapchain))
        return S_OK;

    memset(&image_info, 0, sizeof(image_info));
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = vk_format;
    image_info.extent.width = swapchain->desc.Width;
    image_info.extent.height = swapchain->desc.Height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        assert(swapchain->vk_images[i] == VK_NULL_HANDLE);
        if ((vr = vk_funcs->p_vkCreateImage(vk_device, &image_info, NULL, &swapchain->vk_images[i])) < 0)
        {
            WARN("Failed to create Vulkan image, vr %d.\n", vr);
            swapchain->vk_images[i] = VK_NULL_HANDLE;
            return hresult_from_vk_result(vr);
        }
    }

    memory_size = 0;
    memory_type_mask = ~0u;
    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        vk_funcs->p_vkGetImageMemoryRequirements(vk_device, swapchain->vk_images[i], &requirements);

        TRACE("Size %s, alignment %s, memory types %#x.\n",
                wine_dbgstr_longlong(requirements.size), wine_dbgstr_longlong(requirements.alignment),
                requirements.memoryTypeBits);

        image_offset[i] = (memory_size + (requirements.alignment - 1)) & ~(requirements.alignment - 1);
        memory_size = image_offset[i] + requirements.size;

        memory_type_mask &= requirements.memoryTypeBits;
    }

    TRACE("Allocating %s bytes for user images.\n", wine_dbgstr_longlong(memory_size));

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.allocationSize = memory_size;

    if (FAILED(hr = vk_select_memory_type(vk_funcs, swapchain->vk_physical_device,
            memory_type_mask, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocate_info.memoryTypeIndex)))
        return hr;

    assert(swapchain->vk_memory == VK_NULL_HANDLE);
    if ((vr = vk_funcs->p_vkAllocateMemory(vk_device, &allocate_info, NULL, &swapchain->vk_memory)) < 0)
    {
        WARN("Failed to allocate device memory, vr %d.\n", vr);
        swapchain->vk_memory = VK_NULL_HANDLE;
        return hresult_from_vk_result(vr);
    }

    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        if ((vr = vk_funcs->p_vkBindImageMemory(vk_device, swapchain->vk_images[i],
                swapchain->vk_memory, image_offset[i])) < 0)
        {
            WARN("Failed to bind image memory, vr %d.\n", vr);
            return hresult_from_vk_result(vr);
        }
    }

    return S_OK;
}

static void vk_cmd_image_barrier(const struct dxgi_vk_funcs *vk_funcs, VkCommandBuffer cmd_buffer,
        VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
        VkImageLayout old_layout, VkImageLayout new_layout, VkImage image)
{
    VkImageMemoryBarrier barrier;

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vk_funcs->p_vkCmdPipelineBarrier(cmd_buffer,
            src_stage_mask, dst_stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static VkResult d3d12_swapchain_record_swapchain_blit(struct d3d12_swapchain *swapchain,
        VkCommandBuffer vk_cmd_buffer, VkImage vk_dst_image, VkImage vk_src_image)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkCommandBufferBeginInfo begin_info;
    VkImageBlit blit;
    VkFilter filter;
    VkResult vr;

    if (swapchain->desc.Width != swapchain->vk_swapchain_width
            || swapchain->desc.Height != swapchain->vk_swapchain_height)
        filter = VK_FILTER_LINEAR;
    else
        filter = VK_FILTER_NEAREST;

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;

    if ((vr = vk_funcs->p_vkBeginCommandBuffer(vk_cmd_buffer, &begin_info)) < 0)
    {
        WARN("Failed to begin command buffer, vr %d.\n", vr);
        return vr;
    }

    vk_cmd_image_barrier(vk_funcs, vk_cmd_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_dst_image);

    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = swapchain->desc.Width;
    blit.srcOffsets[1].y = swapchain->desc.Height;
    blit.srcOffsets[1].z = 1;
    blit.dstSubresource = blit.srcSubresource;
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = swapchain->vk_swapchain_width;
    blit.dstOffsets[1].y = swapchain->vk_swapchain_height;
    blit.dstOffsets[1].z = 1;

    vk_funcs->p_vkCmdBlitImage(vk_cmd_buffer,
            vk_src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vk_dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, filter);

    vk_cmd_image_barrier(vk_funcs, vk_cmd_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, vk_dst_image);

    if ((vr = vk_funcs->p_vkEndCommandBuffer(vk_cmd_buffer)) < 0)
        WARN("Failed to end command buffer, vr %d.\n", vr);

    return vr;
}

static HRESULT d3d12_swapchain_prepare_command_buffers(struct d3d12_swapchain *swapchain,
        uint32_t queue_family_index)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkDevice vk_device = swapchain->vk_device;
    VkCommandBufferAllocateInfo allocate_info;
    VkSemaphoreCreateInfo semaphore_info;
    VkCommandPoolCreateInfo pool_info;
    unsigned int i;
    VkResult vr;

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = NULL;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_index;

    assert(swapchain->vk_cmd_pool == VK_NULL_HANDLE);
    if ((vr = vk_funcs->p_vkCreateCommandPool(vk_device, &pool_info,
            NULL, &swapchain->vk_cmd_pool)) < 0)
    {
        WARN("Failed to create command pool, vr %d.\n", vr);
        swapchain->vk_cmd_pool = VK_NULL_HANDLE;
        return hresult_from_vk_result(vr);
    }

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.commandPool = swapchain->vk_cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = swapchain->buffer_count;

    if ((vr = vk_funcs->p_vkAllocateCommandBuffers(vk_device, &allocate_info,
            swapchain->vk_cmd_buffers)) < 0)
    {
        WARN("Failed to allocate command buffers, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    for (i = 0; i < swapchain->buffer_count; ++i)
    {
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = NULL;
        semaphore_info.flags = 0;

        assert(swapchain->vk_semaphores[i] == VK_NULL_HANDLE);
        if ((vr = vk_funcs->p_vkCreateSemaphore(vk_device, &semaphore_info,
                NULL, &swapchain->vk_semaphores[i])) < 0)
        {
            WARN("Failed to create semaphore, vr %d.\n", vr);
            swapchain->vk_semaphores[i] = VK_NULL_HANDLE;
            return hresult_from_vk_result(vr);
        }
    }

    return S_OK;
}

static HRESULT d3d12_swapchain_create_buffers(struct d3d12_swapchain *swapchain,
        VkFormat vk_swapchain_format, VkFormat vk_format)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    struct vkd3d_image_resource_create_info resource_info;
    VkSwapchainKHR vk_swapchain = swapchain->vk_swapchain;
    ID3D12CommandQueue *queue = swapchain->command_queue;
    VkDevice vk_device = swapchain->vk_device;
    ID3D12Device *device = swapchain->device;
    uint32_t image_count, queue_family_index;
    D3D12_COMMAND_QUEUE_DESC queue_desc;
    unsigned int i;
    VkResult vr;
    HRESULT hr;

    if ((vr = vk_funcs->p_vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &image_count, NULL)) < 0)
    {
        WARN("Failed to get Vulkan swapchain images, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }
    if (image_count > ARRAY_SIZE(swapchain->vk_swapchain_images))
    {
        FIXME("Unsupported Vulkan swapchain image count %u.\n", image_count);
        return E_FAIL;
    }
    swapchain->buffer_count = image_count;
    if ((vr = vk_funcs->p_vkGetSwapchainImagesKHR(vk_device, vk_swapchain,
            &image_count, swapchain->vk_swapchain_images)) < 0)
    {
        WARN("Failed to get Vulkan swapchain images, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    resource_info.type = VKD3D_STRUCTURE_TYPE_IMAGE_RESOURCE_CREATE_INFO;
    resource_info.next = NULL;
    resource_info.desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_info.desc.Alignment = 0;
    resource_info.desc.Width = swapchain->desc.Width;
    resource_info.desc.Height = swapchain->desc.Height;
    resource_info.desc.DepthOrArraySize = 1;
    resource_info.desc.MipLevels = 1;
    resource_info.desc.Format = dxgi_format_from_vk_format(vk_format);
    resource_info.desc.SampleDesc.Count = 1;
    resource_info.desc.SampleDesc.Quality = 0;
    resource_info.desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_info.desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resource_info.flags = VKD3D_RESOURCE_INITIAL_STATE_TRANSITION | VKD3D_RESOURCE_PRESENT_STATE_TRANSITION;

    queue_desc = ID3D12CommandQueue_GetDesc(queue);
    if (queue_desc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        /* vkCmdBlitImage() is only supported for graphics queues. */
        FIXME("Swapchain blit not implemented for command queue type %#x.\n", queue_desc.Type);
        if (vk_swapchain_format != vk_format)
            return E_NOTIMPL;
        if (image_count != swapchain->desc.BufferCount)
        {
            FIXME("Got %u swapchain images, expected %u.\n", image_count, swapchain->desc.BufferCount);
            return E_NOTIMPL;
        }
    }
    queue_family_index = vkd3d_get_vk_queue_family_index(queue);

    if (queue_desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        TRACE("Creating user swapchain buffers.\n");

        if (FAILED(hr = d3d12_swapchain_create_user_buffers(swapchain, vk_format)))
            return hr;

        if (FAILED(hr = d3d12_swapchain_prepare_command_buffers(swapchain, queue_family_index)))
            return hr;
    }

    if (swapchain->buffers[0])
        return S_OK;

    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        if (swapchain->vk_images[i])
        {
            resource_info.vk_image = swapchain->vk_images[i];
            resource_info.present_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        else
        {
            resource_info.vk_image = swapchain->vk_swapchain_images[i];
            resource_info.present_state = D3D12_RESOURCE_STATE_PRESENT;
        }

        if (FAILED(hr = vkd3d_create_image_resource(device, &resource_info, &swapchain->buffers[i])))
        {
            WARN("Failed to create vkd3d resource for Vulkan image %u, hr %#x.\n", i, hr);
            return hr;
        }

        vkd3d_resource_incref(swapchain->buffers[i]);
        ID3D12Resource_Release(swapchain->buffers[i]);
    }

    return S_OK;
}

static VkResult d3d12_swapchain_acquire_next_vulkan_image(struct d3d12_swapchain *swapchain)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkDevice vk_device = swapchain->vk_device;
    VkFence vk_fence = swapchain->vk_fence;
    VkResult vr;

    swapchain->vk_image_index = INVALID_VK_IMAGE_INDEX;

    if ((vr = vk_funcs->p_vkAcquireNextImageKHR(vk_device, swapchain->vk_swapchain, UINT64_MAX,
            VK_NULL_HANDLE, vk_fence, &swapchain->vk_image_index)) < 0)
    {
        WARN("Failed to acquire next Vulkan image, vr %d.\n", vr);
        return vr;
    }

    if ((vr = vk_funcs->p_vkWaitForFences(vk_device, 1, &vk_fence, VK_TRUE, UINT64_MAX)) != VK_SUCCESS)
    {
        ERR("Failed to wait for fence, vr %d.\n", vr);
        return vr;
    }
    if ((vr = vk_funcs->p_vkResetFences(vk_device, 1, &vk_fence)) < 0)
        ERR("Failed to reset fence, vr %d.\n", vr);

    return vr;
}

static VkResult d3d12_swapchain_acquire_next_back_buffer(struct d3d12_swapchain *swapchain)
{
    VkResult vr;

    /* If we don't have user images, we need to acquire a Vulkan image in order
     * to get the correct value for the current back buffer index. */
    if (d3d12_swapchain_has_user_images(swapchain))
        return VK_SUCCESS;

    if ((vr = d3d12_swapchain_acquire_next_vulkan_image(swapchain)) >= 0)
        swapchain->current_buffer_index = swapchain->vk_image_index;
    else
        ERR("Failed to acquire Vulkan image, vr %d.\n", vr);

    return vr;
}

static void d3d12_swapchain_destroy_buffers(struct d3d12_swapchain *swapchain, BOOL destroy_user_buffers)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkQueue vk_queue;
    unsigned int i;

    if (swapchain->command_queue)
    {
        if ((vk_queue = vkd3d_acquire_vk_queue(swapchain->command_queue)))
        {
            vk_funcs->p_vkQueueWaitIdle(vk_queue);

            vkd3d_release_vk_queue(swapchain->command_queue);
        }
        else
        {
            WARN("Failed to acquire Vulkan queue.\n");
        }
    }

    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        if (swapchain->buffers[i] && (destroy_user_buffers || !d3d12_swapchain_has_user_images(swapchain)))
        {
            vkd3d_resource_decref(swapchain->buffers[i]);
            swapchain->buffers[i] = NULL;
        }
        if (swapchain->vk_device && destroy_user_buffers)
        {
            vk_funcs->p_vkDestroyImage(swapchain->vk_device, swapchain->vk_images[i], NULL);
            swapchain->vk_images[i] = VK_NULL_HANDLE;
        }
    }

    if (swapchain->vk_device)
    {
        for (i = 0; i < swapchain->buffer_count; ++i)
        {
            vk_funcs->p_vkDestroySemaphore(swapchain->vk_device, swapchain->vk_semaphores[i], NULL);
            swapchain->vk_semaphores[i] = VK_NULL_HANDLE;
        }
        if (destroy_user_buffers)
        {
            vk_funcs->p_vkFreeMemory(swapchain->vk_device, swapchain->vk_memory, NULL);
            swapchain->vk_memory = VK_NULL_HANDLE;
        }
        vk_funcs->p_vkDestroyCommandPool(swapchain->vk_device, swapchain->vk_cmd_pool, NULL);
        swapchain->vk_cmd_pool = VK_NULL_HANDLE;
    }
}

static HRESULT d3d12_swapchain_create_vulkan_swapchain(struct d3d12_swapchain *swapchain)
{
    VkPhysicalDevice vk_physical_device = swapchain->vk_physical_device;
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkSwapchainCreateInfoKHR vk_swapchain_desc;
    VkDevice vk_device = swapchain->vk_device;
    VkFormat vk_format, vk_swapchain_format;
    unsigned int width, height, image_count;
    VkSurfaceCapabilitiesKHR surface_caps;
    VkSwapchainKHR vk_swapchain;
    VkImageUsageFlags usage;
    VkResult vr;
    HRESULT hr;

    if (!(vk_format = vkd3d_get_vk_format(swapchain->desc.Format)))
    {
        WARN("Invalid format %#x.\n", swapchain->desc.Format);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (FAILED(hr = select_vk_format(vk_funcs, vk_physical_device,
            swapchain->vk_surface, &swapchain->desc, &vk_swapchain_format)))
        return hr;

    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device,
            swapchain->vk_surface, &surface_caps)) < 0)
    {
        WARN("Failed to get surface capabilities, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    image_count = swapchain->desc.BufferCount;
    image_count = max(image_count, surface_caps.minImageCount);
    if (surface_caps.maxImageCount)
        image_count = min(image_count, surface_caps.maxImageCount);

    if (image_count != swapchain->desc.BufferCount)
    {
        WARN("Buffer count %u is not supported (%u-%u).\n", swapchain->desc.BufferCount,
                surface_caps.minImageCount, surface_caps.maxImageCount);
    }

    width = swapchain->desc.Width;
    height = swapchain->desc.Height;
    width = max(width, surface_caps.minImageExtent.width);
    width = min(width, surface_caps.maxImageExtent.width);
    height = max(height, surface_caps.minImageExtent.height);
    height = min(height, surface_caps.maxImageExtent.height);

    if (width != swapchain->desc.Width || height != swapchain->desc.Height)
    {
        WARN("Swapchain dimensions %ux%u are not supported (%u-%u x %u-%u).\n",
                swapchain->desc.Width, swapchain->desc.Height,
                surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width,
                surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
    }

    TRACE("Vulkan swapchain extent %ux%u.\n", width, height);

    if (!(surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
    {
        FIXME("Unsupported alpha mode.\n");
        return DXGI_ERROR_UNSUPPORTED;
    }

    usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usage |= surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage |= surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!(usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || !(usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        WARN("Transfer not supported for swapchain images.\n");

    vk_swapchain_desc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vk_swapchain_desc.pNext = NULL;
    vk_swapchain_desc.flags = 0;
    vk_swapchain_desc.surface = swapchain->vk_surface;
    vk_swapchain_desc.minImageCount = image_count;
    vk_swapchain_desc.imageFormat = vk_swapchain_format;
    vk_swapchain_desc.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    vk_swapchain_desc.imageExtent.width = width;
    vk_swapchain_desc.imageExtent.height = height;
    vk_swapchain_desc.imageArrayLayers = 1;
    vk_swapchain_desc.imageUsage = usage;
    vk_swapchain_desc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk_swapchain_desc.queueFamilyIndexCount = 0;
    vk_swapchain_desc.pQueueFamilyIndices = NULL;
    vk_swapchain_desc.preTransform = surface_caps.currentTransform;
    vk_swapchain_desc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vk_swapchain_desc.presentMode = swapchain->present_mode;
    vk_swapchain_desc.clipped = VK_TRUE;
    vk_swapchain_desc.oldSwapchain = swapchain->vk_swapchain;
    if ((vr = vk_funcs->p_vkCreateSwapchainKHR(vk_device, &vk_swapchain_desc, NULL, &vk_swapchain)) < 0)
    {
        WARN("Failed to create Vulkan swapchain, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    if (swapchain->vk_swapchain)
        vk_funcs->p_vkDestroySwapchainKHR(swapchain->vk_device, swapchain->vk_swapchain, NULL);

    swapchain->vk_swapchain = vk_swapchain;
    swapchain->vk_swapchain_width = width;
    swapchain->vk_swapchain_height = height;

    swapchain->vk_image_index = INVALID_VK_IMAGE_INDEX;

    return d3d12_swapchain_create_buffers(swapchain, vk_swapchain_format, vk_format);
}

static HRESULT d3d12_swapchain_recreate_vulkan_swapchain(struct d3d12_swapchain *swapchain)
{
    VkResult vr;
    HRESULT hr;

    if (SUCCEEDED(hr = d3d12_swapchain_create_vulkan_swapchain(swapchain)))
    {
        vr = d3d12_swapchain_acquire_next_back_buffer(swapchain);
        hr = hresult_from_vk_result(vr);
    }
    else
    {
        ERR("Failed to recreate Vulkan swapchain, hr %#x.\n", hr);
    }

    return hr;
}

static inline struct d3d12_swapchain *d3d12_swapchain_from_IDXGISwapChain3(IDXGISwapChain3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_swapchain, IDXGISwapChain3_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_QueryInterface(IDXGISwapChain3 *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(iid, &IID_IDXGISwapChain)
            || IsEqualGUID(iid, &IID_IDXGISwapChain1)
            || IsEqualGUID(iid, &IID_IDXGISwapChain2)
            || IsEqualGUID(iid, &IID_IDXGISwapChain3))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_swapchain_AddRef(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    ULONG refcount = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u.\n", swapchain, refcount);

    return refcount;
}

static void d3d12_swapchain_destroy(struct d3d12_swapchain *swapchain)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    void *vulkan_module = vk_funcs->vulkan_module;

    d3d12_swapchain_destroy_buffers(swapchain, TRUE);

    if (swapchain->command_queue)
        ID3D12CommandQueue_Release(swapchain->command_queue);

    wined3d_private_store_cleanup(&swapchain->private_store);

    if (swapchain->vk_device)
    {
        vk_funcs->p_vkDestroyFence(swapchain->vk_device, swapchain->vk_fence, NULL);
        vk_funcs->p_vkDestroySwapchainKHR(swapchain->vk_device, swapchain->vk_swapchain, NULL);
    }

    if (swapchain->vk_instance)
        vk_funcs->p_vkDestroySurfaceKHR(swapchain->vk_instance, swapchain->vk_surface, NULL);

    if (swapchain->target)
    {
        WARN("Destroying fullscreen swapchain.\n");
        IDXGIOutput_Release(swapchain->target);
    }

    if (swapchain->device)
        ID3D12Device_Release(swapchain->device);

    if (swapchain->factory)
        IWineDXGIFactory_Release(swapchain->factory);

    close_library(vulkan_module);

    wined3d_swapchain_state_destroy(swapchain->state);
}

static ULONG STDMETHODCALLTYPE d3d12_swapchain_Release(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    ULONG refcount = InterlockedDecrement(&swapchain->refcount);

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        d3d12_swapchain_destroy(swapchain);
        heap_free(swapchain);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetPrivateData(IDXGISwapChain3 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetPrivateDataInterface(IDXGISwapChain3 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetPrivateData(IDXGISwapChain3 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetParent(IDXGISwapChain3 *iface, REFIID iid, void **parent)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    return IWineDXGIFactory_QueryInterface(swapchain->factory, iid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDevice(IDXGISwapChain3 *iface, REFIID iid, void **device)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return ID3D12Device_QueryInterface(swapchain->device, iid, device);
}

/* IDXGISwapChain methods */

static HRESULT d3d12_swapchain_set_sync_interval(struct d3d12_swapchain *swapchain,
        unsigned int sync_interval)
{
    VkPresentModeKHR present_mode;

    switch (sync_interval)
    {
        case 0:
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        default:
            FIXME("Unsupported sync interval %u.\n", sync_interval);
        case 1:
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
            break;
    }

    if (swapchain->present_mode == present_mode)
        return S_OK;

    if (!d3d12_swapchain_has_user_images(swapchain))
    {
        FIXME("Cannot recreate swapchain without user images.\n");
        return S_OK;
    }

    if (!d3d12_swapchain_is_present_mode_supported(swapchain, present_mode))
    {
        FIXME("Vulkan present mode %#x is not supported.\n", present_mode);
        return S_OK;
    }

    d3d12_swapchain_destroy_buffers(swapchain, FALSE);
    swapchain->present_mode = present_mode;
    return d3d12_swapchain_recreate_vulkan_swapchain(swapchain);
}

static VkResult d3d12_swapchain_queue_present(struct d3d12_swapchain *swapchain, VkQueue vk_queue)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkPresentInfoKHR present_info;
    VkSubmitInfo submit_info;
    VkResult vr;

    if (swapchain->vk_image_index == INVALID_VK_IMAGE_INDEX)
    {
        if ((vr = d3d12_swapchain_acquire_next_vulkan_image(swapchain)) < 0)
            return vr;
    }

    assert(swapchain->vk_image_index < swapchain->buffer_count);

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 0;
    present_info.pWaitSemaphores = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->vk_swapchain;
    present_info.pImageIndices = &swapchain->vk_image_index;
    present_info.pResults = NULL;

    if (d3d12_swapchain_has_user_images(swapchain))
    {
        /* blit */
        VkCommandBuffer vk_cmd_buffer = swapchain->vk_cmd_buffers[swapchain->vk_image_index];
        VkImage vk_dst_image = swapchain->vk_swapchain_images[swapchain->vk_image_index];
        VkImage vk_src_image = swapchain->vk_images[swapchain->current_buffer_index];

        if ((vr = vk_funcs->p_vkResetCommandBuffer(vk_cmd_buffer, 0)) < 0)
        {
            ERR("Failed to reset command buffer, vr %d.\n", vr);
            return vr;
        }

        if ((vr = d3d12_swapchain_record_swapchain_blit(swapchain,
                vk_cmd_buffer, vk_dst_image, vk_src_image)) < 0 )
            return vr;

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &vk_cmd_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &swapchain->vk_semaphores[swapchain->vk_image_index];

        if ((vr = vk_funcs->p_vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE)) < 0)
        {
            ERR("Failed to blit swapchain buffer, vr %d.\n", vr);
            return vr;
        }

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &swapchain->vk_semaphores[swapchain->vk_image_index];
    }

    if ((vr = vk_funcs->p_vkQueuePresentKHR(vk_queue, &present_info)) >= 0)
        swapchain->vk_image_index = INVALID_VK_IMAGE_INDEX;

    return vr;
}

static HRESULT d3d12_swapchain_present(struct d3d12_swapchain *swapchain,
        unsigned int sync_interval, unsigned int flags)
{
    VkQueue vk_queue;
    VkResult vr;
    HRESULT hr;

    if (sync_interval > 4)
    {
        WARN("Invalid sync interval %u.\n", sync_interval);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (flags & ~DXGI_PRESENT_TEST)
        FIXME("Unimplemented flags %#x.\n", flags);
    if (flags & DXGI_PRESENT_TEST)
    {
        WARN("Returning S_OK for DXGI_PRESENT_TEST.\n");
        return S_OK;
    }

    if (FAILED(hr = d3d12_swapchain_set_sync_interval(swapchain, sync_interval)))
        return hr;

    if (!(vk_queue = vkd3d_acquire_vk_queue(swapchain->command_queue)))
    {
        ERR("Failed to acquire Vulkan queue.\n");
        return E_FAIL;
    }

    vr = d3d12_swapchain_queue_present(swapchain, vk_queue);
    if (vr == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vkd3d_release_vk_queue(swapchain->command_queue);

        if (!d3d12_swapchain_has_user_images(swapchain))
        {
            FIXME("Cannot recreate swapchain without user images.\n");
            return DXGI_STATUS_MODE_CHANGED;
        }

        TRACE("Recreating Vulkan swapchain.\n");

        d3d12_swapchain_destroy_buffers(swapchain, FALSE);
        if (FAILED(hr = d3d12_swapchain_recreate_vulkan_swapchain(swapchain)))
            return hr;

        if (!(vk_queue = vkd3d_acquire_vk_queue(swapchain->command_queue)))
        {
            ERR("Failed to acquire Vulkan queue.\n");
            return E_FAIL;
        }

        if ((vr = d3d12_swapchain_queue_present(swapchain, vk_queue)) < 0)
            ERR("Failed to present after recreating swapchain, vr %d.\n", vr);
    }

    vkd3d_release_vk_queue(swapchain->command_queue);

    if (vr < 0)
    {
        ERR("Failed to queue present, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    swapchain->current_buffer_index = (swapchain->current_buffer_index + 1) % swapchain->desc.BufferCount;
    vr = d3d12_swapchain_acquire_next_back_buffer(swapchain);
    if (vr == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (!d3d12_swapchain_has_user_images(swapchain))
        {
            FIXME("Cannot recreate swapchain without user images.\n");
            return DXGI_STATUS_MODE_CHANGED;
        }

        TRACE("Recreating Vulkan swapchain.\n");

        d3d12_swapchain_destroy_buffers(swapchain, FALSE);
        return d3d12_swapchain_recreate_vulkan_swapchain(swapchain);
    }
    if (vr < 0)
        ERR("Failed to acquire next Vulkan image, vr %d.\n", vr);
    return hresult_from_vk_result(vr);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_Present(IDXGISwapChain3 *iface, UINT sync_interval, UINT flags)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, sync_interval %u, flags %#x.\n", iface, sync_interval, flags);

    return d3d12_swapchain_present(swapchain, sync_interval, flags);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetBuffer(IDXGISwapChain3 *iface,
        UINT buffer_idx, REFIID iid, void **surface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, buffer_idx %u, iid %s, surface %p.\n",
            iface, buffer_idx, debugstr_guid(iid), surface);

    if (buffer_idx >= swapchain->desc.BufferCount)
    {
        WARN("Invalid buffer index %u.\n", buffer_idx);
        return DXGI_ERROR_INVALID_CALL;
    }

    assert(swapchain->buffers[buffer_idx]);
    return ID3D12Resource_QueryInterface(swapchain->buffers[buffer_idx], iid, surface);
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH d3d12_swapchain_SetFullscreenState(IDXGISwapChain3 *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc = &swapchain->fullscreen_desc;
    const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc = &swapchain->desc;
    struct wined3d_swapchain_desc wined3d_desc;
    HWND window = swapchain->window;
    HRESULT hr;

    TRACE("iface %p, fullscreen %#x, target %p.\n", iface, fullscreen, target);

    if (!fullscreen && target)
    {
        WARN("Invalid call.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    if (target)
    {
        IDXGIOutput_AddRef(target);
    }
    else if (FAILED(hr = IDXGISwapChain3_GetContainingOutput(iface, &target)))
    {
        WARN("Failed to get target output for swapchain, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = wined3d_swapchain_desc_from_dxgi(&wined3d_desc, window, swapchain_desc, fullscreen_desc)))
        goto fail;
    wined3d_mutex_lock();
    wined3d_desc.windowed = !fullscreen;
    hr = dxgi_swapchain_set_fullscreen_state(swapchain->state, &wined3d_desc, target);
    wined3d_mutex_unlock();
    if (FAILED(hr))
        goto fail;

    fullscreen_desc->Windowed = wined3d_desc.windowed;
    if (!fullscreen)
    {
        IDXGIOutput_Release(target);
        target = NULL;
    }

    if (swapchain->target)
        IDXGIOutput_Release(swapchain->target);
    swapchain->target = target;

    return S_OK;

fail:
    IDXGIOutput_Release(target);

    return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFullscreenState(IDXGISwapChain3 *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, fullscreen %p, target %p.\n", iface, fullscreen, target);

    if (fullscreen)
        *fullscreen = !swapchain->fullscreen_desc.Windowed;

    if (target && (*target = swapchain->target))
        IDXGIOutput_AddRef(*target);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDesc(IDXGISwapChain3 *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc = &swapchain->fullscreen_desc;
    const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc = &swapchain->desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    desc->BufferDesc.Width = swapchain_desc->Width;
    desc->BufferDesc.Height = swapchain_desc->Height;
    desc->BufferDesc.RefreshRate = fullscreen_desc->RefreshRate;
    desc->BufferDesc.Format = swapchain_desc->Format;
    desc->BufferDesc.ScanlineOrdering = fullscreen_desc->ScanlineOrdering;
    desc->BufferDesc.Scaling = fullscreen_desc->Scaling;
    desc->SampleDesc = swapchain_desc->SampleDesc;
    desc->BufferUsage = swapchain_desc->BufferUsage;
    desc->BufferCount = swapchain_desc->BufferCount;
    desc->OutputWindow = swapchain->window;
    desc->Windowed = fullscreen_desc->Windowed;
    desc->SwapEffect = swapchain_desc->SwapEffect;
    desc->Flags = swapchain_desc->Flags;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeBuffers(IDXGISwapChain3 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    DXGI_SWAP_CHAIN_DESC1 *desc, new_desc;
    unsigned int i;
    ULONG refcount;

    TRACE("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x.\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    for (i = 0; i < swapchain->desc.BufferCount; ++i)
    {
        ID3D12Resource_AddRef(swapchain->buffers[i]);
        if ((refcount = ID3D12Resource_Release(swapchain->buffers[i])))
        {
            WARN("Buffer %p has %u references left.\n", swapchain->buffers[i], refcount);
            return DXGI_ERROR_INVALID_CALL;
        }
    }

    desc = &swapchain->desc;
    new_desc = swapchain->desc;

    if (buffer_count)
        new_desc.BufferCount = buffer_count;
    if (!width || !height)
    {
        RECT client_rect;

        if (!GetClientRect(swapchain->window, &client_rect))
        {
            WARN("Failed to get client rect, last error %#x.\n", GetLastError());
            return DXGI_ERROR_INVALID_CALL;
        }

        if (!width)
            width = client_rect.right;
        if (!height)
            height = client_rect.bottom;
    }
    new_desc.Width = width;
    new_desc.Height = height;

    if (format)
        new_desc.Format = format;

    if (!dxgi_validate_swapchain_desc(&new_desc))
        return DXGI_ERROR_INVALID_CALL;

    if (desc->Width == new_desc.Width && desc->Height == new_desc.Height
            && desc->Format == new_desc.Format && desc->BufferCount == new_desc.BufferCount)
        return S_OK;

    d3d12_swapchain_destroy_buffers(swapchain, TRUE);
    swapchain->desc = new_desc;
    return d3d12_swapchain_recreate_vulkan_swapchain(swapchain);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeTarget(IDXGISwapChain3 *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, target_mode_desc %p.\n", iface, target_mode_desc);

    return dxgi_swapchain_resize_target((IDXGISwapChain1 *)iface, swapchain->state, target_mode_desc);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetContainingOutput(IDXGISwapChain3 *iface,
        IDXGIOutput **output)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    IUnknown *device_parent;
    IDXGIAdapter *adapter;
    HRESULT hr;

    TRACE("iface %p, output %p.\n", iface, output);

    if (swapchain->target)
    {
        IDXGIOutput_AddRef(*output = swapchain->target);
        return S_OK;
    }

    device_parent = vkd3d_get_device_parent(swapchain->device);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(device_parent, &IID_IDXGIAdapter, (void **)&adapter)))
    {
        hr = dxgi_get_output_from_window(adapter, swapchain->window, output);
        IDXGIAdapter_Release(adapter);
    }
    else
    {
        WARN("Failed to get adapter, hr %#x.\n", hr);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFrameStatistics(IDXGISwapChain3 *iface,
        DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetLastPresentCount(IDXGISwapChain3 *iface,
        UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

/* IDXGISwapChain1 methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDesc1(IDXGISwapChain3 *iface, DXGI_SWAP_CHAIN_DESC1 *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *desc = swapchain->desc;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFullscreenDesc(IDXGISwapChain3 *iface,
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *desc = swapchain->fullscreen_desc;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetHwnd(IDXGISwapChain3 *iface, HWND *hwnd)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, hwnd %p.\n", iface, hwnd);

    if (!hwnd)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    *hwnd = swapchain->window;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetCoreWindow(IDXGISwapChain3 *iface,
        REFIID iid, void **core_window)
{
    FIXME("iface %p, iid %s, core_window %p stub!\n", iface, debugstr_guid(iid), core_window);

    if (core_window)
        *core_window = NULL;

    return DXGI_ERROR_INVALID_CALL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_Present1(IDXGISwapChain3 *iface,
        UINT sync_interval, UINT flags, const DXGI_PRESENT_PARAMETERS *present_parameters)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, sync_interval %u, flags %#x, present_parameters %p.\n",
            iface, sync_interval, flags, present_parameters);

    if (present_parameters)
        FIXME("Ignored present parameters %p.\n", present_parameters);

    return d3d12_swapchain_present(swapchain, sync_interval, flags);
}

static BOOL STDMETHODCALLTYPE d3d12_swapchain_IsTemporaryMonoSupported(IDXGISwapChain3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetRestrictToOutput(IDXGISwapChain3 *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    if (!output)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *output = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetBackgroundColor(IDXGISwapChain3 *iface, const DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetBackgroundColor(IDXGISwapChain3 *iface, DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetRotation(IDXGISwapChain3 *iface, DXGI_MODE_ROTATION rotation)
{
    FIXME("iface %p, rotation %#x stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetRotation(IDXGISwapChain3 *iface, DXGI_MODE_ROTATION *rotation)
{
    FIXME("iface %p, rotation %p stub!\n", iface, rotation);

    return E_NOTIMPL;
}

/* IDXGISwapChain2 methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetSourceSize(IDXGISwapChain3 *iface, UINT width, UINT height)
{
    FIXME("iface %p, width %u, height %u stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetSourceSize(IDXGISwapChain3 *iface, UINT *width, UINT *height)
{
    FIXME("iface %p, width %p, height %p stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetMaximumFrameLatency(IDXGISwapChain3 *iface, UINT max_latency)
{
    FIXME("iface %p, max_latency %u stub!\n", iface, max_latency);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetMaximumFrameLatency(IDXGISwapChain3 *iface, UINT *max_latency)
{
    FIXME("iface %p, max_latency %p stub!\n", iface, max_latency);

    return E_NOTIMPL;
}

static HANDLE STDMETHODCALLTYPE d3d12_swapchain_GetFrameLatencyWaitableObject(IDXGISwapChain3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetMatrixTransform(IDXGISwapChain3 *iface,
        const DXGI_MATRIX_3X2_F *matrix)
{
    FIXME("iface %p, matrix %p stub!\n", iface, matrix);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetMatrixTransform(IDXGISwapChain3 *iface,
        DXGI_MATRIX_3X2_F *matrix)
{
    FIXME("iface %p, matrix %p stub!\n", iface, matrix);

    return E_NOTIMPL;
}

/* IDXGISwapChain3 methods */

static UINT STDMETHODCALLTYPE d3d12_swapchain_GetCurrentBackBufferIndex(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p.\n", iface);

    TRACE("Current back buffer index %u.\n", swapchain->current_buffer_index);
    assert(swapchain->current_buffer_index < swapchain->desc.BufferCount);
    return swapchain->current_buffer_index;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_CheckColorSpaceSupport(IDXGISwapChain3 *iface,
        DXGI_COLOR_SPACE_TYPE colour_space, UINT *colour_space_support)
{
    FIXME("iface %p, colour_space %#x, colour_space_support %p stub!\n",
            iface, colour_space, colour_space_support);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetColorSpace1(IDXGISwapChain3 *iface,
        DXGI_COLOR_SPACE_TYPE colour_space)
{
    FIXME("iface %p, colour_space %#x stub!\n", iface, colour_space);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeBuffers1(IDXGISwapChain3 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags,
        const UINT *node_mask, IUnknown * const *present_queue)
{
    FIXME("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x, "
            "node_mask %p, present_queue %p stub!\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags, node_mask, present_queue);

    return E_NOTIMPL;
}

static const struct IDXGISwapChain3Vtbl d3d12_swapchain_vtbl =
{
    /* IUnknown methods */
    d3d12_swapchain_QueryInterface,
    d3d12_swapchain_AddRef,
    d3d12_swapchain_Release,
    /* IDXGIObject methods */
    d3d12_swapchain_SetPrivateData,
    d3d12_swapchain_SetPrivateDataInterface,
    d3d12_swapchain_GetPrivateData,
    d3d12_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    d3d12_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    d3d12_swapchain_Present,
    d3d12_swapchain_GetBuffer,
    d3d12_swapchain_SetFullscreenState,
    d3d12_swapchain_GetFullscreenState,
    d3d12_swapchain_GetDesc,
    d3d12_swapchain_ResizeBuffers,
    d3d12_swapchain_ResizeTarget,
    d3d12_swapchain_GetContainingOutput,
    d3d12_swapchain_GetFrameStatistics,
    d3d12_swapchain_GetLastPresentCount,
    /* IDXGISwapChain1 methods */
    d3d12_swapchain_GetDesc1,
    d3d12_swapchain_GetFullscreenDesc,
    d3d12_swapchain_GetHwnd,
    d3d12_swapchain_GetCoreWindow,
    d3d12_swapchain_Present1,
    d3d12_swapchain_IsTemporaryMonoSupported,
    d3d12_swapchain_GetRestrictToOutput,
    d3d12_swapchain_SetBackgroundColor,
    d3d12_swapchain_GetBackgroundColor,
    d3d12_swapchain_SetRotation,
    d3d12_swapchain_GetRotation,
    /* IDXGISwapChain2 methods */
    d3d12_swapchain_SetSourceSize,
    d3d12_swapchain_GetSourceSize,
    d3d12_swapchain_SetMaximumFrameLatency,
    d3d12_swapchain_GetMaximumFrameLatency,
    d3d12_swapchain_GetFrameLatencyWaitableObject,
    d3d12_swapchain_SetMatrixTransform,
    d3d12_swapchain_GetMatrixTransform,
    /* IDXGISwapChain3 methods */
    d3d12_swapchain_GetCurrentBackBufferIndex,
    d3d12_swapchain_CheckColorSpaceSupport,
    d3d12_swapchain_SetColorSpace1,
    d3d12_swapchain_ResizeBuffers1,
};

static BOOL load_vkd3d_functions(void *vkd3d_handle)
{
#define LOAD_FUNCPTR(f) if (!(f = get_library_proc(vkd3d_handle, #f))) return FALSE;
    LOAD_FUNCPTR(vkd3d_acquire_vk_queue)
    LOAD_FUNCPTR(vkd3d_create_image_resource)
    LOAD_FUNCPTR(vkd3d_get_device_parent)
    LOAD_FUNCPTR(vkd3d_get_vk_device)
    LOAD_FUNCPTR(vkd3d_get_vk_format)
    LOAD_FUNCPTR(vkd3d_get_vk_physical_device)
    LOAD_FUNCPTR(vkd3d_get_vk_queue_family_index)
    LOAD_FUNCPTR(vkd3d_instance_from_device)
    LOAD_FUNCPTR(vkd3d_instance_get_vk_instance)
    LOAD_FUNCPTR(vkd3d_release_vk_queue)
    LOAD_FUNCPTR(vkd3d_resource_decref)
    LOAD_FUNCPTR(vkd3d_resource_incref)
#undef LOAD_FUNCPTR

    return TRUE;
}

static void *vkd3d_handle;

static BOOL WINAPI init_vkd3d_once(INIT_ONCE *once, void *param, void **context)
{
    TRACE("Loading vkd3d %s.\n", SONAME_LIBVKD3D);

    if (!(vkd3d_handle = load_library(SONAME_LIBVKD3D)))
        return FALSE;

    if (!load_vkd3d_functions(vkd3d_handle))
    {
        ERR("Failed to load vkd3d functions.\n");
        close_library(vkd3d_handle);
        vkd3d_handle = NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL init_vkd3d(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, init_vkd3d_once, NULL, NULL);
    return !!vkd3d_handle;
}

static BOOL init_vk_funcs(struct dxgi_vk_funcs *dxgi, VkInstance vk_instance, VkDevice vk_device)
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

    dxgi->vulkan_module = NULL;

    if (!(vkGetInstanceProcAddr = load_vulkan(&dxgi->vulkan_module)))
    {
        ERR_(winediag)("Failed to load Vulkan.\n");
        return FALSE;
    }

    vkGetDeviceProcAddr = (void *)vkGetInstanceProcAddr(vk_instance, "vkGetDeviceProcAddr");

#define LOAD_INSTANCE_PFN(name) \
    if (!(dxgi->p_##name = (void *)vkGetInstanceProcAddr(vk_instance, #name))) \
    { \
        ERR("Failed to get instance proc "#name".\n"); \
        close_library(dxgi->vulkan_module); \
        return FALSE; \
    }
    LOAD_INSTANCE_PFN(vkCreateWin32SurfaceKHR)
    LOAD_INSTANCE_PFN(vkDestroySurfaceKHR)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceMemoryProperties)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceFormatsKHR)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceSurfacePresentModesKHR)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceSupportKHR)
    LOAD_INSTANCE_PFN(vkGetPhysicalDeviceWin32PresentationSupportKHR)
#undef LOAD_INSTANCE_PFN

#define LOAD_DEVICE_PFN(name) \
    if (!(dxgi->p_##name = (void *)vkGetDeviceProcAddr(vk_device, #name))) \
    { \
        ERR("Failed to get device proc "#name".\n"); \
        close_library(dxgi->vulkan_module); \
        return FALSE; \
    }
    LOAD_DEVICE_PFN(vkAcquireNextImageKHR)
    LOAD_DEVICE_PFN(vkAllocateCommandBuffers)
    LOAD_DEVICE_PFN(vkAllocateMemory)
    LOAD_DEVICE_PFN(vkBeginCommandBuffer)
    LOAD_DEVICE_PFN(vkBindImageMemory)
    LOAD_DEVICE_PFN(vkCmdBlitImage)
    LOAD_DEVICE_PFN(vkCmdPipelineBarrier)
    LOAD_DEVICE_PFN(vkCreateCommandPool)
    LOAD_DEVICE_PFN(vkCreateFence)
    LOAD_DEVICE_PFN(vkCreateImage)
    LOAD_DEVICE_PFN(vkCreateSemaphore)
    LOAD_DEVICE_PFN(vkCreateSwapchainKHR)
    LOAD_DEVICE_PFN(vkDestroyCommandPool)
    LOAD_DEVICE_PFN(vkDestroyFence)
    LOAD_DEVICE_PFN(vkDestroyImage)
    LOAD_DEVICE_PFN(vkDestroySemaphore)
    LOAD_DEVICE_PFN(vkDestroySwapchainKHR)
    LOAD_DEVICE_PFN(vkEndCommandBuffer)
    LOAD_DEVICE_PFN(vkFreeMemory)
    LOAD_DEVICE_PFN(vkResetCommandBuffer)
    LOAD_DEVICE_PFN(vkGetImageMemoryRequirements)
    LOAD_DEVICE_PFN(vkGetSwapchainImagesKHR)
    LOAD_DEVICE_PFN(vkQueuePresentKHR)
    LOAD_DEVICE_PFN(vkQueueSubmit)
    LOAD_DEVICE_PFN(vkQueueWaitIdle)
    LOAD_DEVICE_PFN(vkResetFences)
    LOAD_DEVICE_PFN(vkWaitForFences)
#undef LOAD_DEVICE_PFN

    return TRUE;
}

static HRESULT d3d12_swapchain_init(struct d3d12_swapchain *swapchain, IWineDXGIFactory *factory,
        ID3D12Device *device, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    struct wined3d_swapchain_desc wined3d_desc;
    VkWin32SurfaceCreateInfoKHR surface_desc;
    VkPhysicalDevice vk_physical_device;
    struct dxgi_adapter *dxgi_adapter;
    VkFenceCreateInfo fence_desc;
    uint32_t queue_family_index;
    VkSurfaceKHR vk_surface;
    IUnknown *device_parent;
    VkInstance vk_instance;
    IDXGIAdapter *adapter;
    VkBool32 supported;
    VkDevice vk_device;
    VkFence vk_fence;
    VkResult vr;
    HRESULT hr;

    if (window == GetDesktopWindow())
    {
        WARN("D3D12 swapchain cannot be created on desktop window.\n");
        return E_ACCESSDENIED;
    }

    swapchain->IDXGISwapChain3_iface.lpVtbl = &d3d12_swapchain_vtbl;
    swapchain->refcount = 1;

    swapchain->window = window;
    swapchain->desc = *swapchain_desc;
    swapchain->fullscreen_desc = *fullscreen_desc;

    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR;

    switch (swapchain_desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
            FIXME("Ignoring swap effect %#x.\n", swapchain_desc->SwapEffect);
            break;
        default:
            WARN("Invalid swap effect %#x.\n", swapchain_desc->SwapEffect);
            return DXGI_ERROR_INVALID_CALL;
    }

    if (!init_vkd3d())
    {
        ERR_(winediag)("libvkd3d could not be loaded.\n");
        return DXGI_ERROR_UNSUPPORTED;
    }

    device_parent = vkd3d_get_device_parent(device);
    if (FAILED(hr = IUnknown_QueryInterface(device_parent, &IID_IDXGIAdapter, (void **)&adapter)))
        return hr;
    dxgi_adapter = unsafe_impl_from_IDXGIAdapter(adapter);
    IDXGIAdapter_Release(adapter);
    if (FAILED(hr = wined3d_swapchain_desc_from_dxgi(&wined3d_desc, window, swapchain_desc, fullscreen_desc)))
        return hr;
    if (FAILED(hr = wined3d_swapchain_state_create(&wined3d_desc, window,
            dxgi_adapter->factory->wined3d, dxgi_adapter->ordinal, &swapchain->state)))
        return hr;

    if (swapchain_desc->BufferUsage && swapchain_desc->BufferUsage != DXGI_USAGE_RENDER_TARGET_OUTPUT)
        FIXME("Ignoring buffer usage %#x.\n", swapchain_desc->BufferUsage);
    if (swapchain_desc->Scaling != DXGI_SCALING_STRETCH)
        FIXME("Ignoring scaling %#x.\n", swapchain_desc->Scaling);
    if (swapchain_desc->AlphaMode && swapchain_desc->AlphaMode != DXGI_ALPHA_MODE_IGNORE)
        FIXME("Ignoring alpha mode %#x.\n", swapchain_desc->AlphaMode);
    if (swapchain_desc->Flags)
        FIXME("Ignoring swapchain flags %#x.\n", swapchain_desc->Flags);

    FIXME("Ignoring refresh rate.\n");
    if (fullscreen_desc->ScanlineOrdering)
        FIXME("Unhandled scanline ordering %#x.\n", fullscreen_desc->ScanlineOrdering);
    if (fullscreen_desc->Scaling)
        FIXME("Unhandled mode scaling %#x.\n", fullscreen_desc->Scaling);
    if (!fullscreen_desc->Windowed)
        FIXME("Fullscreen not supported yet.\n");

    vk_instance = vkd3d_instance_get_vk_instance(vkd3d_instance_from_device(device));
    vk_physical_device = vkd3d_get_vk_physical_device(device);
    vk_device = vkd3d_get_vk_device(device);

    swapchain->vk_instance = vk_instance;
    swapchain->vk_device = vk_device;
    swapchain->vk_physical_device = vk_physical_device;

    if (!init_vk_funcs(&swapchain->vk_funcs, vk_instance, vk_device))
    {
        wined3d_swapchain_state_destroy(swapchain->state);
        return E_FAIL;
    }

    wined3d_private_store_init(&swapchain->private_store);

    surface_desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_desc.pNext = NULL;
    surface_desc.flags = 0;
    surface_desc.hinstance = GetModuleHandleA("dxgi.dll");
    surface_desc.hwnd = window;
    if ((vr = vk_funcs->p_vkCreateWin32SurfaceKHR(vk_instance, &surface_desc, NULL, &vk_surface)) < 0)
    {
        WARN("Failed to create Vulkan surface, vr %d.\n", vr);
        d3d12_swapchain_destroy(swapchain);
        return hresult_from_vk_result(vr);
    }
    swapchain->vk_surface = vk_surface;

    queue_family_index = vkd3d_get_vk_queue_family_index(queue);
    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device,
            queue_family_index, vk_surface, &supported)) < 0 || !supported)
    {
        FIXME("Queue family does not support presentation, vr %d.\n", vr);
        d3d12_swapchain_destroy(swapchain);
        return DXGI_ERROR_UNSUPPORTED;
    }

    ID3D12CommandQueue_AddRef(swapchain->command_queue = queue);
    ID3D12Device_AddRef(swapchain->device = device);

    if (FAILED(hr = d3d12_swapchain_create_vulkan_swapchain(swapchain)))
    {
        d3d12_swapchain_destroy(swapchain);
        return hr;
    }

    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    fence_desc.flags = 0;
    if ((vr = vk_funcs->p_vkCreateFence(vk_device, &fence_desc, NULL, &vk_fence)) < 0)
    {
        WARN("Failed to create Vulkan fence, vr %d.\n", vr);
        d3d12_swapchain_destroy(swapchain);
        return hresult_from_vk_result(vr);
    }
    swapchain->vk_fence = vk_fence;

    swapchain->current_buffer_index = 0;
    if ((vr = d3d12_swapchain_acquire_next_back_buffer(swapchain)) < 0)
    {
        d3d12_swapchain_destroy(swapchain);
        return hresult_from_vk_result(vr);
    }

    IWineDXGIFactory_AddRef(swapchain->factory = factory);

    return S_OK;
}

HRESULT d3d12_swapchain_create(IWineDXGIFactory *factory, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc,
        IDXGISwapChain1 **swapchain)
{
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC default_fullscreen_desc;
    struct d3d12_swapchain *object;
    ID3D12Device *device;
    HRESULT hr;

    if (swapchain_desc->Format == DXGI_FORMAT_UNKNOWN)
        return DXGI_ERROR_INVALID_CALL;

    if (!fullscreen_desc)
    {
        memset(&default_fullscreen_desc, 0, sizeof(default_fullscreen_desc));
        default_fullscreen_desc.Windowed = TRUE;
        fullscreen_desc = &default_fullscreen_desc;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&device)))
    {
        ERR("Failed to get D3D12 device, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    hr = d3d12_swapchain_init(object, factory, device, queue, window, swapchain_desc, fullscreen_desc);
    ID3D12Device_Release(device);
    if (FAILED(hr))
    {
        heap_free(object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);

    *swapchain = (IDXGISwapChain1 *)&object->IDXGISwapChain3_iface;

    return S_OK;
}

#else

HRESULT d3d12_swapchain_create(IWineDXGIFactory *factory, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc,
        IDXGISwapChain1 **swapchain)
{
    ERR_(winediag)("Wine was built without Direct3D 12 support.\n");
    return DXGI_ERROR_UNSUPPORTED;
}

#endif  /* SONAME_LIBVKD3D */
