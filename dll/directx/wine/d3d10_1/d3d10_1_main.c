/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

#include "wine/debug.h"

#define COBJMACROS
#include "d3d10_1.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device);

#define WINE_D3D10_TO_STR(x) case x: return #x

static const char *debug_d3d10_driver_type(D3D10_DRIVER_TYPE driver_type)
{
    switch (driver_type)
    {
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_HARDWARE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_REFERENCE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_NULL);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_SOFTWARE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_WARP);
        default:
            FIXME("Unrecognized D3D10_DRIVER_TYPE %#x.\n", driver_type);
            return "unrecognized";
    }
}

static const char *debug_d3d10_feature_level(D3D10_FEATURE_LEVEL1 feature_level)
{
    switch (feature_level)
    {
        WINE_D3D10_TO_STR(D3D10_FEATURE_LEVEL_10_0);
        WINE_D3D10_TO_STR(D3D10_FEATURE_LEVEL_10_1);
        WINE_D3D10_TO_STR(D3D10_FEATURE_LEVEL_9_1);
        WINE_D3D10_TO_STR(D3D10_FEATURE_LEVEL_9_2);
        WINE_D3D10_TO_STR(D3D10_FEATURE_LEVEL_9_3);
        default:
            FIXME("Unrecognized D3D10_FEATURE_LEVEL1 %#x.\n", feature_level);
            return "unrecognized";
    }
}

#undef WINE_D3D10_TO_STR

static D3D_FEATURE_LEVEL d3d_feature_level_from_d3d10_1(D3D10_FEATURE_LEVEL1 level)
{
    return (D3D_FEATURE_LEVEL)level;
}

static HRESULT d3d10_create_device1(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type, HMODULE swrast,
        UINT flags, D3D10_FEATURE_LEVEL1 hw_level, UINT sdk_version, ID3D10Device1 **device)
{
    IDXGIFactory *factory;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, hw_level %s, sdk_version %d, device %p.\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags,
            debug_d3d10_feature_level(hw_level), sdk_version, device);

    if (!device)
        return E_INVALIDARG;

    *device = NULL;

    if (!hw_level)
        return E_INVALIDARG;

    if (adapter)
    {
        IDXGIAdapter_AddRef(adapter);
        if (FAILED(hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory)))
        {
            WARN("Failed to get dxgi factory, hr %#lx.\n", hr);
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory)))
        {
            WARN("Failed to create dxgi factory, hr %#lx.\n", hr);
            return hr;
        }

        switch (driver_type)
        {
            case D3D10_DRIVER_TYPE_WARP:
                FIXME("WARP driver not implemented, falling back to hardware.\n");
            case D3D10_DRIVER_TYPE_HARDWARE:
            {
                if (FAILED(hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter)))
                {
                    WARN("No adapters found, hr %#lx.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            case D3D10_DRIVER_TYPE_NULL:
                FIXME("NULL device not implemented, falling back to refrast.\n");
                /* Fall through, for now. */
            case D3D10_DRIVER_TYPE_REFERENCE:
            {
                HMODULE refrast;

                if (!(refrast = LoadLibraryA("d3d10ref.dll")))
                {
                    WARN("Failed to load refrast, returning E_FAIL.\n");
                    IDXGIFactory_Release(factory);
                    return E_FAIL;
                }
                hr = IDXGIFactory_CreateSoftwareAdapter(factory, refrast, &adapter);
                FreeLibrary(refrast);
                if (FAILED(hr))
                {
                    WARN("Failed to create a software adapter, hr %#lx.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            case D3D10_DRIVER_TYPE_SOFTWARE:
            {
                if (!swrast)
                {
                    WARN("Software device requested, but NULL swrast passed, returning E_FAIL.\n");
                    IDXGIFactory_Release(factory);
                    return E_FAIL;
                }
                if (FAILED(hr = IDXGIFactory_CreateSoftwareAdapter(factory, swrast, &adapter)))
                {
                    WARN("Failed to create a software adapter, hr %#lx.\n", hr);
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

    hr = D3D10CoreCreateDevice(factory, adapter, flags,
            d3d_feature_level_from_d3d10_1(hw_level), (ID3D10Device **)device);
    IDXGIAdapter_Release(adapter);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        WARN("Failed to create a device, hr %#lx.\n", hr);
        return hr;
    }

    TRACE("Created device %p.\n", *device);

    return hr;
}

HRESULT WINAPI D3D10CreateDevice1(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type, HMODULE swrast,
        UINT flags, D3D10_FEATURE_LEVEL1 hw_level, UINT sdk_version, ID3D10Device1 **device)
{
    return d3d10_create_device1(adapter, driver_type, swrast, flags, hw_level, sdk_version, device);
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain1(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, D3D10_FEATURE_LEVEL1 feature_level, UINT sdk_version,
        DXGI_SWAP_CHAIN_DESC *swapchain_desc, IDXGISwapChain **swapchain, ID3D10Device1 **device)
{
    IDXGIDevice *dxgi_device;
    IDXGIFactory *factory;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, "
            "feature_level %s, sdk_version %d, swapchain_desc %p, swapchain %p, device %p.\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags,
            debug_d3d10_feature_level(feature_level), sdk_version, swapchain_desc, swapchain, device);

    if (swapchain)
        *swapchain = NULL;

    if (!device)
        return E_INVALIDARG;

    /* Avoid forwarding to D3D10CreateDevice1(), since it breaks applications
     * hooking these entry-points. */
    if (FAILED(hr = d3d10_create_device1(adapter, driver_type, swrast, flags, feature_level, sdk_version, device)))
    {
        WARN("Failed to create a device, returning %#lx.\n", hr);
        *device = NULL;
        return hr;
    }

    if (swapchain)
    {
        if (FAILED(hr = ID3D10Device1_QueryInterface(*device, &IID_IDXGIDevice, (void **)&dxgi_device)))
        {
            ERR("Failed to get a dxgi device from the d3d10 device, returning %#lx.\n", hr);
            goto cleanup;
        }

        hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
        IDXGIDevice_Release(dxgi_device);
        if (FAILED(hr))
        {
            ERR("Failed to get the device adapter, returning %#lx.\n", hr);
            goto cleanup;
        }

        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
        IDXGIAdapter_Release(adapter);
        if (FAILED(hr))
        {
            ERR("Failed to get the adapter factory, returning %#lx.\n", hr);
            goto cleanup;
        }

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)*device, swapchain_desc, swapchain);
        IDXGIFactory_Release(factory);
        if (FAILED(hr))
        {
            WARN("Failed to create a swapchain, returning %#lx.\n", hr);
            goto cleanup;
        }

        TRACE("Created IDXGISwapChain %p.\n", *swapchain);
    }

    return S_OK;

cleanup:
    ID3D10Device1_Release(*device);
    *device = NULL;
    return hr;
}
