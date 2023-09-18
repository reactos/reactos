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
#include "d3d11.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

HRESULT WINAPI D3D11CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter, unsigned int flags,
        const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count, ID3D11Device **device);

HRESULT WINAPI D3D10CoreRegisterLayers(void)
{
    TRACE("\n");

    return E_NOTIMPL;
}

HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device)
{
    ID3D11Device *device11;
    HRESULT hr;

    TRACE("factory %p, adapter %p, flags %#x, feature_level %#x, device %p.\n",
            factory, adapter, flags, feature_level, device);

    if (FAILED(hr = D3D11CoreCreateDevice(factory, adapter, flags, &feature_level, 1, &device11)))
        return hr;

    hr = ID3D11Device_QueryInterface(device11, &IID_ID3D10Device, (void **)device);
    ID3D11Device_Release(device11);
    if (FAILED(hr))
    {
        ERR("Device should implement ID3D10Device, returning E_FAIL.\n");
        return E_FAIL;
    }

    return S_OK;
}
