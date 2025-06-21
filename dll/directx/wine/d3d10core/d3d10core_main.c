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

#include "wine/debug.h"

#include "initguid.h"

#define COBJMACROS
#include "d3d10.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d11, IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count, void **device);

HRESULT WINAPI D3D10CoreRegisterLayers(void)
{
    TRACE("\n");

    return E_NOTIMPL;
}

HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device)
{
    IUnknown *dxgi_device;
    HMODULE d3d11;
    HRESULT hr;

    TRACE("factory %p, adapter %p, flags %#x, feature_level %#x, device %p.\n",
            factory, adapter, flags, feature_level, device);

    d3d11 = LoadLibraryA("d3d11.dll");
    hr = DXGID3D10CreateDevice(d3d11, factory, adapter, flags, &feature_level, 1, (void **)&dxgi_device);
    FreeLibrary(d3d11);
    if (FAILED(hr))
    {
        WARN("Failed to create device, hr %#lx.\n", hr);
        return hr;
    }

    hr = IUnknown_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)device);
    IUnknown_Release(dxgi_device);
    if (FAILED(hr))
    {
        ERR("Failed to query ID3D10Device interface, returning E_FAIL.\n");
        return E_FAIL;
    }

    return S_OK;
}
