/*
 * D3DX10 main file
 *
 * Copyright (c) 2010 Owen Rudge for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10_1.h"
#include "d3dx10.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/***********************************************************************
 * D3DX10CheckVersion
 *
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DX10CheckVersion(UINT d3dsdkvers, UINT d3dxsdkvers)
{
    if ((d3dsdkvers == D3D10_SDK_VERSION) && (d3dxsdkvers == 43))
        return TRUE;

    return FALSE;
}

HRESULT WINAPI D3DX10CreateEffectPoolFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile, UINT hlslflags,
        UINT fxflags, ID3D10Device *device, ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool,
        ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("data %p, datasize %Iu, filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, pump %p, effectpool %p, errors %p, hresult %p.\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            pump, effectpool, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10UnsetAllDeviceObjects(ID3D10Device *device)
{
    static ID3D10ShaderResourceView * const views[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    static ID3D10RenderTargetView * const target_views[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    static ID3D10SamplerState * const sampler_states[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    static ID3D10Buffer * const buffers[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const unsigned int so_offsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] =
            {~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u};
    static const unsigned int strides[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const unsigned int offsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const float blend_factors[4];

    TRACE("device %p.\n", device);

    if (!device)
        return E_INVALIDARG;

    ID3D10Device_VSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);
    ID3D10Device_PSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);
    ID3D10Device_GSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);

    ID3D10Device_VSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);
    ID3D10Device_PSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);
    ID3D10Device_GSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);

    ID3D10Device_VSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);
    ID3D10Device_PSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);
    ID3D10Device_GSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);

    ID3D10Device_VSSetShader(device, NULL);
    ID3D10Device_PSSetShader(device, NULL);
    ID3D10Device_GSSetShader(device, NULL);

    ID3D10Device_OMSetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, target_views, NULL);

    ID3D10Device_IASetIndexBuffer(device, NULL, DXGI_FORMAT_R32_UINT, 0);
    ID3D10Device_IASetInputLayout(device, NULL);
    ID3D10Device_IASetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, buffers, strides, offsets);

    ID3D10Device_SOSetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, buffers, so_offsets);

    ID3D10Device_OMSetBlendState(device, NULL, blend_factors, 0);
    ID3D10Device_OMSetDepthStencilState(device, NULL, 0);

    ID3D10Device_RSSetState(device, NULL);

    ID3D10Device_SetPredication(device, NULL, FALSE);

    return S_OK;
}

HRESULT WINAPI D3DX10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, ID3D10Device **device)
{
    HRESULT hr;

    TRACE("adapter %p, driver_type %d, swrast %p, flags %#x, device %p.\n", adapter, driver_type,
            swrast, flags, device);

    if (SUCCEEDED(hr = D3D10CreateDevice1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_1,
            D3D10_SDK_VERSION, (ID3D10Device1 **)device)))
        return hr;

    if (SUCCEEDED(hr = D3D10CreateDevice1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_0,
            D3D10_SDK_VERSION, (ID3D10Device1 **)device)))
        return hr;

    return hr;
}

HRESULT WINAPI D3DX10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain,
        ID3D10Device **device)
{
    HRESULT hr;

    TRACE("adapter %p, driver_type %d, swrast %p, flags %#x, desc %p, swapchain %p, device %p.\n",
            adapter, driver_type, swrast, flags, desc, swapchain, device);

    if (SUCCEEDED(hr = D3D10CreateDeviceAndSwapChain1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_1,
            D3D10_1_SDK_VERSION, desc, swapchain, (ID3D10Device1 **)device)))
        return hr;

    return D3D10CreateDeviceAndSwapChain1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_0,
            D3D10_1_SDK_VERSION, desc, swapchain, (ID3D10Device1 **)device);
}

HRESULT WINAPI D3DX10FilterTexture(ID3D10Resource *texture, UINT src_level, UINT filter)
{
    FIXME("texture %p, src_level %u, filter %#x stub!\n", texture, src_level, filter);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10GetFeatureLevel1(ID3D10Device *device, ID3D10Device1 **device1)
{
    TRACE("device %p, device1 %p.\n", device, device1);

    return ID3D10Device_QueryInterface(device, &IID_ID3D10Device1, (void **)device1);
}

D3DX_CPU_OPTIMIZATION WINAPI D3DXCpuOptimizations(BOOL enable)
{
    FIXME("enable %#x stub.\n", enable);

    return D3DX_NOT_OPTIMIZED;
}

HRESULT WINAPI D3DX10LoadTextureFromTexture(ID3D10Resource *src_texture, D3DX10_TEXTURE_LOAD_INFO *load_info,
        ID3D10Resource *dst_texture)
{
    FIXME("src_texture %p, load_info %p, dst_texture %p stub!\n", src_texture, load_info, dst_texture);

    return E_NOTIMPL;
}
