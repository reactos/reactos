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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

#define WINE_D3D_TO_STR(x) case x: return #x

const char *debug_d3d10_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY topology)
{
    switch (topology)
    {
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
        WINE_D3D_TO_STR(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
        default:
            FIXME("Unrecognized D3D10_PRIMITIVE_TOPOLOGY %#x\n", topology);
            return "unrecognized";
    }
}

const char *debug_dxgi_format(DXGI_FORMAT format)
{
    switch(format)
    {
        WINE_D3D_TO_STR(DXGI_FORMAT_UNKNOWN);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32A32_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32A32_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32A32_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32A32_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32B32_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16B16A16_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G32_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32G8X24_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R10G10B10A2_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R10G10B10A2_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R10G10B10A2_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R11G11B10_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8B8A8_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16G16_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_D32_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R32_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R24G8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_D24_UNORM_S8_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_FLOAT);
        WINE_D3D_TO_STR(DXGI_FORMAT_D16_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R16_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8_UINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8_SINT);
        WINE_D3D_TO_STR(DXGI_FORMAT_A8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R1_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
        WINE_D3D_TO_STR(DXGI_FORMAT_R8G8_B8G8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_G8R8_G8B8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC1_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC1_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC1_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC2_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC2_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC2_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC3_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC3_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC3_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC4_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC4_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC4_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC5_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC5_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC5_SNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_B5G6R5_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_B5G5R5A1_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8A8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8X8_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8A8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8X8_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC6H_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC6H_UF16);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC6H_SF16);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC7_TYPELESS);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC7_UNORM);
        WINE_D3D_TO_STR(DXGI_FORMAT_BC7_UNORM_SRGB);
        WINE_D3D_TO_STR(DXGI_FORMAT_B4G4R4A4_UNORM);
        default:
            FIXME("Unrecognized DXGI_FORMAT %#x.\n", format);
            return "unrecognized";
    }
}

#undef WINE_D3D_TO_STR

const char *debug_float4(const float *values)
{
    if (!values)
        return "(null)";
    return wine_dbg_sprintf("{%.8e, %.8e, %.8e, %.8e}",
            values[0], values[1], values[2], values[3]);
}

void d3d11_primitive_topology_from_wined3d_primitive_type(enum wined3d_primitive_type primitive_type,
        unsigned int patch_vertex_count, D3D11_PRIMITIVE_TOPOLOGY *topology)
{
    if (primitive_type <= WINED3D_PT_TRIANGLESTRIP_ADJ)
    {
        *topology = (D3D11_PRIMITIVE_TOPOLOGY)primitive_type;
        return;
    }

    if (primitive_type == WINED3D_PT_PATCH)
    {
        *topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + patch_vertex_count - 1;
        return;
    }

    *topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void wined3d_primitive_type_from_d3d11_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY topology,
        enum wined3d_primitive_type *type, unsigned int *patch_vertex_count)
{
    if (topology <= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ)
    {
        *type = (enum wined3d_primitive_type)topology;
        *patch_vertex_count = 0;
        return;
    }

    if (D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST <= topology
            && topology <= D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST)
    {
        *type = WINED3D_PT_PATCH;
        *patch_vertex_count = topology - D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1;
        return;
    }

    WARN("Invalid primitive topology %#x.\n", topology);
    *type = WINED3D_PT_UNDEFINED;
    *patch_vertex_count = 0;
}

DXGI_FORMAT dxgi_format_from_wined3dformat(enum wined3d_format_id format)
{
    switch(format)
    {
        case WINED3DFMT_UNKNOWN: return DXGI_FORMAT_UNKNOWN;
        case WINED3DFMT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case WINED3DFMT_R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case WINED3DFMT_R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
        case WINED3DFMT_R32G32B32A32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
        case WINED3DFMT_R32G32B32_TYPELESS: return DXGI_FORMAT_R32G32B32_TYPELESS;
        case WINED3DFMT_R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
        case WINED3DFMT_R32G32B32_UINT: return DXGI_FORMAT_R32G32B32_UINT;
        case WINED3DFMT_R32G32B32_SINT: return DXGI_FORMAT_R32G32B32_SINT;
        case WINED3DFMT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case WINED3DFMT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case WINED3DFMT_R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case WINED3DFMT_R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
        case WINED3DFMT_R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case WINED3DFMT_R16G16B16A16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
        case WINED3DFMT_R32G32_TYPELESS: return DXGI_FORMAT_R32G32_TYPELESS;
        case WINED3DFMT_R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
        case WINED3DFMT_R32G32_UINT: return DXGI_FORMAT_R32G32_UINT;
        case WINED3DFMT_R32G32_SINT: return DXGI_FORMAT_R32G32_SINT;
        case WINED3DFMT_R32G8X24_TYPELESS: return DXGI_FORMAT_R32G8X24_TYPELESS;
        case WINED3DFMT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case WINED3DFMT_R32_FLOAT_X8X24_TYPELESS: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case WINED3DFMT_X32_TYPELESS_G8X24_UINT: return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        case WINED3DFMT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_TYPELESS;
        case WINED3DFMT_R10G10B10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case WINED3DFMT_R10G10B10A2_UINT: return DXGI_FORMAT_R10G10B10A2_UINT;
        case WINED3DFMT_R11G11B10_FLOAT: return DXGI_FORMAT_R11G11B10_FLOAT;
        case WINED3DFMT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case WINED3DFMT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case WINED3DFMT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case WINED3DFMT_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
        case WINED3DFMT_R8G8B8A8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case WINED3DFMT_R8G8B8A8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
        case WINED3DFMT_R16G16_TYPELESS: return DXGI_FORMAT_R16G16_TYPELESS;
        case WINED3DFMT_R16G16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
        case WINED3DFMT_R16G16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
        case WINED3DFMT_R16G16_UINT: return DXGI_FORMAT_R16G16_UINT;
        case WINED3DFMT_R16G16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
        case WINED3DFMT_R16G16_SINT: return DXGI_FORMAT_R16G16_SINT;
        case WINED3DFMT_R32_TYPELESS: return DXGI_FORMAT_R32_TYPELESS;
        case WINED3DFMT_D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
        case WINED3DFMT_R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
        case WINED3DFMT_R32_UINT: return DXGI_FORMAT_R32_UINT;
        case WINED3DFMT_R32_SINT: return DXGI_FORMAT_R32_SINT;
        case WINED3DFMT_R24G8_TYPELESS: return DXGI_FORMAT_R24G8_TYPELESS;
        case WINED3DFMT_D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case WINED3DFMT_R24_UNORM_X8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case WINED3DFMT_X24_TYPELESS_G8_UINT: return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        case WINED3DFMT_R8G8_TYPELESS: return DXGI_FORMAT_R8G8_TYPELESS;
        case WINED3DFMT_R8G8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
        case WINED3DFMT_R8G8_UINT: return DXGI_FORMAT_R8G8_UINT;
        case WINED3DFMT_R8G8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
        case WINED3DFMT_R8G8_SINT: return DXGI_FORMAT_R8G8_SINT;
        case WINED3DFMT_R16_TYPELESS: return DXGI_FORMAT_R16_TYPELESS;
        case WINED3DFMT_R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;
        case WINED3DFMT_D16_UNORM: return DXGI_FORMAT_D16_UNORM;
        case WINED3DFMT_R16_UNORM: return DXGI_FORMAT_R16_UNORM;
        case WINED3DFMT_R16_UINT: return DXGI_FORMAT_R16_UINT;
        case WINED3DFMT_R16_SNORM: return DXGI_FORMAT_R16_SNORM;
        case WINED3DFMT_R16_SINT: return DXGI_FORMAT_R16_SINT;
        case WINED3DFMT_R8_TYPELESS: return DXGI_FORMAT_R8_TYPELESS;
        case WINED3DFMT_R8_UNORM: return DXGI_FORMAT_R8_UNORM;
        case WINED3DFMT_R8_UINT: return DXGI_FORMAT_R8_UINT;
        case WINED3DFMT_R8_SNORM: return DXGI_FORMAT_R8_SNORM;
        case WINED3DFMT_R8_SINT: return DXGI_FORMAT_R8_SINT;
        case WINED3DFMT_A8_UNORM: return DXGI_FORMAT_A8_UNORM;
        case WINED3DFMT_R1_UNORM: return DXGI_FORMAT_R1_UNORM;
        case WINED3DFMT_R9G9B9E5_SHAREDEXP: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        case WINED3DFMT_R8G8_B8G8_UNORM: return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case WINED3DFMT_G8R8_G8B8_UNORM: return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case WINED3DFMT_BC1_TYPELESS: return DXGI_FORMAT_BC1_TYPELESS;
        case WINED3DFMT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM;
        case WINED3DFMT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case WINED3DFMT_BC2_TYPELESS: return DXGI_FORMAT_BC2_TYPELESS;
        case WINED3DFMT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM;
        case WINED3DFMT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case WINED3DFMT_BC3_TYPELESS: return DXGI_FORMAT_BC3_TYPELESS;
        case WINED3DFMT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM;
        case WINED3DFMT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case WINED3DFMT_BC4_TYPELESS: return DXGI_FORMAT_BC4_TYPELESS;
        case WINED3DFMT_BC4_UNORM: return DXGI_FORMAT_BC4_UNORM;
        case WINED3DFMT_BC4_SNORM: return DXGI_FORMAT_BC4_SNORM;
        case WINED3DFMT_BC5_TYPELESS: return DXGI_FORMAT_BC5_TYPELESS;
        case WINED3DFMT_BC5_UNORM: return DXGI_FORMAT_BC5_UNORM;
        case WINED3DFMT_BC5_SNORM: return DXGI_FORMAT_BC5_SNORM;
        case WINED3DFMT_B5G6R5_UNORM: return DXGI_FORMAT_B5G6R5_UNORM;
        case WINED3DFMT_B5G5R5A1_UNORM: return DXGI_FORMAT_B5G5R5A1_UNORM;
        case WINED3DFMT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case WINED3DFMT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM;
        case WINED3DFMT_R10G10B10_XR_BIAS_A2_UNORM: return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
        case WINED3DFMT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case WINED3DFMT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case WINED3DFMT_B8G8R8X8_TYPELESS: return DXGI_FORMAT_B8G8R8X8_TYPELESS;
        case WINED3DFMT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case WINED3DFMT_BC6H_TYPELESS: return DXGI_FORMAT_BC6H_TYPELESS;
        case WINED3DFMT_BC6H_UF16: return DXGI_FORMAT_BC6H_UF16;
        case WINED3DFMT_BC6H_SF16: return DXGI_FORMAT_BC6H_SF16;
        case WINED3DFMT_BC7_TYPELESS: return DXGI_FORMAT_BC7_TYPELESS;
        case WINED3DFMT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM;
        case WINED3DFMT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM_SRGB;
        case WINED3DFMT_B4G4R4A4_UNORM: return DXGI_FORMAT_B4G4R4A4_UNORM;
        default:
            FIXME("Unhandled wined3d format %#x.\n", format);
            return DXGI_FORMAT_UNKNOWN;
    }
}

enum wined3d_format_id wined3dformat_from_dxgi_format(DXGI_FORMAT format)
{
    switch(format)
    {
        case DXGI_FORMAT_UNKNOWN: return WINED3DFMT_UNKNOWN;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return WINED3DFMT_R32G32B32A32_TYPELESS;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return WINED3DFMT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT: return WINED3DFMT_R32G32B32A32_UINT;
        case DXGI_FORMAT_R32G32B32A32_SINT: return WINED3DFMT_R32G32B32A32_SINT;
        case DXGI_FORMAT_R32G32B32_TYPELESS: return WINED3DFMT_R32G32B32_TYPELESS;
        case DXGI_FORMAT_R32G32B32_FLOAT: return WINED3DFMT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32B32_UINT: return WINED3DFMT_R32G32B32_UINT;
        case DXGI_FORMAT_R32G32B32_SINT: return WINED3DFMT_R32G32B32_SINT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return WINED3DFMT_R16G16B16A16_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return WINED3DFMT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return WINED3DFMT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_UINT: return WINED3DFMT_R16G16B16A16_UINT;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return WINED3DFMT_R16G16B16A16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_SINT: return WINED3DFMT_R16G16B16A16_SINT;
        case DXGI_FORMAT_R32G32_TYPELESS: return WINED3DFMT_R32G32_TYPELESS;
        case DXGI_FORMAT_R32G32_FLOAT: return WINED3DFMT_R32G32_FLOAT;
        case DXGI_FORMAT_R32G32_UINT: return WINED3DFMT_R32G32_UINT;
        case DXGI_FORMAT_R32G32_SINT: return WINED3DFMT_R32G32_SINT;
        case DXGI_FORMAT_R32G8X24_TYPELESS: return WINED3DFMT_R32G8X24_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return WINED3DFMT_D32_FLOAT_S8X24_UINT;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return WINED3DFMT_R32_FLOAT_X8X24_TYPELESS;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return WINED3DFMT_X32_TYPELESS_G8X24_UINT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS: return WINED3DFMT_R10G10B10A2_TYPELESS;
        case DXGI_FORMAT_R10G10B10A2_UNORM: return WINED3DFMT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R10G10B10A2_UINT: return WINED3DFMT_R10G10B10A2_UINT;
        case DXGI_FORMAT_R11G11B10_FLOAT: return WINED3DFMT_R11G11B10_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS: return WINED3DFMT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return WINED3DFMT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return WINED3DFMT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_R8G8B8A8_UINT: return WINED3DFMT_R8G8B8A8_UINT;
        case DXGI_FORMAT_R8G8B8A8_SNORM: return WINED3DFMT_R8G8B8A8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_SINT: return WINED3DFMT_R8G8B8A8_SINT;
        case DXGI_FORMAT_R16G16_TYPELESS: return WINED3DFMT_R16G16_TYPELESS;
        case DXGI_FORMAT_R16G16_FLOAT: return WINED3DFMT_R16G16_FLOAT;
        case DXGI_FORMAT_R16G16_UNORM: return WINED3DFMT_R16G16_UNORM;
        case DXGI_FORMAT_R16G16_UINT: return WINED3DFMT_R16G16_UINT;
        case DXGI_FORMAT_R16G16_SNORM: return WINED3DFMT_R16G16_SNORM;
        case DXGI_FORMAT_R16G16_SINT: return WINED3DFMT_R16G16_SINT;
        case DXGI_FORMAT_R32_TYPELESS: return WINED3DFMT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT: return WINED3DFMT_D32_FLOAT;
        case DXGI_FORMAT_R32_FLOAT: return WINED3DFMT_R32_FLOAT;
        case DXGI_FORMAT_R32_UINT: return WINED3DFMT_R32_UINT;
        case DXGI_FORMAT_R32_SINT: return WINED3DFMT_R32_SINT;
        case DXGI_FORMAT_R24G8_TYPELESS: return WINED3DFMT_R24G8_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return WINED3DFMT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return WINED3DFMT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return WINED3DFMT_X24_TYPELESS_G8_UINT;
        case DXGI_FORMAT_R8G8_TYPELESS: return WINED3DFMT_R8G8_TYPELESS;
        case DXGI_FORMAT_R8G8_UNORM: return WINED3DFMT_R8G8_UNORM;
        case DXGI_FORMAT_R8G8_UINT: return WINED3DFMT_R8G8_UINT;
        case DXGI_FORMAT_R8G8_SNORM: return WINED3DFMT_R8G8_SNORM;
        case DXGI_FORMAT_R8G8_SINT: return WINED3DFMT_R8G8_SINT;
        case DXGI_FORMAT_R16_TYPELESS: return WINED3DFMT_R16_TYPELESS;
        case DXGI_FORMAT_R16_FLOAT: return WINED3DFMT_R16_FLOAT;
        case DXGI_FORMAT_D16_UNORM: return WINED3DFMT_D16_UNORM;
        case DXGI_FORMAT_R16_UNORM: return WINED3DFMT_R16_UNORM;
        case DXGI_FORMAT_R16_UINT: return WINED3DFMT_R16_UINT;
        case DXGI_FORMAT_R16_SNORM: return WINED3DFMT_R16_SNORM;
        case DXGI_FORMAT_R16_SINT: return WINED3DFMT_R16_SINT;
        case DXGI_FORMAT_R8_TYPELESS: return WINED3DFMT_R8_TYPELESS;
        case DXGI_FORMAT_R8_UNORM: return WINED3DFMT_R8_UNORM;
        case DXGI_FORMAT_R8_UINT: return WINED3DFMT_R8_UINT;
        case DXGI_FORMAT_R8_SNORM: return WINED3DFMT_R8_SNORM;
        case DXGI_FORMAT_R8_SINT: return WINED3DFMT_R8_SINT;
        case DXGI_FORMAT_A8_UNORM: return WINED3DFMT_A8_UNORM;
        case DXGI_FORMAT_R1_UNORM: return WINED3DFMT_R1_UNORM;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return WINED3DFMT_R9G9B9E5_SHAREDEXP;
        case DXGI_FORMAT_R8G8_B8G8_UNORM: return WINED3DFMT_R8G8_B8G8_UNORM;
        case DXGI_FORMAT_G8R8_G8B8_UNORM: return WINED3DFMT_G8R8_G8B8_UNORM;
        case DXGI_FORMAT_BC1_TYPELESS: return WINED3DFMT_BC1_TYPELESS;
        case DXGI_FORMAT_BC1_UNORM: return WINED3DFMT_BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return WINED3DFMT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_TYPELESS: return WINED3DFMT_BC2_TYPELESS;
        case DXGI_FORMAT_BC2_UNORM: return WINED3DFMT_BC2_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB: return WINED3DFMT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_TYPELESS: return WINED3DFMT_BC3_TYPELESS;
        case DXGI_FORMAT_BC3_UNORM: return WINED3DFMT_BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return WINED3DFMT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC4_TYPELESS: return WINED3DFMT_BC4_TYPELESS;
        case DXGI_FORMAT_BC4_UNORM: return WINED3DFMT_BC4_UNORM;
        case DXGI_FORMAT_BC4_SNORM: return WINED3DFMT_BC4_SNORM;
        case DXGI_FORMAT_BC5_TYPELESS: return WINED3DFMT_BC5_TYPELESS;
        case DXGI_FORMAT_BC5_UNORM: return WINED3DFMT_BC5_UNORM;
        case DXGI_FORMAT_BC5_SNORM: return WINED3DFMT_BC5_SNORM;
        case DXGI_FORMAT_B5G6R5_UNORM: return WINED3DFMT_B5G6R5_UNORM;
        case DXGI_FORMAT_B5G5R5A1_UNORM: return WINED3DFMT_B5G5R5A1_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return WINED3DFMT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM: return WINED3DFMT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return WINED3DFMT_R10G10B10_XR_BIAS_A2_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS: return WINED3DFMT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return WINED3DFMT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS: return WINED3DFMT_B8G8R8X8_TYPELESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return WINED3DFMT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_BC6H_TYPELESS: return WINED3DFMT_BC6H_TYPELESS;
        case DXGI_FORMAT_BC6H_UF16: return WINED3DFMT_BC6H_UF16;
        case DXGI_FORMAT_BC6H_SF16: return WINED3DFMT_BC6H_SF16;
        case DXGI_FORMAT_BC7_TYPELESS: return WINED3DFMT_BC7_TYPELESS;
        case DXGI_FORMAT_BC7_UNORM: return WINED3DFMT_BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB: return WINED3DFMT_BC7_UNORM_SRGB;
        case DXGI_FORMAT_B4G4R4A4_UNORM: return WINED3DFMT_B4G4R4A4_UNORM;
        default:
            FIXME("Unhandled DXGI_FORMAT %#x.\n", format);
            return WINED3DFMT_UNKNOWN;
    }
}

unsigned int wined3d_getdata_flags_from_d3d11_async_getdata_flags(unsigned int d3d11_flags)
{
    if (d3d11_flags & ~D3D11_ASYNC_GETDATA_DONOTFLUSH)
        FIXME("Unhandled async getdata flags %#x.\n", d3d11_flags);

    if (d3d11_flags & D3D11_ASYNC_GETDATA_DONOTFLUSH)
        return 0;

    return WINED3DGETDATA_FLUSH;
}

DWORD wined3d_usage_from_d3d11(enum D3D11_USAGE usage)
{
    DWORD wined3d_usage = 0;

    if (usage == D3D11_USAGE_DYNAMIC)
        wined3d_usage |= WINED3DUSAGE_DYNAMIC;

    return wined3d_usage;
}

enum D3D11_USAGE d3d11_usage_from_d3d10_usage(enum D3D10_USAGE usage)
{
    switch (usage)
    {
        case D3D10_USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
        case D3D10_USAGE_IMMUTABLE: return D3D11_USAGE_IMMUTABLE;
        case D3D10_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
        case D3D10_USAGE_STAGING: return D3D11_USAGE_STAGING;
        default:
            FIXME("Unhandled usage %#x.\n", usage);
            return D3D11_USAGE_DEFAULT;
    }
}

enum D3D10_USAGE d3d10_usage_from_d3d11_usage(enum D3D11_USAGE usage)
{
    switch (usage)
    {
        case D3D11_USAGE_DEFAULT: return D3D10_USAGE_DEFAULT;
        case D3D11_USAGE_IMMUTABLE: return D3D10_USAGE_IMMUTABLE;
        case D3D11_USAGE_DYNAMIC: return D3D10_USAGE_DYNAMIC;
        case D3D11_USAGE_STAGING: return D3D10_USAGE_STAGING;
        default:
            FIXME("Unhandled usage %#x.\n", usage);
            return D3D10_USAGE_DEFAULT;
    }
}

UINT d3d11_bind_flags_from_d3d10_bind_flags(UINT bind_flags)
{
    static const UINT handled_flags = D3D10_BIND_VERTEX_BUFFER
            | D3D10_BIND_INDEX_BUFFER
            | D3D10_BIND_CONSTANT_BUFFER
            | D3D10_BIND_SHADER_RESOURCE
            | D3D10_BIND_STREAM_OUTPUT
            | D3D10_BIND_RENDER_TARGET
            | D3D10_BIND_DEPTH_STENCIL;
    UINT d3d11_bind_flags = bind_flags & handled_flags;

    if (bind_flags & ~handled_flags)
        FIXME("Unhandled bind flags %#x.\n", bind_flags & ~handled_flags);

    return d3d11_bind_flags;
}

UINT d3d10_bind_flags_from_d3d11_bind_flags(UINT bind_flags)
{
    static const UINT handled_flags = D3D11_BIND_VERTEX_BUFFER
            | D3D11_BIND_INDEX_BUFFER
            | D3D11_BIND_CONSTANT_BUFFER
            | D3D11_BIND_SHADER_RESOURCE
            | D3D11_BIND_STREAM_OUTPUT
            | D3D11_BIND_RENDER_TARGET
            | D3D11_BIND_DEPTH_STENCIL
            | D3D11_BIND_UNORDERED_ACCESS
            | D3D11_BIND_DECODER
            | D3D11_BIND_VIDEO_ENCODER;
    UINT d3d10_bind_flags = bind_flags & handled_flags;

    if (bind_flags & ~handled_flags)
        FIXME("Unhandled bind flags %#x.\n", bind_flags & ~handled_flags);

    return d3d10_bind_flags;
}

UINT d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(UINT cpu_access_flags)
{
    static const UINT handled_flags = D3D10_CPU_ACCESS_WRITE
            | D3D10_CPU_ACCESS_READ;
    UINT d3d11_cpu_access_flags = cpu_access_flags & handled_flags;

    if (cpu_access_flags & ~handled_flags)
        FIXME("Unhandled cpu access flags %#x.\n", cpu_access_flags & ~handled_flags);

    return d3d11_cpu_access_flags;
}

UINT d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(UINT cpu_access_flags)
{
    static const UINT handled_flags = D3D11_CPU_ACCESS_WRITE
            | D3D11_CPU_ACCESS_READ;
    UINT d3d10_cpu_access_flags = cpu_access_flags & handled_flags;

    if (cpu_access_flags & ~handled_flags)
        FIXME("Unhandled cpu access flags %#x.\n", cpu_access_flags & ~handled_flags);

    return d3d10_cpu_access_flags;
}

UINT d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(UINT resource_misc_flags)
{
    static const UINT bitwise_identical_flags = D3D10_RESOURCE_MISC_GENERATE_MIPS
            | D3D10_RESOURCE_MISC_SHARED
            | D3D10_RESOURCE_MISC_TEXTURECUBE;
    const UINT handled_flags = bitwise_identical_flags
            | D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX
            | D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
    UINT d3d11_resource_misc_flags = resource_misc_flags & bitwise_identical_flags;

    if (resource_misc_flags & D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX)
        d3d11_resource_misc_flags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    if (resource_misc_flags & D3D10_RESOURCE_MISC_GDI_COMPATIBLE)
        d3d11_resource_misc_flags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    if (resource_misc_flags & ~handled_flags)
        FIXME("Unhandled resource misc flags %#x.\n", resource_misc_flags & ~handled_flags);

    return d3d11_resource_misc_flags;
}

UINT d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(UINT resource_misc_flags)
{
    static const UINT bitwise_identical_flags = D3D11_RESOURCE_MISC_GENERATE_MIPS
            | D3D11_RESOURCE_MISC_SHARED
            | D3D11_RESOURCE_MISC_TEXTURECUBE;
    const UINT handled_flags = bitwise_identical_flags
            | D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS
            | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
            | D3D11_RESOURCE_MISC_BUFFER_STRUCTURED
            | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
            | D3D11_RESOURCE_MISC_GDI_COMPATIBLE
            | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    UINT d3d10_resource_misc_flags = resource_misc_flags & bitwise_identical_flags;

    if (resource_misc_flags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)
        d3d10_resource_misc_flags |= D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    if (resource_misc_flags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
        d3d10_resource_misc_flags |= D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

    if (resource_misc_flags & ~handled_flags)
        FIXME("Unhandled resource misc flags #%x.\n", resource_misc_flags & ~handled_flags);

    return d3d10_resource_misc_flags;
}

static BOOL d3d11_bind_flags_are_gpu_read_only(UINT bind_flags)
{
    static const BOOL read_only_bind_flags = D3D11_BIND_VERTEX_BUFFER
            | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_CONSTANT_BUFFER
            | D3D11_BIND_SHADER_RESOURCE;

    return !(bind_flags & ~read_only_bind_flags);
}

BOOL validate_d3d11_resource_access_flags(D3D11_RESOURCE_DIMENSION resource_dimension,
        D3D11_USAGE usage, UINT bind_flags, UINT cpu_access_flags, D3D_FEATURE_LEVEL feature_level)
{
    const BOOL is_texture = resource_dimension != D3D11_RESOURCE_DIMENSION_BUFFER;

    switch (usage)
    {
        case D3D11_USAGE_DEFAULT:
            if ((bind_flags == D3D11_BIND_SHADER_RESOURCE && feature_level >= D3D_FEATURE_LEVEL_11_0)
                    || (is_texture && bind_flags == D3D11_BIND_RENDER_TARGET)
                    || bind_flags == D3D11_BIND_UNORDERED_ACCESS)
                break;
            if (cpu_access_flags)
            {
                WARN("Default resources are not CPU accessible.\n");
                return FALSE;
            }
            break;

        case D3D11_USAGE_IMMUTABLE:
            if (!bind_flags)
            {
                WARN("Bind flags must be non-zero for immutable resources.\n");
                return FALSE;
            }
            if (!d3d11_bind_flags_are_gpu_read_only(bind_flags))
            {
                WARN("Immutable resources cannot be writable by GPU.\n");
                return FALSE;
            }

            if (cpu_access_flags)
            {
                WARN("Immutable resources are not CPU accessible.\n");
                return FALSE;
            }
            break;

        case D3D11_USAGE_DYNAMIC:
            if (!bind_flags)
            {
                WARN("Bind flags must be non-zero for dynamic resources.\n");
                return FALSE;
            }
            if (!d3d11_bind_flags_are_gpu_read_only(bind_flags))
            {
                WARN("Dynamic resources cannot be writable by GPU.\n");
                return FALSE;
            }

            if (cpu_access_flags != D3D11_CPU_ACCESS_WRITE)
            {
                WARN("CPU access must be D3D11_CPU_ACCESS_WRITE for dynamic resources.\n");
                return FALSE;
            }
            break;

        case D3D11_USAGE_STAGING:
            if (bind_flags)
            {
                WARN("Invalid bind flags %#x for staging resources.\n", bind_flags);
                return FALSE;
            }

            if (!cpu_access_flags)
            {
                WARN("CPU access must be non-zero for staging resources.\n");
                return FALSE;
            }
            break;

        default:
            WARN("Invalid usage %#x.\n", usage);
            return FALSE;
    }

    return TRUE;
}

struct wined3d_resource *wined3d_resource_from_d3d11_resource(ID3D11Resource *resource)
{
    D3D11_RESOURCE_DIMENSION dimension;

    ID3D11Resource_GetType(resource, &dimension);

    switch (dimension)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            return wined3d_buffer_get_resource(unsafe_impl_from_ID3D11Buffer(
                    (ID3D11Buffer *)resource)->wined3d_buffer);

        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D11Texture1D(
                    (ID3D11Texture1D *)resource)->wined3d_texture);

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D11Texture2D(
                    (ID3D11Texture2D *)resource)->wined3d_texture);

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D11Texture3D(
                    (ID3D11Texture3D *)resource)->wined3d_texture);

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
            return NULL;
    }
}

struct wined3d_resource *wined3d_resource_from_d3d10_resource(ID3D10Resource *resource)
{
    D3D10_RESOURCE_DIMENSION dimension;

    ID3D10Resource_GetType(resource, &dimension);

    switch (dimension)
    {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            return wined3d_buffer_get_resource(unsafe_impl_from_ID3D10Buffer(
                    (ID3D10Buffer *)resource)->wined3d_buffer);

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D10Texture1D(
                    (ID3D10Texture1D *)resource)->wined3d_texture);

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D10Texture2D(
                    (ID3D10Texture2D *)resource)->wined3d_texture);

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            return wined3d_texture_get_resource(unsafe_impl_from_ID3D10Texture3D(
                    (ID3D10Texture3D *)resource)->wined3d_texture);

        default:
            FIXME("Unhandled resource dimension %#x.\n", dimension);
            return NULL;
    }
}

DWORD wined3d_map_flags_from_d3d11_map_type(D3D11_MAP map_type)
{
    switch (map_type)
    {
        case D3D11_MAP_WRITE:
            return WINED3D_MAP_WRITE;

        case D3D11_MAP_READ_WRITE:
            return WINED3D_MAP_READ | WINED3D_MAP_WRITE;

        case D3D11_MAP_READ:
            return WINED3D_MAP_READ;

        case D3D11_MAP_WRITE_DISCARD:
            return WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD;

        case D3D11_MAP_WRITE_NO_OVERWRITE:
            return WINED3D_MAP_WRITE | WINED3D_MAP_NOOVERWRITE;

        default:
            FIXME("Unhandled map_type %#x.\n", map_type);
            return WINED3D_MAP_READ | WINED3D_MAP_WRITE;
    }
}

DWORD wined3d_clear_flags_from_d3d11_clear_flags(UINT clear_flags)
{
    DWORD wined3d_clear_flags = 0;

    if (clear_flags & D3D11_CLEAR_DEPTH)
        wined3d_clear_flags |= WINED3DCLEAR_ZBUFFER;
    if (clear_flags & D3D11_CLEAR_STENCIL)
        wined3d_clear_flags |= WINED3DCLEAR_STENCIL;

    if (clear_flags & ~(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL))
    {
        FIXME("Unhandled clear flags %#x.\n",
                clear_flags & ~(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL));
    }

    return wined3d_clear_flags;
}

unsigned int wined3d_access_from_d3d11(D3D11_USAGE usage, UINT cpu_access)
{
    unsigned int access;

    access = usage == D3D11_USAGE_STAGING ? WINED3D_RESOURCE_ACCESS_CPU : WINED3D_RESOURCE_ACCESS_GPU;
    if (cpu_access & D3D11_CPU_ACCESS_WRITE)
        access |= WINED3D_RESOURCE_ACCESS_MAP_W;
    if (cpu_access & D3D11_CPU_ACCESS_READ)
        access |= WINED3D_RESOURCE_ACCESS_MAP_R;
    if (cpu_access &= ~(D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ))
        FIXME("Unhandled CPU access flags %#x.\n", cpu_access);

    return access;
}

HRESULT d3d_get_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT *data_size, void *data)
{
    const struct wined3d_private_data *stored_data;
    DWORD size_in;

    if (!data_size)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    if (!(stored_data = wined3d_private_store_get_private_data(store, guid)))
    {
        *data_size = 0;
        wined3d_mutex_unlock();
        return DXGI_ERROR_NOT_FOUND;
    }

    size_in = *data_size;
    *data_size = stored_data->size;
    if (!data)
    {
        wined3d_mutex_unlock();
        return S_OK;
    }
    if (size_in < stored_data->size)
    {
        wined3d_mutex_unlock();
        return DXGI_ERROR_MORE_DATA;
    }

    if (stored_data->flags & WINED3DSPD_IUNKNOWN)
        IUnknown_AddRef(stored_data->content.object);
    memcpy(data, stored_data->content.data, stored_data->size);

    wined3d_mutex_unlock();
    return S_OK;
}

HRESULT d3d_set_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT data_size, const void *data)
{
    struct wined3d_private_data *entry;
    HRESULT hr;

    wined3d_mutex_lock();
    if (!data)
    {
        if (!(entry = wined3d_private_store_get_private_data(store, guid)))
        {
            wined3d_mutex_unlock();
            return S_FALSE;
        }
        wined3d_private_store_free_private_data(store, entry);
        wined3d_mutex_unlock();

        return S_OK;
    }

    hr = wined3d_private_store_set_private_data(store, guid, data, data_size, 0);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT d3d_set_private_data_interface(struct wined3d_private_store *store,
        REFGUID guid, const IUnknown *object)
{
    HRESULT hr;

    if (!object)
        return d3d_set_private_data(store, guid, sizeof(object), &object);

    wined3d_mutex_lock();
    hr = wined3d_private_store_set_private_data(store,
            guid, object, sizeof(object), WINED3DSPD_IUNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}
