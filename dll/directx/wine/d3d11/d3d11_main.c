/*
 * Direct3D 11
 *
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2013 Austin English
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

#define D3D11_INIT_GUID
#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static const char *debug_d3d_driver_type(D3D_DRIVER_TYPE driver_type)
{
    switch (driver_type)
    {
#define D3D11_TO_STR(x) case x: return #x
        D3D11_TO_STR(D3D_DRIVER_TYPE_UNKNOWN);
        D3D11_TO_STR(D3D_DRIVER_TYPE_HARDWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_REFERENCE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_NULL);
        D3D11_TO_STR(D3D_DRIVER_TYPE_SOFTWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_WARP);
#undef D3D11_TO_STR
        default:
            return wine_dbg_sprintf("Unrecognized D3D_DRIVER_TYPE %#x\n", driver_type);
    }
}

static HRESULT WINAPI layer_init(enum dxgi_device_layer_id id, DWORD *count, DWORD *values)
{
    TRACE("id %#x, count %p, values %p\n", id, count, values);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        return E_NOTIMPL;
    }

    return S_OK;
}

static UINT WINAPI layer_get_size(enum dxgi_device_layer_id id, struct layer_get_size_args *args, DWORD unknown0)
{
    TRACE("id %#x, args %p, unknown0 %#x\n", id, args, unknown0);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        return 0;
    }

    return sizeof(struct d3d_device);
}

static HRESULT WINAPI layer_create(enum dxgi_device_layer_id id, void **layer_base, DWORD unknown0,
        void *device_object, REFIID riid, void **device_layer)
{
    struct d3d_device *object;

    TRACE("id %#x, layer_base %p, unknown0 %#x, device_object %p, riid %s, device_layer %p\n",
            id, layer_base, unknown0, device_object, debugstr_guid(riid), device_layer);

    if (id != DXGI_DEVICE_LAYER_D3D10_DEVICE)
    {
        WARN("Unknown layer id %#x\n", id);
        *device_layer = NULL;
        return E_NOTIMPL;
    }

    object = *layer_base;
    d3d_device_init(object, device_object);
    *device_layer = &object->IUnknown_inner;

    TRACE("Created d3d10 device at %p\n", object);

    return S_OK;
}

HRESULT WINAPI D3D11CoreRegisterLayers(void)
{
    static const struct dxgi_device_layer layers[] =
    {
        {DXGI_DEVICE_LAYER_D3D10_DEVICE, layer_init, layer_get_size, layer_create},
    };

    DXGID3D10RegisterLayers(layers, ARRAY_SIZE(layers));

    return S_OK;
}

HRESULT WINAPI D3D11CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, ID3D11Device **device)
{
    IUnknown *dxgi_device;
    HMODULE d3d11;
    HRESULT hr;

    TRACE("factory %p, adapter %p, flags %#x, feature_levels %p, levels %u, device %p.\n",
            factory, adapter, flags, feature_levels, levels, device);

    d3d11 = GetModuleHandleA("d3d11.dll");
    hr = DXGID3D10CreateDevice(d3d11, factory, adapter, flags, feature_levels, levels, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create device, returning %#x.\n", hr);
        return hr;
    }

    hr = IUnknown_QueryInterface(dxgi_device, &IID_ID3D11Device, (void **)device);
    IUnknown_Release(dxgi_device);
    if (FAILED(hr))
    {
        ERR("Failed to query ID3D11Device interface, returning E_FAIL.\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT d3d11_create_device(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context)
{
    static const D3D_FEATURE_LEVEL default_feature_levels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };
    IDXGIFactory *factory;
    ID3D11Device *device;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, feature_levels %p, levels %u, sdk_version %u, "
            "device %p, obtained_feature_level %p, immediate_context %p.\n",
            adapter, debug_d3d_driver_type(driver_type), swrast, flags, feature_levels, levels, sdk_version,
            device_out, obtained_feature_level, immediate_context);

    if (device_out)
        *device_out = NULL;
    if (obtained_feature_level)
        *obtained_feature_level = 0;
    if (immediate_context)
        *immediate_context = NULL;

    if (adapter)
    {
        IDXGIAdapter_AddRef(adapter);
        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
        if (FAILED(hr))
        {
            WARN("Failed to get dxgi factory, returning %#x.\n", hr);
            return hr;
        }
    }
    else
    {
        hr = CreateDXGIFactory1(&IID_IDXGIFactory, (void **)&factory);
        if (FAILED(hr))
        {
            WARN("Failed to create dxgi factory, returning %#x.\n", hr);
            return hr;
        }

        switch(driver_type)
        {
            case D3D_DRIVER_TYPE_WARP:
                FIXME("WARP driver not implemented, falling back to hardware.\n");
            case D3D_DRIVER_TYPE_HARDWARE:
            {
                hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
                if (FAILED(hr))
                {
                    WARN("No adapters found, returning %#x.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            case D3D_DRIVER_TYPE_NULL:
                FIXME("NULL device not implemented, falling back to refrast.\n");
                /* fall through, for now */
            case D3D_DRIVER_TYPE_REFERENCE:
            {
                HMODULE refrast = LoadLibraryA("d3d11ref.dll");
                if (!refrast)
                {
                    WARN("Failed to load refrast, returning E_FAIL.\n");
                    IDXGIFactory_Release(factory);
                    return E_FAIL;
                }
                hr = IDXGIFactory_CreateSoftwareAdapter(factory, refrast, &adapter);
                FreeLibrary(refrast);
                if (FAILED(hr))
                {
                    WARN("Failed to create a software adapter, returning %#x.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            case D3D_DRIVER_TYPE_SOFTWARE:
            {
                if (!swrast)
                {
                    WARN("Software device requested, but NULL swrast passed, returning E_FAIL.\n");
                    IDXGIFactory_Release(factory);
                    return E_FAIL;
                }
                hr = IDXGIFactory_CreateSoftwareAdapter(factory, swrast, &adapter);
                if (FAILED(hr))
                {
                    WARN("Failed to create a software adapter, returning %#x.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            default:
                FIXME("Unhandled driver type %#x.\n", driver_type);
                IDXGIFactory_Release(factory);
                return E_FAIL;
        }
    }

    if (!feature_levels)
    {
        feature_levels = default_feature_levels;
        levels = ARRAY_SIZE(default_feature_levels);
    }
    hr = D3D11CoreCreateDevice(factory, adapter, flags, feature_levels, levels, &device);
    IDXGIAdapter_Release(adapter);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        WARN("Failed to create a device, returning %#x.\n", hr);
        return hr;
    }

    TRACE("Created ID3D11Device %p.\n", device);

    if (obtained_feature_level)
        *obtained_feature_level = ID3D11Device_GetFeatureLevel(device);

    if (immediate_context)
        ID3D11Device_GetImmediateContext(device, immediate_context);

    if (device_out)
        *device_out = device;
    else
        ID3D11Device_Release(device);

    return (device_out || immediate_context) ? S_OK : S_FALSE;
}

HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context)
{
    return d3d11_create_device(adapter, driver_type, swrast, flags, feature_levels,
            levels, sdk_version, device_out, obtained_feature_level, immediate_context);
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, const D3D_FEATURE_LEVEL *feature_levels, UINT levels,
        UINT sdk_version, const DXGI_SWAP_CHAIN_DESC *swapchain_desc, IDXGISwapChain **swapchain,
        ID3D11Device **device_out, D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context)
{
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGIDevice *dxgi_device;
    IDXGIFactory *factory;
    ID3D11Device *device;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, feature_levels %p, levels %u, sdk_version %u, "
            "swapchain_desc %p, swapchain %p, device %p, obtained_feature_level %p, immediate_context %p.\n",
            adapter, debug_d3d_driver_type(driver_type), swrast, flags, feature_levels, levels, sdk_version,
            swapchain_desc, swapchain, device_out, obtained_feature_level, immediate_context);

    if (swapchain)
        *swapchain = NULL;
    if (device_out)
        *device_out = NULL;

    /* Avoid forwarding to D3D11CreateDevice(), since it breaks applications
     * hooking these entry-points. */
    if (FAILED(hr = d3d11_create_device(adapter, driver_type, swrast, flags, feature_levels,
            levels, sdk_version, &device, obtained_feature_level, immediate_context)))
    {
        WARN("Failed to create a device, returning %#x.\n", hr);
        return hr;
    }

    if (swapchain)
    {
        if (FAILED(hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device)))
        {
            ERR("Failed to get a dxgi device from the d3d11 device, returning %#x.\n", hr);
            goto cleanup;
        }

        hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
        IDXGIDevice_Release(dxgi_device);
        if (FAILED(hr))
        {
            ERR("Failed to get the device adapter, returning %#x.\n", hr);
            goto cleanup;
        }

        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
        IDXGIAdapter_Release(adapter);
        if (FAILED(hr))
        {
            ERR("Failed to get the adapter factory, returning %#x.\n", hr);
            goto cleanup;
        }

        desc = *swapchain_desc;
        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &desc, swapchain);
        IDXGIFactory_Release(factory);
        if (FAILED(hr))
        {
            WARN("Failed to create a swapchain, returning %#x.\n", hr);
            goto cleanup;
        }

        TRACE("Created IDXGISwapChain %p.\n", *swapchain);
    }

    if (device_out)
        *device_out = device;
    else
        ID3D11Device_Release(device);

    return (swapchain || device_out || immediate_context) ? S_OK : S_FALSE;

cleanup:
    ID3D11Device_Release(device);
    if (obtained_feature_level)
        *obtained_feature_level = 0;
    if (immediate_context)
    {
        ID3D11DeviceContext_Release(*immediate_context);
        *immediate_context = NULL;
    }

    return hr;
}

HRESULT WINAPI D3D11On12CreateDevice(IUnknown *device, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT feature_level_count,
        IUnknown * const *queues, UINT queue_count, UINT node_mask,
        ID3D11Device **d3d11_device, ID3D11DeviceContext **d3d11_immediate_context,
        D3D_FEATURE_LEVEL *obtained_feature_level)
{
    FIXME("device %p, flags %#x, feature_levels %p, feature_level_count %u, "
            "queues %p, queue_count %u, node_mask 0x%08x, "
            "d3d11_device %p, d3d11_immediate_context %p, obtained_feature_level %p stub!\n",
            device, flags, feature_levels, feature_level_count, queues, queue_count,
            node_mask, d3d11_device, d3d11_immediate_context, obtained_feature_level);

    return E_NOTIMPL;
}
