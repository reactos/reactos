/*
 * Direct3D 10
 *
 * Copyright 2007 Andras Kovacs
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

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
            FIXME("Unrecognised D3D10_DRIVER_TYPE %#x.\n", driver_type);
            return "unrecognised";
    }
}

#undef WINE_D3D10_TO_STR

static HRESULT d3d10_create_device(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, UINT sdk_version, ID3D10Device **device)
{
    IDXGIFactory *factory;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, sdk_version %#x, device %p.\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags, sdk_version, device);

    if (sdk_version != D3D10_SDK_VERSION)
    {
        WARN("Invalid SDK version %#x.\n", sdk_version);
        return E_INVALIDARG;
    }

    if (adapter)
    {
        IDXGIAdapter_AddRef(adapter);
        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
        if (FAILED(hr))
        {
            WARN("Failed to get dxgi factory, returning %#lx.\n", hr);
            return hr;
        }
    }
    else
    {
        hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
        if (FAILED(hr))
        {
            WARN("Failed to create dxgi factory, returning %#lx.\n", hr);
            return hr;
        }

        switch (driver_type)
        {
            case D3D10_DRIVER_TYPE_WARP:
                FIXME("WARP driver not implemented, falling back to hardware.\n");
            case D3D10_DRIVER_TYPE_HARDWARE:
            {
                hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
                if (FAILED(hr))
                {
                    WARN("No adapters found, returning %#lx.\n", hr);
                    IDXGIFactory_Release(factory);
                    return hr;
                }
                break;
            }

            case D3D10_DRIVER_TYPE_NULL:
                FIXME("NULL device not implemented, falling back to refrast.\n");
                /* fall through, for now */
            case D3D10_DRIVER_TYPE_REFERENCE:
            {
                HMODULE refrast = LoadLibraryA("d3d10ref.dll");
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
                    WARN("Failed to create a software adapter, returning %#lx.\n", hr);
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
                hr = IDXGIFactory_CreateSoftwareAdapter(factory, swrast, &adapter);
                if (FAILED(hr))
                {
                    WARN("Failed to create a software adapter, returning %#lx.\n", hr);
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

    hr = D3D10CoreCreateDevice(factory, adapter, flags, D3D_FEATURE_LEVEL_10_0, device);
    IDXGIAdapter_Release(adapter);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        WARN("Failed to create a device, returning %#lx.\n", hr);
        return hr;
    }

    TRACE("Created ID3D10Device %p.\n", *device);

    return hr;
}

HRESULT WINAPI D3D10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, UINT sdk_version, ID3D10Device **device)
{
    return d3d10_create_device(adapter, driver_type, swrast, flags, sdk_version, device);
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, UINT sdk_version, DXGI_SWAP_CHAIN_DESC *swapchain_desc,
        IDXGISwapChain **swapchain, ID3D10Device **device)
{
    IDXGIDevice *dxgi_device;
    IDXGIFactory *factory;
    HRESULT hr;

    TRACE("adapter %p, driver_type %s, swrast %p, flags %#x, sdk_version %d, "
            "swapchain_desc %p, swapchain %p, device %p\n",
            adapter, debug_d3d10_driver_type(driver_type), swrast, flags, sdk_version,
            swapchain_desc, swapchain, device);

    /* Avoid forwarding to D3D10CreateDevice(), since it breaks applications
     * hooking these entry-points. */
    if (FAILED(hr = d3d10_create_device(adapter, driver_type, swrast, flags, sdk_version, device)))
    {
        WARN("Failed to create a device, returning %#lx.\n", hr);
        *device = NULL;
        return hr;
    }

    TRACE("Created ID3D10Device %p\n", *device);

    hr = ID3D10Device_QueryInterface(*device, &IID_IDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        ERR("Failed to get a dxgi device from the d3d10 device, returning %#lx.\n", hr);
        ID3D10Device_Release(*device);
        *device = NULL;
        return hr;
    }

    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        ERR("Failed to get the device adapter, returning %#lx.\n", hr);
        ID3D10Device_Release(*device);
        *device = NULL;
        return hr;
    }

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    IDXGIAdapter_Release(adapter);
    if (FAILED(hr))
    {
        ERR("Failed to get the adapter factory, returning %#lx.\n", hr);
        ID3D10Device_Release(*device);
        *device = NULL;
        return hr;
    }

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)*device, swapchain_desc, swapchain);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        ID3D10Device_Release(*device);
        *device = NULL;

        WARN("Failed to create a swapchain, returning %#lx.\n", hr);
        return hr;
    }

    TRACE("Created IDXGISwapChain %p\n", *swapchain);

    return S_OK;
}

HRESULT WINAPI D3D10CompileEffectFromMemory(void *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, UINT hlsl_flags, UINT fx_flags,
        ID3D10Blob **effect, ID3D10Blob **errors)
{
    TRACE("data %p, data_size %Iu, filename %s, defines %p, include %p, "
            "hlsl_flags %#x, fx_flags %#x, effect %p, errors %p.\n",
            data, data_size, wine_dbgstr_a(filename), defines, include,
            hlsl_flags, fx_flags, effect, errors);

    return D3DCompileFromMemory(data, data_size, filename, defines, include,
            NULL, "fx_4_0", hlsl_flags, fx_flags, effect, errors);
}

const char * WINAPI D3D10GetVertexShaderProfile(ID3D10Device *device)
{
    FIXME("device %p stub!\n", device);

    return "vs_4_0";
}

const char * WINAPI D3D10GetGeometryShaderProfile(ID3D10Device *device)
{
    FIXME("device %p stub!\n", device);

    return "gs_4_0";
}

const char * WINAPI D3D10GetPixelShaderProfile(ID3D10Device *device)
{
    FIXME("device %p stub!\n", device);

    return "ps_4_0";
}
