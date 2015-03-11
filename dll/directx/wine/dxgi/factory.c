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

#include <assert.h>

static inline struct dxgi_factory *impl_from_IDXGIFactory1(IDXGIFactory1 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_factory, IDXGIFactory1_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IDXGIFactory1 *iface, REFIID iid, void **out)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if ((factory->extended && IsEqualGUID(iid, &IID_IDXGIFactory1))
            || IsEqualGUID(iid, &IID_IDXGIFactory)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IDXGIFactory1 *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IDXGIFactory1 *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        UINT i;

        if (factory->device_window)
            DestroyWindow(factory->device_window);
        for (i = 0; i < factory->adapter_count; ++i)
        {
            IDXGIAdapter1_Release(factory->adapters[i]);
        }
        HeapFree(GetProcessHeap(), 0, factory->adapters);

        EnterCriticalSection(&dxgi_cs);
        wined3d_decref(factory->wined3d);
        LeaveCriticalSection(&dxgi_cs);
        wined3d_private_store_cleanup(&factory->private_store);
        HeapFree(GetProcessHeap(), 0, factory);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IDXGIFactory1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&factory->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IDXGIFactory1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&factory->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IDXGIFactory1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&factory->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IDXGIFactory1 *iface, REFIID iid, void **parent)
{
    WARN("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    *parent = NULL;

    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters1(IDXGIFactory1 *iface,
        UINT adapter_idx, IDXGIAdapter1 **adapter)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, adapter_idx %u, adapter %p.\n", iface, adapter_idx, adapter);

    if (!adapter)
        return DXGI_ERROR_INVALID_CALL;

    if (adapter_idx >= factory->adapter_count)
    {
        *adapter = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    *adapter = (IDXGIAdapter1 *)factory->adapters[adapter_idx];
    IDXGIAdapter1_AddRef(*adapter);

    TRACE("Returning adapter %p.\n", *adapter);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IDXGIFactory1 *iface,
        UINT adapter_idx, IDXGIAdapter **adapter)
{
    TRACE("iface %p, adapter_idx %u, adapter %p.\n", iface, adapter_idx, adapter);

    return dxgi_factory_EnumAdapters1(iface, adapter_idx, (IDXGIAdapter1 **)adapter);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IDXGIFactory1 *iface, HWND window, UINT flags)
{
    FIXME("iface %p, window %p, flags %#x stub!\n", iface, window, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IDXGIFactory1 *iface, HWND *window)
{
    FIXME("iface %p, window %p stub!\n", iface, window);

    return E_NOTIMPL;
}

static UINT dxgi_rational_to_uint(const DXGI_RATIONAL *rational)
{
    if (rational->Denominator)
        return rational->Numerator / rational->Denominator;
    else
        return rational->Numerator;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IDXGIFactory1 *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_desc wined3d_desc;
    IWineDXGIDevice *dxgi_device;
    HRESULT hr;

    FIXME("iface %p, device %p, desc %p, swapchain %p partial stub!\n", iface, device, desc, swapchain);

    hr = IUnknown_QueryInterface(device, &IID_IWineDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        ERR("This is not the device we're looking for\n");
        return hr;
    }

    if (!desc->OutputWindow)
    {
        FIXME("No output window, should use factory output window\n");
    }

    FIXME("Ignoring SwapEffect and Flags\n");

    wined3d_desc.backbuffer_width = desc->BufferDesc.Width;
    wined3d_desc.backbuffer_height = desc->BufferDesc.Height;
    wined3d_desc.backbuffer_format = wined3dformat_from_dxgi_format(desc->BufferDesc.Format);
    wined3d_desc.backbuffer_count = desc->BufferCount;
    if (desc->SampleDesc.Count > 1)
    {
        wined3d_desc.multisample_type = desc->SampleDesc.Count;
        wined3d_desc.multisample_quality = desc->SampleDesc.Quality;
    }
    else
    {
        wined3d_desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
        wined3d_desc.multisample_quality = 0;
    }
    wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
    wined3d_desc.device_window = desc->OutputWindow;
    wined3d_desc.windowed = desc->Windowed;
    wined3d_desc.enable_auto_depth_stencil = FALSE;
    wined3d_desc.auto_depth_stencil_format = 0;
    wined3d_desc.flags = 0; /* WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL? */
    wined3d_desc.refresh_rate = dxgi_rational_to_uint(&desc->BufferDesc.RefreshRate);
    wined3d_desc.swap_interval = WINED3DPRESENT_INTERVAL_DEFAULT;

    hr = IWineDXGIDevice_create_swapchain(dxgi_device, &wined3d_desc, &wined3d_swapchain);
    IWineDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create swapchain, hr %#x.\n", hr);
        return hr;
    }

    *swapchain = wined3d_swapchain_get_parent(wined3d_swapchain);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IDXGIFactory1 *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    FIXME("iface %p, swrast %p, adapter %p stub!\n", iface, swrast, adapter);

    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxgi_factory_IsCurrent(IDXGIFactory1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return TRUE;
}

static const struct IDXGIFactory1Vtbl dxgi_factory_vtbl =
{
    dxgi_factory_QueryInterface,
    dxgi_factory_AddRef,
    dxgi_factory_Release,
    dxgi_factory_SetPrivateData,
    dxgi_factory_SetPrivateDataInterface,
    dxgi_factory_GetPrivateData,
    dxgi_factory_GetParent,
    dxgi_factory_EnumAdapters,
    dxgi_factory_MakeWindowAssociation,
    dxgi_factory_GetWindowAssociation,
    dxgi_factory_CreateSwapChain,
    dxgi_factory_CreateSoftwareAdapter,
    dxgi_factory_EnumAdapters1,
    dxgi_factory_IsCurrent,
};

struct dxgi_factory *unsafe_impl_from_IDXGIFactory1(IDXGIFactory1 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &dxgi_factory_vtbl);
    return CONTAINING_RECORD(iface, struct dxgi_factory, IDXGIFactory1_iface);
}

static HRESULT dxgi_factory_init(struct dxgi_factory *factory, BOOL extended)
{
    HRESULT hr;
    UINT i;

    factory->IDXGIFactory1_iface.lpVtbl = &dxgi_factory_vtbl;
    factory->refcount = 1;
    wined3d_private_store_init(&factory->private_store);

    EnterCriticalSection(&dxgi_cs);
    factory->wined3d = wined3d_create(0);
    if (!factory->wined3d)
    {
        LeaveCriticalSection(&dxgi_cs);
        wined3d_private_store_cleanup(&factory->private_store);
        return DXGI_ERROR_UNSUPPORTED;
    }

    factory->adapter_count = wined3d_get_adapter_count(factory->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    factory->adapters = HeapAlloc(GetProcessHeap(), 0, factory->adapter_count * sizeof(*factory->adapters));
    if (!factory->adapters)
    {
        ERR("Failed to allocate DXGI adapter array memory.\n");
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    for (i = 0; i < factory->adapter_count; ++i)
    {
        struct dxgi_adapter *adapter = HeapAlloc(GetProcessHeap(), 0, sizeof(*adapter));
        if (!adapter)
        {
            UINT j;

            ERR("Failed to allocate DXGI adapter memory.\n");

            for (j = 0; j < i; ++j)
            {
                IDXGIAdapter1_Release(factory->adapters[j]);
            }
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        if (FAILED(hr = dxgi_adapter_init(adapter, factory, i)))
        {
            UINT j;

            ERR("Failed to initialize adapter, hr %#x.\n", hr);

            HeapFree(GetProcessHeap(), 0, adapter);
            for (j = 0; j < i; ++j)
            {
                IDXGIAdapter1_Release(factory->adapters[j]);
            }
            goto fail;
        }

        factory->adapters[i] = &adapter->IDXGIAdapter1_iface;
    }

    factory->extended = extended;

    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, factory->adapters);
    EnterCriticalSection(&dxgi_cs);
    wined3d_decref(factory->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    wined3d_private_store_cleanup(&factory->private_store);
    return hr;
}

HRESULT dxgi_factory_create(REFIID riid, void **factory, BOOL extended)
{
    struct dxgi_factory *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = dxgi_factory_init(object, extended)))
    {
        WARN("Failed to initialize factory, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created factory %p.\n", object);

    hr = IDXGIFactory1_QueryInterface(&object->IDXGIFactory1_iface, riid, factory);
    IDXGIFactory1_Release(&object->IDXGIFactory1_iface);

    return hr;
}

HWND dxgi_factory_get_device_window(struct dxgi_factory *factory)
{
    EnterCriticalSection(&dxgi_cs);

    if (!factory->device_window)
    {
        if (!(factory->device_window = CreateWindowA("static", "DXGI device window",
                WS_DISABLED, 0, 0, 0, 0, NULL, NULL, NULL, NULL)))
        {
            LeaveCriticalSection(&dxgi_cs);
            ERR("Failed to create a window.\n");
            return NULL;
        }
        TRACE("Created device window %p for factory %p.\n", factory->device_window, factory);
    }

    LeaveCriticalSection(&dxgi_cs);

    return factory->device_window;
}
