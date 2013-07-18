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

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static inline struct dxgi_factory *impl_from_IWineDXGIFactory(IWineDXGIFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_factory, IWineDXGIFactory_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IWineDXGIFactory *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIFactory)
            || IsEqualGUID(riid, &IID_IWineDXGIFactory))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = impl_from_IWineDXGIFactory(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = impl_from_IWineDXGIFactory(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        UINT i;

        for (i = 0; i < This->adapter_count; ++i)
        {
            IDXGIAdapter_Release(This->adapters[i]);
        }
        HeapFree(GetProcessHeap(), 0, This->adapters);

        EnterCriticalSection(&dxgi_cs);
        wined3d_decref(This->wined3d);
        LeaveCriticalSection(&dxgi_cs);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IWineDXGIFactory *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IWineDXGIFactory *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IWineDXGIFactory *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IWineDXGIFactory *iface, REFIID riid, void **parent)
{
    WARN("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    *parent = NULL;

    return E_NOINTERFACE;
}

/* IDXGIFactory methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IWineDXGIFactory *iface,
        UINT adapter_idx, IDXGIAdapter **adapter)
{
    struct dxgi_factory *This = impl_from_IWineDXGIFactory(iface);

    TRACE("iface %p, adapter_idx %u, adapter %p\n", iface, adapter_idx, adapter);

    if (!adapter) return DXGI_ERROR_INVALID_CALL;

    if (adapter_idx >= This->adapter_count)
    {
        *adapter = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    *adapter = This->adapters[adapter_idx];
    IDXGIAdapter_AddRef(*adapter);

    TRACE("Returning adapter %p\n", *adapter);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IWineDXGIFactory *iface, HWND window, UINT flags)
{
    FIXME("iface %p, window %p, flags %#x stub!\n\n", iface, window, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IWineDXGIFactory *iface, HWND *window)
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

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IWineDXGIFactory *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_desc wined3d_desc;
    struct wined3d_device *wined3d_device;
    IWineDXGIDevice *dxgi_device;
    UINT count;
    HRESULT hr;

    FIXME("iface %p, device %p, desc %p, swapchain %p partial stub!\n", iface, device, desc, swapchain);

    hr = IUnknown_QueryInterface(device, &IID_IWineDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        ERR("This is not the device we're looking for\n");
        return hr;
    }

    wined3d_device = IWineDXGIDevice_get_wined3d_device(dxgi_device);
    IWineDXGIDevice_Release(dxgi_device);

    count = wined3d_device_get_swapchain_count(wined3d_device);
    if (count)
    {
        FIXME("Only a single swapchain supported.\n");
        wined3d_device_decref(wined3d_device);
        return E_FAIL;
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

    hr = wined3d_device_init_3d(wined3d_device, &wined3d_desc);
    if (FAILED(hr))
    {
        WARN("Failed to initialize 3D, returning %#x\n", hr);
        wined3d_device_decref(wined3d_device);
        return hr;
    }

    wined3d_swapchain = wined3d_device_get_swapchain(wined3d_device, 0);
    wined3d_device_decref(wined3d_device);
    if (!wined3d_swapchain)
    {
        WARN("Failed to get swapchain.\n");
        return E_FAIL;
    }

    *swapchain = wined3d_swapchain_get_parent(wined3d_swapchain);

    /* FIXME? The swapchain is created with refcount 1 by the wined3d device,
     * but the wined3d device can't hold a real reference. */

    TRACE("Created IDXGISwapChain %p\n", *swapchain);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IWineDXGIFactory *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    FIXME("iface %p, swrast %p, adapter %p stub!\n", iface, swrast, adapter);

    return E_NOTIMPL;
}

/* IWineDXGIFactory methods */

static struct wined3d * STDMETHODCALLTYPE dxgi_factory_get_wined3d(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = impl_from_IWineDXGIFactory(iface);

    TRACE("iface %p\n", iface);

    EnterCriticalSection(&dxgi_cs);
    wined3d_incref(This->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    return This->wined3d;
}

static const struct IWineDXGIFactoryVtbl dxgi_factory_vtbl =
{
    /* IUnknown methods */
    dxgi_factory_QueryInterface,
    dxgi_factory_AddRef,
    dxgi_factory_Release,
    /* IDXGIObject methods */
    dxgi_factory_SetPrivateData,
    dxgi_factory_SetPrivateDataInterface,
    dxgi_factory_GetPrivateData,
    dxgi_factory_GetParent,
    /* IDXGIFactory methods */
    dxgi_factory_EnumAdapters,
    dxgi_factory_MakeWindowAssociation,
    dxgi_factory_GetWindowAssociation,
    dxgi_factory_CreateSwapChain,
    dxgi_factory_CreateSoftwareAdapter,
    /* IWineDXGIFactory methods */
    dxgi_factory_get_wined3d,
};

HRESULT dxgi_factory_init(struct dxgi_factory *factory)
{
    HRESULT hr;
    UINT i;

    factory->IWineDXGIFactory_iface.lpVtbl = &dxgi_factory_vtbl;
    factory->refcount = 1;

    EnterCriticalSection(&dxgi_cs);
    factory->wined3d = wined3d_create(10, 0);
    if (!factory->wined3d)
    {
        LeaveCriticalSection(&dxgi_cs);
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
                IDXGIAdapter_Release(factory->adapters[j]);
            }
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        hr = dxgi_adapter_init(adapter, &factory->IWineDXGIFactory_iface, i);
        if (FAILED(hr))
        {
            UINT j;

            ERR("Failed to initialize adapter, hr %#x.\n", hr);

            HeapFree(GetProcessHeap(), 0, adapter);
            for (j = 0; j < i; ++j)
            {
                IDXGIAdapter_Release(factory->adapters[j]);
            }
            goto fail;
        }

        factory->adapters[i] = (IDXGIAdapter *)adapter;
    }

    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, factory->adapters);
    EnterCriticalSection(&dxgi_cs);
    wined3d_decref(factory->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    return hr;
}
