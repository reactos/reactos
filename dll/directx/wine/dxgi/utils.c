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

#define WINE_DXGI_TO_STR(x) case x: return #x

static const char *debug_feature_level(D3D_FEATURE_LEVEL feature_level)
{
    switch (feature_level)
    {
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_9_1);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_9_2);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_9_3);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_10_0);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_10_1);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_11_0);
        WINE_DXGI_TO_STR(D3D_FEATURE_LEVEL_11_1);
        default:
            FIXME("Unrecognized D3D_FEATURE_LEVEL %#x.\n", feature_level);
            return "unrecognized";
    }
}

const char *debug_dxgi_format(DXGI_FORMAT format)
{
    switch(format)
    {
        WINE_DXGI_TO_STR(DXGI_FORMAT_UNKNOWN);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32A32_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32A32_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32A32_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32A32_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32B32_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16B16A16_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G32_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32G8X24_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R10G10B10A2_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R10G10B10A2_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R10G10B10A2_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R11G11B10_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8B8A8_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16G16_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_D32_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R32_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R24G8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_D24_UNORM_S8_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_FLOAT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_D16_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R16_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8_UINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8_SINT);
        WINE_DXGI_TO_STR(DXGI_FORMAT_A8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R1_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R8G8_B8G8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_G8R8_G8B8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC1_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC1_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC1_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC2_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC2_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC2_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC3_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC3_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC3_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC4_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC4_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC4_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC5_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC5_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC5_SNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B5G6R5_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B5G5R5A1_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8A8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8X8_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8A8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8X8_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC6H_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC6H_UF16);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC6H_SF16);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC7_TYPELESS);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC7_UNORM);
        WINE_DXGI_TO_STR(DXGI_FORMAT_BC7_UNORM_SRGB);
        WINE_DXGI_TO_STR(DXGI_FORMAT_B4G4R4A4_UNORM);
        default:
            FIXME("Unrecognized DXGI_FORMAT %#x.\n", format);
            return "unrecognized";
    }
}

#undef WINE_DXGI_TO_STR

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

const char *debug_dxgi_mode(const DXGI_MODE_DESC *desc)
{
    if (!desc)
        return "(null)";

    return wine_dbg_sprintf("resolution %ux%u, refresh rate %u / %u, "
            "format %s, scanline ordering %#x, scaling %#x",
            desc->Width, desc->Height, desc->RefreshRate.Numerator, desc->RefreshRate.Denominator,
            debug_dxgi_format(desc->Format), desc->ScanlineOrdering, desc->Scaling);
}

const char *debug_dxgi_mode1(const DXGI_MODE_DESC1 *desc)
{
    if (!desc)
        return "(null)";

    return wine_dbg_sprintf("resolution %ux%u, refresh rate %u / %u, "
            "format %s, scanline ordering %#x, scaling %#x, stereo %#x",
            desc->Width, desc->Height, desc->RefreshRate.Numerator, desc->RefreshRate.Denominator,
            debug_dxgi_format(desc->Format), desc->ScanlineOrdering, desc->Scaling, desc->Stereo);
}

void dump_feature_levels(const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count)
{
    unsigned int i;

    if (!feature_levels || !level_count)
    {
        TRACE("Feature levels: (null).\n");
        return;
    }

    TRACE("Feature levels (count = %u):\n", level_count);
    for (i = 0; i < level_count; ++i)
        TRACE("    [%u] = %s.\n", i, debug_feature_level(feature_levels[i]));
}

static unsigned int dxgi_rational_to_uint(const DXGI_RATIONAL *rational)
{
    if (rational->Denominator)
        return rational->Numerator / rational->Denominator;
    else
        return rational->Numerator;
}

static enum wined3d_scanline_ordering wined3d_scanline_ordering_from_dxgi(DXGI_MODE_SCANLINE_ORDER scanline_order)
{
    switch (scanline_order)
    {
        case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED:
            return WINED3D_SCANLINE_ORDERING_UNKNOWN;
        case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
            return WINED3D_SCANLINE_ORDERING_PROGRESSIVE;
        default:
            FIXME("Unhandled scanline ordering %#x.\n", scanline_order);
            return WINED3D_SCANLINE_ORDERING_UNKNOWN;
    }
}

void dxgi_sample_desc_from_wined3d(DXGI_SAMPLE_DESC *desc,
        enum wined3d_multisample_type wined3d_type, unsigned int wined3d_quality)
{
    desc->Count = wined3d_type == WINED3D_MULTISAMPLE_NONE ? 1 : wined3d_type;
    desc->Quality = wined3d_quality;
}

void wined3d_sample_desc_from_dxgi(enum wined3d_multisample_type *wined3d_type,
        unsigned int *wined3d_quality, const DXGI_SAMPLE_DESC *dxgi_desc)
{
    if (dxgi_desc->Count > 1)
    {
        *wined3d_type = dxgi_desc->Count;
        *wined3d_quality = dxgi_desc->Quality;
    }
    else
    {
        *wined3d_type = WINED3D_MULTISAMPLE_NONE;
        *wined3d_quality = 0;
    }
}

void wined3d_display_mode_from_dxgi(struct wined3d_display_mode *wined3d_mode,
        const DXGI_MODE_DESC *mode)
{
    wined3d_mode->width = mode->Width;
    wined3d_mode->height = mode->Height;
    wined3d_mode->refresh_rate = dxgi_rational_to_uint(&mode->RefreshRate);
    wined3d_mode->format_id = wined3dformat_from_dxgi_format(mode->Format);
    wined3d_mode->scanline_ordering = wined3d_scanline_ordering_from_dxgi(mode->ScanlineOrdering);
}

void wined3d_display_mode_from_dxgi1(struct wined3d_display_mode *wined3d_mode,
        const DXGI_MODE_DESC1 *mode)
{
    wined3d_mode->width = mode->Width;
    wined3d_mode->height = mode->Height;
    wined3d_mode->refresh_rate = dxgi_rational_to_uint(&mode->RefreshRate);
    wined3d_mode->format_id = wined3dformat_from_dxgi_format(mode->Format);
    wined3d_mode->scanline_ordering = wined3d_scanline_ordering_from_dxgi(mode->ScanlineOrdering);
    FIXME("Ignoring stereo %#x.\n", mode->Stereo);
}

DXGI_USAGE dxgi_usage_from_wined3d_bind_flags(unsigned int wined3d_bind_flags)
{
    DXGI_USAGE dxgi_usage = 0;

    if (wined3d_bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        dxgi_usage |= DXGI_USAGE_SHADER_INPUT;
    if (wined3d_bind_flags & WINED3D_BIND_RENDER_TARGET)
        dxgi_usage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;

    wined3d_bind_flags &= ~(WINED3D_BIND_SHADER_RESOURCE | WINED3D_BIND_RENDER_TARGET);
    if (wined3d_bind_flags)
        FIXME("Unhandled wined3d bind flags %#x.\n", wined3d_bind_flags);
    return dxgi_usage;
}

unsigned int wined3d_bind_flags_from_dxgi_usage(DXGI_USAGE dxgi_usage)
{
    unsigned int wined3d_bind_flags = 0;

    if (dxgi_usage & DXGI_USAGE_SHADER_INPUT)
        wined3d_bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
    if (dxgi_usage & DXGI_USAGE_RENDER_TARGET_OUTPUT)
        wined3d_bind_flags |= WINED3D_BIND_RENDER_TARGET;

    dxgi_usage &= ~(DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT);
    if (dxgi_usage)
        FIXME("Unhandled DXGI usage %#x.\n", dxgi_usage);
    return wined3d_bind_flags;
}

#define DXGI_WINED3D_SWAPCHAIN_FLAGS \
        (WINED3D_SWAPCHAIN_USE_CLOSEST_MATCHING_MODE | WINED3D_SWAPCHAIN_RESTORE_WINDOW_RECT | WINED3D_SWAPCHAIN_HOOK)

unsigned int dxgi_swapchain_flags_from_wined3d(unsigned int wined3d_flags)
{
    unsigned int flags = 0;

    wined3d_flags &= ~DXGI_WINED3D_SWAPCHAIN_FLAGS;

    if (wined3d_flags & WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH)
    {
        wined3d_flags &= ~WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH;
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    }

    if (wined3d_flags & WINED3D_SWAPCHAIN_GDI_COMPATIBLE)
    {
        wined3d_flags &= ~WINED3D_SWAPCHAIN_GDI_COMPATIBLE;
        flags |= DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
    }

    if (wined3d_flags)
        FIXME("Unhandled flags %#x.\n", flags);

    return flags;
}

static unsigned int wined3d_swapchain_flags_from_dxgi(unsigned int flags)
{
    unsigned int wined3d_flags = DXGI_WINED3D_SWAPCHAIN_FLAGS; /* WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL? */

    if (flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    {
        flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        wined3d_flags |= WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH;
    }

    if (flags & DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE)
    {
        flags &= ~DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
        wined3d_flags |= WINED3D_SWAPCHAIN_GDI_COMPATIBLE;
    }

    if (flags)
        FIXME("Unhandled flags %#x.\n", flags);

    return wined3d_flags;
}

HRESULT wined3d_swapchain_desc_from_dxgi(struct wined3d_swapchain_desc *wined3d_desc, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *dxgi_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *dxgi_fullscreen_desc)
{
    if (dxgi_desc->Scaling != DXGI_SCALING_STRETCH)
        FIXME("Ignoring scaling %#x.\n", dxgi_desc->Scaling);
    if (dxgi_desc->AlphaMode != DXGI_ALPHA_MODE_IGNORE)
        FIXME("Ignoring alpha mode %#x.\n", dxgi_desc->AlphaMode);
    if (dxgi_fullscreen_desc && dxgi_fullscreen_desc->ScanlineOrdering)
        FIXME("Unhandled scanline ordering %#x.\n", dxgi_fullscreen_desc->ScanlineOrdering);
    if (dxgi_fullscreen_desc && dxgi_fullscreen_desc->Scaling)
        FIXME("Unhandled mode scaling %#x.\n", dxgi_fullscreen_desc->Scaling);

    switch (dxgi_desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_DISCARD:
            wined3d_desc->swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
            break;
        case DXGI_SWAP_EFFECT_SEQUENTIAL:
            wined3d_desc->swap_effect = WINED3D_SWAP_EFFECT_SEQUENTIAL;
            break;
        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
            wined3d_desc->swap_effect = WINED3D_SWAP_EFFECT_FLIP_DISCARD;
            break;
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
            wined3d_desc->swap_effect = WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL;
            break;
        default:
            WARN("Invalid swap effect %#x.\n", dxgi_desc->SwapEffect);
            return DXGI_ERROR_INVALID_CALL;
    }

    wined3d_desc->backbuffer_width = dxgi_desc->Width;
    wined3d_desc->backbuffer_height = dxgi_desc->Height;
    wined3d_desc->backbuffer_format = wined3dformat_from_dxgi_format(dxgi_desc->Format);
    wined3d_desc->backbuffer_count = dxgi_desc->BufferCount;
    wined3d_desc->backbuffer_bind_flags = wined3d_bind_flags_from_dxgi_usage(dxgi_desc->BufferUsage);
    wined3d_sample_desc_from_dxgi(&wined3d_desc->multisample_type,
            &wined3d_desc->multisample_quality, &dxgi_desc->SampleDesc);
    wined3d_desc->device_window = window;
    wined3d_desc->windowed = dxgi_fullscreen_desc ? dxgi_fullscreen_desc->Windowed : TRUE;
    wined3d_desc->enable_auto_depth_stencil = FALSE;
    wined3d_desc->auto_depth_stencil_format = 0;
    wined3d_desc->flags = wined3d_swapchain_flags_from_dxgi(dxgi_desc->Flags);
    wined3d_desc->refresh_rate = dxgi_fullscreen_desc ? dxgi_rational_to_uint(&dxgi_fullscreen_desc->RefreshRate) : 0;
    wined3d_desc->auto_restore_display_mode = TRUE;

    return S_OK;
}

HRESULT dxgi_get_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT *data_size, void *data)
{
    const struct wined3d_private_data *stored_data;
    DWORD size_in;
    HRESULT hr;

    if (!data_size)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    if (!(stored_data = wined3d_private_store_get_private_data(store, guid)))
    {
        hr = DXGI_ERROR_NOT_FOUND;
        *data_size = 0;
        goto done;
    }

    size_in = *data_size;
    *data_size = stored_data->size;
    if (!data)
    {
        hr = S_OK;
        goto done;
    }
    if (size_in < stored_data->size)
    {
        hr = DXGI_ERROR_MORE_DATA;
        goto done;
    }

    if (stored_data->flags & WINED3DSPD_IUNKNOWN)
        IUnknown_AddRef(stored_data->content.object);
    memcpy(data, stored_data->content.data, stored_data->size);
    hr = S_OK;

done:
    wined3d_mutex_unlock();

    return hr;
}

HRESULT dxgi_set_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT data_size, const void *data)
{
    struct wined3d_private_data *entry;
    HRESULT hr;

    if (!data)
    {
        wined3d_mutex_lock();
        if (!(entry = wined3d_private_store_get_private_data(store, guid)))
        {
            wined3d_mutex_unlock();
            return S_FALSE;
        }

        wined3d_private_store_free_private_data(store, entry);
        wined3d_mutex_unlock();

        return S_OK;
    }

    wined3d_mutex_lock();
    hr = wined3d_private_store_set_private_data(store, guid, data, data_size, 0);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT dxgi_set_private_data_interface(struct wined3d_private_store *store,
        REFGUID guid, const IUnknown *object)
{
    HRESULT hr;

    if (!object)
        return dxgi_set_private_data(store, guid, sizeof(object), &object);

    wined3d_mutex_lock();
    hr = wined3d_private_store_set_private_data(store,
            guid, object, sizeof(object), WINED3DSPD_IUNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}
