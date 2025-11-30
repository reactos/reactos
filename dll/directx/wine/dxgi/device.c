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

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static void STDMETHODCALLTYPE dxgi_null_wined3d_object_destroyed(void *parent) {}

static const struct wined3d_parent_ops dxgi_null_wined3d_parent_ops =
{
    dxgi_null_wined3d_object_destroyed,
};

static inline struct dxgi_device *impl_from_IWineDXGIDevice(IWineDXGIDevice *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_device, IWineDXGIDevice_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryInterface(IWineDXGIDevice *iface, REFIID riid, void **object)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDevice)
            || IsEqualGUID(riid, &IID_IDXGIDevice1)
            || IsEqualGUID(riid, &IID_IDXGIDevice2)
            || IsEqualGUID(riid, &IID_IDXGIDevice3)
            || IsEqualGUID(riid, &IID_IWineDXGIDevice))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IWineDXGISwapChainFactory))
    {
        IUnknown_AddRef(iface);
        *object = &device->IWineDXGISwapChainFactory_iface;
        return S_OK;
    }

    if (device->child_layer)
    {
        TRACE("Forwarding to child layer %p.\n", device->child_layer);
        return IUnknown_QueryInterface(device->child_layer, riid, object);
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_device_AddRef(IWineDXGIDevice *iface)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %lu\n", device, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_device_Release(IWineDXGIDevice *iface)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %lu.\n", device, refcount);

    if (!refcount)
    {
        if (device->child_layer)
            IUnknown_Release(device->child_layer);
        wined3d_mutex_lock();
        wined3d_swapchain_decref(device->implicit_swapchain);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        IWineDXGIAdapter_Release(device->adapter);
        wined3d_private_store_cleanup(&device->private_store);
        free(device);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateDataInterface(IWineDXGIDevice *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&device->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetParent(IWineDXGIDevice *iface, REFIID riid, void **parent)
{
    IDXGIAdapter *adapter;
    HRESULT hr;

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    hr = IWineDXGIDevice_GetAdapter(iface, &adapter);
    if (FAILED(hr))
    {
        ERR("Failed to get adapter, hr %#lx.\n", hr);
        return hr;
    }

    hr = IDXGIAdapter_QueryInterface(adapter, riid, parent);
    IDXGIAdapter_Release(adapter);

    return hr;
}

/* IDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_GetAdapter(IWineDXGIDevice *iface, IDXGIAdapter **adapter)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, adapter %p.\n", iface, adapter);

    *adapter = (IDXGIAdapter *)device->adapter;
    IDXGIAdapter_AddRef(*adapter);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_CreateSurface(IWineDXGIDevice *iface,
        const DXGI_SURFACE_DESC *desc, UINT surface_count, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IDXGISurface **surface)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);
    struct wined3d_resource_desc surface_desc;
    IWineDXGIDeviceParent *dxgi_device_parent;
    HRESULT hr;
    UINT i;
    UINT j;

    TRACE("iface %p, desc %p, surface_count %u, usage %#x, shared_resource %p, surface %p.\n",
            iface, desc, surface_count, usage, shared_resource, surface);

    hr = IWineDXGIDevice_QueryInterface(iface, &IID_IWineDXGIDeviceParent, (void **)&dxgi_device_parent);
    if (FAILED(hr))
    {
        ERR("Device should implement IWineDXGIDeviceParent.\n");
        return E_FAIL;
    }

    surface_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    surface_desc.format = wined3dformat_from_dxgi_format(desc->Format);
    wined3d_sample_desc_from_dxgi(&surface_desc.multisample_type,
            &surface_desc.multisample_quality, &desc->SampleDesc);
    surface_desc.bind_flags = wined3d_bind_flags_from_dxgi_usage(usage);
    surface_desc.usage = 0;
    surface_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    surface_desc.width = desc->Width;
    surface_desc.height = desc->Height;
    surface_desc.depth = 1;
    surface_desc.size = 0;

    wined3d_mutex_lock();
    memset(surface, 0, surface_count * sizeof(*surface));
    for (i = 0; i < surface_count; ++i)
    {
        struct wined3d_texture *wined3d_texture;

        if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &surface_desc,
                1, 1, 0, NULL, NULL, &dxgi_null_wined3d_parent_ops, &wined3d_texture)))
        {
            ERR("Failed to create wined3d texture, hr %#lx.\n", hr);
            goto fail;
        }

        if (FAILED(hr = IWineDXGIDeviceParent_register_swapchain_texture(
                dxgi_device_parent, wined3d_texture, 0, &surface[i])))
        {
            wined3d_texture_decref(wined3d_texture);
            ERR("Failed to create parent swapchain texture, hr %#lx.\n", hr);
            goto fail;
        }
        wined3d_texture_decref(wined3d_texture);

        TRACE("Created IDXGISurface %p (%u/%u).\n", surface[i], i + 1, surface_count);
    }
    wined3d_mutex_unlock();
    IWineDXGIDeviceParent_Release(dxgi_device_parent);

    return S_OK;

fail:
    wined3d_mutex_unlock();
    for (j = 0; j < i; ++j)
    {
        IDXGISurface_Release(surface[i]);
    }
    IWineDXGIDeviceParent_Release(dxgi_device_parent);
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryResourceResidency(IWineDXGIDevice *iface,
        IUnknown *const *resources, DXGI_RESIDENCY *residency, UINT resource_count)
{
    FIXME("iface %p, resources %p, residency %p, resource_count %u stub!\n",
            iface, resources, residency, resource_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetGPUThreadPriority(IWineDXGIDevice *iface, INT priority)
{
    FIXME("iface %p, priority %d stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetGPUThreadPriority(IWineDXGIDevice *iface, INT *priority)
{
    FIXME("iface %p, priority %p stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetMaximumFrameLatency(IWineDXGIDevice *iface, UINT max_latency)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, max_latency %u.\n", iface, max_latency);

    if (max_latency > DXGI_FRAME_LATENCY_MAX)
        return DXGI_ERROR_INVALID_CALL;

    wined3d_mutex_lock();
    wined3d_device_set_max_frame_latency(device->wined3d_device, max_latency);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetMaximumFrameLatency(IWineDXGIDevice *iface, UINT *max_latency)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, max_latency %p.\n", iface, max_latency);

    if (!max_latency)
        return DXGI_ERROR_INVALID_CALL;

    wined3d_mutex_lock();
    *max_latency = wined3d_device_get_max_frame_latency(device->wined3d_device);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_OfferResources(IWineDXGIDevice *iface, UINT resource_count,
        IDXGIResource * const *resources, DXGI_OFFER_RESOURCE_PRIORITY priority)
{
    FIXME("iface %p, resource_count %u, resources %p, priority %u stub!\n", iface, resource_count,
        resources, priority);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_ReclaimResources(IWineDXGIDevice *iface, UINT resource_count,
        IDXGIResource * const *resources, BOOL *discarded)
{
    FIXME("iface %p, resource_count %u, resources %p, discarded %p stub!\n", iface, resource_count,
        resources, discarded);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_EnqueueSetEvent(IWineDXGIDevice *iface, HANDLE event)
{
    FIXME("iface %p, event %p stub!\n", iface, event);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_device_Trim(IWineDXGIDevice *iface)
{
    FIXME("iface %p stub!\n", iface);
}

/* IWineDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_create_resource(IWineDXGIDevice *iface,
        struct wined3d_resource *wined3d_resource, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IUnknown *outer, BOOL needs_surface, void **resource)
{
    struct dxgi_resource *object;
    HRESULT hr;

    TRACE("iface %p, wined3d_resource %p, usage %#x, shared_resource %p, outer %p, needs_surface %d, "
            "resource %p.\n", iface, wined3d_resource, usage, shared_resource, outer, needs_surface,
            resource);

    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate DXGI resource object memory.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = dxgi_resource_init(object, (IDXGIDevice *)iface, outer, needs_surface, wined3d_resource)))
    {
        WARN("Failed to initialize resource, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created resource %p.\n", object);
    *resource = outer ? &object->IUnknown_iface : (IUnknown *)&object->IDXGIResource_iface;

    return S_OK;
}

static const struct IWineDXGIDeviceVtbl dxgi_device_vtbl =
{
    /* IUnknown methods */
    dxgi_device_QueryInterface,
    dxgi_device_AddRef,
    dxgi_device_Release,
    /* IDXGIObject methods */
    dxgi_device_SetPrivateData,
    dxgi_device_SetPrivateDataInterface,
    dxgi_device_GetPrivateData,
    dxgi_device_GetParent,
    /* IDXGIDevice methods */
    dxgi_device_GetAdapter,
    dxgi_device_CreateSurface,
    dxgi_device_QueryResourceResidency,
    dxgi_device_SetGPUThreadPriority,
    dxgi_device_GetGPUThreadPriority,
    /* IDXGIDevice1 methods */
    dxgi_device_SetMaximumFrameLatency,
    dxgi_device_GetMaximumFrameLatency,
    /* IDXGIDevice2 methods */
    dxgi_device_OfferResources,
    dxgi_device_ReclaimResources,
    dxgi_device_EnqueueSetEvent,
    /* IDXGIDevice3 methods */
    dxgi_device_Trim,
    /* IWineDXGIDevice methods */
    dxgi_device_create_resource,
};

static inline struct dxgi_device *impl_from_IWineDXGISwapChainFactory(IWineDXGISwapChainFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_device, IWineDXGISwapChainFactory_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_factory_QueryInterface(IWineDXGISwapChainFactory *iface,
        REFIID iid, void **out)
{
    struct dxgi_device *device = impl_from_IWineDXGISwapChainFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return dxgi_device_QueryInterface(&device->IWineDXGIDevice_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_factory_AddRef(IWineDXGISwapChainFactory *iface)
{
    struct dxgi_device *device = impl_from_IWineDXGISwapChainFactory(iface);

    TRACE("iface %p.\n", iface);

    return dxgi_device_AddRef(&device->IWineDXGIDevice_iface);
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_factory_Release(IWineDXGISwapChainFactory *iface)
{
    struct dxgi_device *device = impl_from_IWineDXGISwapChainFactory(iface);

    TRACE("iface %p.\n", iface);

    return dxgi_device_Release(&device->IWineDXGIDevice_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_factory_create_swapchain(IWineDXGISwapChainFactory *iface,
        IDXGIFactory *factory, HWND window, const DXGI_SWAP_CHAIN_DESC1 *desc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc, IDXGIOutput *output, IDXGISwapChain1 **swapchain)
{
    struct dxgi_device *device = impl_from_IWineDXGISwapChainFactory(iface);
    struct wined3d_swapchain_desc wined3d_desc;
    struct IDXGIOutput *containing_output;
    struct dxgi_factory *dxgi_factory;
    struct d3d11_swapchain *object;
    HRESULT hr;

    TRACE("iface %p, factory %p, window %p, desc %p, fullscreen_desc %p, output %p, swapchain %p.\n",
            iface, factory, window, desc, fullscreen_desc, output, swapchain);

    if (!(dxgi_factory = unsafe_impl_from_IDXGIFactory(factory)))
    {
        WARN("Factory %p is not a valid dxgi factory.\n", factory);
        return E_FAIL;
    }

    if (FAILED(hr = dxgi_get_output_from_window(&dxgi_factory->IWineDXGIFactory_iface, window, &containing_output)))
    {
        WARN("Failed to get output from window %p, hr %#lx.\n", window, hr);
        return hr;
    }

    hr = wined3d_swapchain_desc_from_dxgi(&wined3d_desc, containing_output, window, desc,
            fullscreen_desc);
    IDXGIOutput_Release(containing_output);
    if (FAILED(hr))
        return hr;

    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate swapchain memory.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = d3d11_swapchain_init(object, device, &wined3d_desc)))
    {
        WARN("Failed to initialise swapchain, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);

    *swapchain = (IDXGISwapChain1 *)&object->IDXGISwapChain4_iface;

    return S_OK;
}

static const struct IWineDXGISwapChainFactoryVtbl dxgi_swapchain_factory_vtbl =
{
    dxgi_swapchain_factory_QueryInterface,
    dxgi_swapchain_factory_AddRef,
    dxgi_swapchain_factory_Release,
    dxgi_swapchain_factory_create_swapchain,
};

HRESULT dxgi_device_init(struct dxgi_device *device, struct dxgi_device_layer *layer,
        IDXGIFactory *factory, IDXGIAdapter *adapter,
        const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count)
{
    struct wined3d_device_parent *wined3d_device_parent;
    struct wined3d_swapchain_desc swapchain_desc;
    IWineDXGIDeviceParent *dxgi_device_parent;
    struct dxgi_adapter *dxgi_adapter;
    struct dxgi_factory *dxgi_factory;
    struct dxgi_output *dxgi_output;
    struct IDXGIOutput *output;
    void *layer_base;
    HWND window;
    HRESULT hr;

    if (!(dxgi_factory = unsafe_impl_from_IDXGIFactory(factory)))
    {
        WARN("This is not the factory we're looking for.\n");
        return E_FAIL;
    }

    if (!(dxgi_adapter = unsafe_impl_from_IDXGIAdapter(adapter)))
    {
        WARN("This is not the adapter we're looking for.\n");
        return E_FAIL;
    }

    device->IWineDXGIDevice_iface.lpVtbl = &dxgi_device_vtbl;
    device->IWineDXGISwapChainFactory_iface.lpVtbl = &dxgi_swapchain_factory_vtbl;
    device->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&device->private_store);

    layer_base = device + 1;

    if (FAILED(hr = layer->create(layer->id, &layer_base, 0,
            device, &IID_IUnknown, (void **)&device->child_layer)))
    {
        WARN("Failed to create device, returning %#lx.\n", hr);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    if (FAILED(hr = IWineDXGIDevice_QueryInterface(&device->IWineDXGIDevice_iface,
            &IID_IWineDXGIDeviceParent, (void **)&dxgi_device_parent)))
    {
        ERR("DXGI device should implement IWineDXGIDeviceParent.\n");
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_device_parent = IWineDXGIDeviceParent_get_wined3d_device_parent(dxgi_device_parent);
    IWineDXGIDeviceParent_Release(dxgi_device_parent);

    if (FAILED(hr = wined3d_device_create(dxgi_factory->wined3d,
            dxgi_adapter->wined3d_adapter, WINED3D_DEVICE_TYPE_HAL, NULL, 0, 4,
            (const enum wined3d_feature_level *)feature_levels, level_count,
            wined3d_device_parent, &device->wined3d_device)))
    {
        WARN("Failed to create a wined3d device, returning %#lx.\n", hr);
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    window = dxgi_factory_get_device_window(dxgi_factory);
    if (FAILED(hr = dxgi_get_output_from_window(&dxgi_factory->IWineDXGIFactory_iface, window, &output)))
    {
        ERR("Failed to get output from window %p.\n", window);
        wined3d_device_decref(device->wined3d_device);
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    dxgi_output = unsafe_impl_from_IDXGIOutput(output);

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    swapchain_desc.swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
    swapchain_desc.device_window = window;
    swapchain_desc.windowed = TRUE;
    swapchain_desc.flags = WINED3D_SWAPCHAIN_IMPLICIT;
    swapchain_desc.output = dxgi_output->wined3d_output;
    IDXGIOutput_Release(output);

    if (FAILED(hr = wined3d_swapchain_create(device->wined3d_device, &swapchain_desc,
            NULL, NULL, &dxgi_null_wined3d_parent_ops, &device->implicit_swapchain)))
    {
        ERR("Failed to create implicit swapchain, hr %#lx.\n", hr);
        wined3d_device_decref(device->wined3d_device);
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    wined3d_mutex_unlock();

    device->adapter = &dxgi_adapter->IWineDXGIAdapter_iface;
    IWineDXGIAdapter_AddRef(device->adapter);

    return S_OK;
}
