/*
 * Utility functions for the WineD3D Library
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
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
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define WINED3D_FORMAT_FOURCC_BASE (WINED3DFMT_BC7_UNORM_SRGB + 1)

static const struct
{
    enum wined3d_format_id id;
    unsigned int idx;
}
format_index_remap[] =
{
    {WINED3DFMT_UYVY,         WINED3D_FORMAT_FOURCC_BASE},
    {WINED3DFMT_YUY2,         WINED3D_FORMAT_FOURCC_BASE + 1},
    {WINED3DFMT_YV12,         WINED3D_FORMAT_FOURCC_BASE + 2},
    {WINED3DFMT_DXT1,         WINED3D_FORMAT_FOURCC_BASE + 3},
    {WINED3DFMT_DXT2,         WINED3D_FORMAT_FOURCC_BASE + 4},
    {WINED3DFMT_DXT3,         WINED3D_FORMAT_FOURCC_BASE + 5},
    {WINED3DFMT_DXT4,         WINED3D_FORMAT_FOURCC_BASE + 6},
    {WINED3DFMT_DXT5,         WINED3D_FORMAT_FOURCC_BASE + 7},
    {WINED3DFMT_MULTI2_ARGB8, WINED3D_FORMAT_FOURCC_BASE + 8},
    {WINED3DFMT_G8R8_G8B8,    WINED3D_FORMAT_FOURCC_BASE + 9},
    {WINED3DFMT_R8G8_B8G8,    WINED3D_FORMAT_FOURCC_BASE + 10},
    {WINED3DFMT_ATI1N,        WINED3D_FORMAT_FOURCC_BASE + 11},
    {WINED3DFMT_ATI2N,        WINED3D_FORMAT_FOURCC_BASE + 12},
    {WINED3DFMT_INST,         WINED3D_FORMAT_FOURCC_BASE + 13},
    {WINED3DFMT_NVDB,         WINED3D_FORMAT_FOURCC_BASE + 14},
    {WINED3DFMT_NVHU,         WINED3D_FORMAT_FOURCC_BASE + 15},
    {WINED3DFMT_NVHS,         WINED3D_FORMAT_FOURCC_BASE + 16},
    {WINED3DFMT_INTZ,         WINED3D_FORMAT_FOURCC_BASE + 17},
    {WINED3DFMT_RESZ,         WINED3D_FORMAT_FOURCC_BASE + 18},
    {WINED3DFMT_NULL,         WINED3D_FORMAT_FOURCC_BASE + 19},
    {WINED3DFMT_R16,          WINED3D_FORMAT_FOURCC_BASE + 20},
    {WINED3DFMT_AL16,         WINED3D_FORMAT_FOURCC_BASE + 21},
    {WINED3DFMT_NV12,         WINED3D_FORMAT_FOURCC_BASE + 22},
};

#define WINED3D_FORMAT_COUNT (WINED3D_FORMAT_FOURCC_BASE + ARRAY_SIZE(format_index_remap))

struct wined3d_format_channels
{
    enum wined3d_format_id id;
    DWORD red_size, green_size, blue_size, alpha_size;
    DWORD red_offset, green_offset, blue_offset, alpha_offset;
    UINT bpp;
    BYTE depth_size, stencil_size;
};

static const struct wined3d_format_channels formats[] =
{
    /*                                          size            offset
     *  format id                           r   g   b   a    r   g   b   a    bpp depth stencil */
    {WINED3DFMT_UNKNOWN,                    0,  0,  0,  0,   0,  0,  0,  0,    0,   0,     0},
    /* FourCC formats */
    {WINED3DFMT_UYVY,                       0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_YUY2,                       0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_YV12,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_NV12,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_DXT1,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_DXT2,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_DXT3,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_DXT4,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_DXT5,                       0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_MULTI2_ARGB8,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_G8R8_G8B8,                  0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_R8G8_B8G8,                  0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    /* Hmm? */
    {WINED3DFMT_R8G8_SNORM_Cx,              0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_R11G11B10_FLOAT,           11, 11, 10,  0,   0, 11, 22,  0,    4,   0,     0},
    /* Palettized formats */
    {WINED3DFMT_P8_UINT_A8_UNORM,           0,  0,  0,  8,   0,  0,  0,  8,    2,   0,     0},
    {WINED3DFMT_P8_UINT,                    0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    /* Standard ARGB formats. */
    {WINED3DFMT_B8G8R8_UNORM,               8,  8,  8,  0,  16,  8,  0,  0,    3,   0,     0},
    {WINED3DFMT_B5G6R5_UNORM,               5,  6,  5,  0,  11,  5,  0,  0,    2,   0,     0},
    {WINED3DFMT_B5G5R5X1_UNORM,             5,  5,  5,  0,  10,  5,  0,  0,    2,   0,     0},
    {WINED3DFMT_B5G5R5A1_UNORM,             5,  5,  5,  1,  10,  5,  0, 15,    2,   0,     0},
    {WINED3DFMT_B4G4R4A4_UNORM,             4,  4,  4,  4,   8,  4,  0, 12,    2,   0,     0},
    {WINED3DFMT_B2G3R3_UNORM,               3,  3,  2,  0,   5,  2,  0,  0,    1,   0,     0},
    {WINED3DFMT_A8_UNORM,                   0,  0,  0,  8,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_B2G3R3A8_UNORM,             3,  3,  2,  8,   5,  2,  0,  8,    2,   0,     0},
    {WINED3DFMT_B4G4R4X4_UNORM,             4,  4,  4,  0,   8,  4,  0,  0,    2,   0,     0},
    {WINED3DFMT_R8G8B8X8_UNORM,             8,  8,  8,  0,   0,  8, 16,  0,    4,   0,     0},
    {WINED3DFMT_B10G10R10A2_UNORM,         10, 10, 10,  2,  20, 10,  0, 30,    4,   0,     0},
    /* Luminance */
    {WINED3DFMT_L8_UNORM,                   0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_L8A8_UNORM,                 0,  0,  0,  8,   0,  0,  0,  8,    2,   0,     0},
    {WINED3DFMT_L4A4_UNORM,                 0,  0,  0,  4,   0,  0,  0,  4,    1,   0,     0},
    {WINED3DFMT_L16_UNORM,                  0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    /* Bump mapping stuff */
    {WINED3DFMT_R5G5_SNORM_L6_UNORM,        5,  5,  0,  0,   0,  5,  0,  0,    2,   0,     0},
    {WINED3DFMT_R8G8_SNORM_L8X8_UNORM,      8,  8,  0,  0,   0,  8,  0,  0,    4,   0,     0},
    {WINED3DFMT_R10G11B11_SNORM,           10, 11, 11,  0,   0, 10, 21,  0,    4,   0,     0},
    {WINED3DFMT_R10G10B10_SNORM_A2_UNORM,  10, 10, 10,  2,   0, 10, 20, 30,    4,   0,     0},
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE,               0,  0,  0,  0,   0,  0,  0,  0,    2,  16,     0},
    {WINED3DFMT_D32_UNORM,                  0,  0,  0,  0,   0,  0,  0,  0,    4,  32,     0},
    {WINED3DFMT_S1_UINT_D15_UNORM,          0,  0,  0,  0,   0,  0,  0,  0,    2,  15,     1},
    {WINED3DFMT_X8D24_UNORM,                0,  0,  0,  0,   0,  0,  0,  0,    4,  24,     0},
    {WINED3DFMT_S4X4_UINT_D24_UNORM,        0,  0,  0,  0,   0,  0,  0,  0,    4,  24,     4},
    {WINED3DFMT_S8_UINT_D24_FLOAT,          0,  0,  0,  0,   0,  0,  0,  0,    4,  24,     8},
    /* Vendor-specific formats */
    {WINED3DFMT_ATI1N,                      0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_ATI2N,                      0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_NVDB,                       0,  0,  0,  0,   0,  0,  0,  0,    0,   0,     0},
    {WINED3DFMT_INST,                       0,  0,  0,  0,   0,  0,  0,  0,    0,   0,     0},
    {WINED3DFMT_INTZ,                       0,  0,  0,  0,   0,  0,  0,  0,    4,  24,     8},
    {WINED3DFMT_RESZ,                       0,  0,  0,  0,   0,  0,  0,  0,    0,   0,     0},
    {WINED3DFMT_NVHU,                       0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_NVHS,                       0,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_NULL,                       8,  8,  8,  8,   0,  8, 16, 24,    4,   0,     0},
    /* Unsure about them, could not find a Windows driver that supports them */
    {WINED3DFMT_R16,                       16,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_AL16,                       0,  0,  0, 16,   0,  0,  0, 16,    4,   0,     0},
    /* DirectX 10 HDR formats */
    {WINED3DFMT_R9G9B9E5_SHAREDEXP,         0,  0,  0,  0,   0,  0,  0,  0,    4,   0,     0},
    /* Typeless */
    {WINED3DFMT_R32G32B32A32_TYPELESS,     32, 32, 32, 32,   0, 32, 64, 96,   16,   0,     0},
    {WINED3DFMT_R32G32B32_TYPELESS,        32, 32, 32,  0,   0, 32, 64,  0,   12,   0,     0},
    {WINED3DFMT_R16G16B16A16_TYPELESS,     16, 16, 16, 16,   0, 16, 32, 48,    8,   0,     0},
    {WINED3DFMT_R32G32_TYPELESS,           32, 32,  0,  0,   0, 32,  0,  0,    8,   0,     0},
    {WINED3DFMT_R32G8X24_TYPELESS,          0,  0,  0,  0,   0,  0,  0,  0,    8,  32,     8},
    {WINED3DFMT_R10G10B10A2_TYPELESS,      10, 10, 10,  2,   0, 10, 20, 30,    4,   0,     0},
    {WINED3DFMT_R10G10B10X2_TYPELESS,      10, 10, 10,  0,   0, 10, 20,  0,    4,   0,     0},
    {WINED3DFMT_R8G8B8A8_TYPELESS,          8,  8,  8,  8,   0,  8, 16, 24,    4,   0,     0},
    {WINED3DFMT_R16G16_TYPELESS,           16, 16,  0,  0,   0, 16,  0,  0,    4,   0,     0},
    {WINED3DFMT_R32_TYPELESS,              32,  0,  0,  0,   0,  0,  0,  0,    4,   0,     0},
    {WINED3DFMT_R24G8_TYPELESS,             0,  0,  0,  0,   0,  0,  0,  0,    4,  24,     8},
    {WINED3DFMT_R8G8_TYPELESS,              8,  8,  0,  0,   0,  8,  0,  0,    2,   0,     0},
    {WINED3DFMT_R16_TYPELESS,              16,  0,  0,  0,   0,  0,  0,  0,    2,   0,     0},
    {WINED3DFMT_R8_TYPELESS,                8,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC1_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC2_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC3_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC4_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC5_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC6H_TYPELESS,              0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_BC7_TYPELESS,               0,  0,  0,  0,   0,  0,  0,  0,    1,   0,     0},
    {WINED3DFMT_B8G8R8A8_TYPELESS,          8,  8,  8,  8,  16,  8,  0, 24,    4,   0,     0},
    {WINED3DFMT_B8G8R8X8_TYPELESS,          8,  8,  8,  0,  16,  8,  0,  0,    4,   0,     0},
};

enum wined3d_channel_type
{
    WINED3D_CHANNEL_TYPE_NONE,
    WINED3D_CHANNEL_TYPE_UNORM,
    WINED3D_CHANNEL_TYPE_SNORM,
    WINED3D_CHANNEL_TYPE_UINT,
    WINED3D_CHANNEL_TYPE_SINT,
    WINED3D_CHANNEL_TYPE_FLOAT,
    WINED3D_CHANNEL_TYPE_DEPTH,
    WINED3D_CHANNEL_TYPE_STENCIL,
    WINED3D_CHANNEL_TYPE_UNUSED,
};

struct wined3d_typed_format_info
{
    enum wined3d_format_id id;
    enum wined3d_format_id typeless_id;
    const char *channels;
};

/**
 * The last entry for a given typeless format defines its internal format.
 *
 * u - WINED3D_CHANNEL_TYPE_UNORM
 * i - WINED3D_CHANNEL_TYPE_SNORM
 * U - WINED3D_CHANNEL_TYPE_UINT
 * I - WINED3D_CHANNEL_TYPE_SINT
 * F - WINED3D_CHANNEL_TYPE_FLOAT
 * D - WINED3D_CHANNEL_TYPE_DEPTH
 * S - WINED3D_CHANNEL_TYPE_STENCIL
 * X - WINED3D_CHANNEL_TYPE_UNUSED
 */
static const struct wined3d_typed_format_info typed_formats[] =
{
    {WINED3DFMT_R32G32B32A32_UINT,        WINED3DFMT_R32G32B32A32_TYPELESS, "UUUU"},
    {WINED3DFMT_R32G32B32A32_SINT,        WINED3DFMT_R32G32B32A32_TYPELESS, "IIII"},
    {WINED3DFMT_R32G32B32A32_FLOAT,       WINED3DFMT_R32G32B32A32_TYPELESS, "FFFF"},
    {WINED3DFMT_R32G32B32_UINT,           WINED3DFMT_R32G32B32_TYPELESS,    "UUU"},
    {WINED3DFMT_R32G32B32_SINT,           WINED3DFMT_R32G32B32_TYPELESS,    "III"},
    {WINED3DFMT_R32G32B32_FLOAT,          WINED3DFMT_R32G32B32_TYPELESS,    "FFF"},
    {WINED3DFMT_R16G16B16A16_UNORM,       WINED3DFMT_R16G16B16A16_TYPELESS, "uuuu"},
    {WINED3DFMT_R16G16B16A16_SNORM,       WINED3DFMT_R16G16B16A16_TYPELESS, "iiii"},
    {WINED3DFMT_R16G16B16A16_UINT,        WINED3DFMT_R16G16B16A16_TYPELESS, "UUUU"},
    {WINED3DFMT_R16G16B16A16_SINT,        WINED3DFMT_R16G16B16A16_TYPELESS, "IIII"},
    {WINED3DFMT_R16G16B16A16_FLOAT,       WINED3DFMT_R16G16B16A16_TYPELESS, "FFFF"},
    {WINED3DFMT_R32G32_UINT,              WINED3DFMT_R32G32_TYPELESS,       "UU"},
    {WINED3DFMT_R32G32_SINT,              WINED3DFMT_R32G32_TYPELESS,       "II"},
    {WINED3DFMT_R32G32_FLOAT,             WINED3DFMT_R32G32_TYPELESS,       "FF"},
    {WINED3DFMT_R32_FLOAT_X8X24_TYPELESS, WINED3DFMT_R32G8X24_TYPELESS,     "DX"},
    {WINED3DFMT_X32_TYPELESS_G8X24_UINT,  WINED3DFMT_R32G8X24_TYPELESS,     "XS"},
    {WINED3DFMT_D32_FLOAT_S8X24_UINT,     WINED3DFMT_R32G8X24_TYPELESS,     "DS"},
    {WINED3DFMT_R10G10B10A2_SNORM,        WINED3DFMT_R10G10B10A2_TYPELESS,  "iiii"},
    {WINED3DFMT_R10G10B10A2_UINT,         WINED3DFMT_R10G10B10A2_TYPELESS,  "UUUU"},
    {WINED3DFMT_R10G10B10A2_UNORM,        WINED3DFMT_R10G10B10A2_TYPELESS,  "uuuu"},
    {WINED3DFMT_R10G10B10X2_SNORM,        WINED3DFMT_R10G10B10X2_TYPELESS,  "iiiX"},
    {WINED3DFMT_R10G10B10X2_UINT,         WINED3DFMT_R10G10B10X2_TYPELESS,  "UUUX"},
    {WINED3DFMT_R8G8B8A8_UINT,            WINED3DFMT_R8G8B8A8_TYPELESS,     "UUUU"},
    {WINED3DFMT_R8G8B8A8_SINT,            WINED3DFMT_R8G8B8A8_TYPELESS,     "IIII"},
    {WINED3DFMT_R8G8B8A8_SNORM,           WINED3DFMT_R8G8B8A8_TYPELESS,     "iiii"},
    {WINED3DFMT_R8G8B8A8_UNORM_SRGB,      WINED3DFMT_R8G8B8A8_TYPELESS,     "uuuu"},
    {WINED3DFMT_R8G8B8A8_UNORM,           WINED3DFMT_R8G8B8A8_TYPELESS,     "uuuu"},
    {WINED3DFMT_R16G16_UNORM,             WINED3DFMT_R16G16_TYPELESS,       "uu"},
    {WINED3DFMT_R16G16_SNORM,             WINED3DFMT_R16G16_TYPELESS,       "ii"},
    {WINED3DFMT_R16G16_UINT,              WINED3DFMT_R16G16_TYPELESS,       "UU"},
    {WINED3DFMT_R16G16_SINT,              WINED3DFMT_R16G16_TYPELESS,       "II"},
    {WINED3DFMT_R16G16_FLOAT,             WINED3DFMT_R16G16_TYPELESS,       "FF"},
    {WINED3DFMT_D32_FLOAT,                WINED3DFMT_R32_TYPELESS,          "D"},
    {WINED3DFMT_R32_FLOAT,                WINED3DFMT_R32_TYPELESS,          "F"},
    {WINED3DFMT_R32_UINT,                 WINED3DFMT_R32_TYPELESS,          "U"},
    {WINED3DFMT_R32_SINT,                 WINED3DFMT_R32_TYPELESS,          "I"},
    {WINED3DFMT_R24_UNORM_X8_TYPELESS,    WINED3DFMT_R24G8_TYPELESS,        "DX"},
    {WINED3DFMT_X24_TYPELESS_G8_UINT,     WINED3DFMT_R24G8_TYPELESS,        "XS"},
    {WINED3DFMT_D24_UNORM_S8_UINT,        WINED3DFMT_R24G8_TYPELESS,        "DS"},
    {WINED3DFMT_R8G8_SNORM,               WINED3DFMT_R8G8_TYPELESS,         "ii"},
    {WINED3DFMT_R8G8_UNORM,               WINED3DFMT_R8G8_TYPELESS,         "uu"},
    {WINED3DFMT_R8G8_UINT,                WINED3DFMT_R8G8_TYPELESS,         "UU"},
    {WINED3DFMT_R8G8_SINT,                WINED3DFMT_R8G8_TYPELESS,         "II"},
    {WINED3DFMT_D16_UNORM,                WINED3DFMT_R16_TYPELESS,          "D"},
    {WINED3DFMT_R16_UNORM,                WINED3DFMT_R16_TYPELESS,          "u"},
    {WINED3DFMT_R16_SNORM,                WINED3DFMT_R16_TYPELESS,          "i"},
    {WINED3DFMT_R16_UINT,                 WINED3DFMT_R16_TYPELESS,          "U"},
    {WINED3DFMT_R16_SINT,                 WINED3DFMT_R16_TYPELESS,          "I"},
    {WINED3DFMT_R16_FLOAT,                WINED3DFMT_R16_TYPELESS,          "F"},
    {WINED3DFMT_R8_UNORM,                 WINED3DFMT_R8_TYPELESS,           "u"},
    {WINED3DFMT_R8_SNORM,                 WINED3DFMT_R8_TYPELESS,           "i"},
    {WINED3DFMT_R8_UINT,                  WINED3DFMT_R8_TYPELESS,           "U"},
    {WINED3DFMT_R8_SINT,                  WINED3DFMT_R8_TYPELESS,           "I"},
    {WINED3DFMT_BC1_UNORM_SRGB,           WINED3DFMT_BC1_TYPELESS,          ""},
    {WINED3DFMT_BC1_UNORM,                WINED3DFMT_BC1_TYPELESS,          ""},
    {WINED3DFMT_BC2_UNORM_SRGB,           WINED3DFMT_BC2_TYPELESS,          ""},
    {WINED3DFMT_BC2_UNORM,                WINED3DFMT_BC2_TYPELESS,          ""},
    {WINED3DFMT_BC3_UNORM_SRGB,           WINED3DFMT_BC3_TYPELESS,          ""},
    {WINED3DFMT_BC3_UNORM,                WINED3DFMT_BC3_TYPELESS,          ""},
    {WINED3DFMT_BC4_UNORM,                WINED3DFMT_BC4_TYPELESS,          ""},
    {WINED3DFMT_BC4_SNORM,                WINED3DFMT_BC4_TYPELESS,          ""},
    {WINED3DFMT_BC5_UNORM,                WINED3DFMT_BC5_TYPELESS,          ""},
    {WINED3DFMT_BC5_SNORM,                WINED3DFMT_BC5_TYPELESS,          ""},
    {WINED3DFMT_BC6H_UF16,                WINED3DFMT_BC6H_TYPELESS,         ""},
    {WINED3DFMT_BC6H_SF16,                WINED3DFMT_BC6H_TYPELESS,         ""},
    {WINED3DFMT_BC7_UNORM_SRGB,           WINED3DFMT_BC7_TYPELESS,          ""},
    {WINED3DFMT_BC7_UNORM,                WINED3DFMT_BC7_TYPELESS,          ""},
    {WINED3DFMT_B8G8R8A8_UNORM_SRGB,      WINED3DFMT_B8G8R8A8_TYPELESS,     "uuuu"},
    {WINED3DFMT_B8G8R8A8_UNORM,           WINED3DFMT_B8G8R8A8_TYPELESS,     "uuuu"},
    {WINED3DFMT_B8G8R8X8_UNORM_SRGB,      WINED3DFMT_B8G8R8X8_TYPELESS,     "uuuX"},
    {WINED3DFMT_B8G8R8X8_UNORM,           WINED3DFMT_B8G8R8X8_TYPELESS,     "uuuX"},
};

struct wined3d_typeless_format_depth_stencil_info
{
    enum wined3d_format_id typeless_id;
    enum wined3d_format_id depth_stencil_id;
    enum wined3d_format_id depth_view_id;
    enum wined3d_format_id stencil_view_id;
    BOOL separate_depth_view_format;
};

static const struct wined3d_typeless_format_depth_stencil_info typeless_depth_stencil_formats[] =
{
    {WINED3DFMT_R32G8X24_TYPELESS, WINED3DFMT_D32_FLOAT_S8X24_UINT,
            WINED3DFMT_R32_FLOAT_X8X24_TYPELESS, WINED3DFMT_X32_TYPELESS_G8X24_UINT, TRUE},
    {WINED3DFMT_R24G8_TYPELESS,    WINED3DFMT_D24_UNORM_S8_UINT,
            WINED3DFMT_R24_UNORM_X8_TYPELESS,    WINED3DFMT_X24_TYPELESS_G8_UINT,    TRUE},
    {WINED3DFMT_R32_TYPELESS,      WINED3DFMT_D32_FLOAT, WINED3DFMT_R32_FLOAT},
    {WINED3DFMT_R16_TYPELESS,      WINED3DFMT_D16_UNORM, WINED3DFMT_R16_UNORM},
};

struct wined3d_format_ddi_info
{
    enum wined3d_format_id id;
    D3DDDIFORMAT ddi_format;
};

static const struct wined3d_format_ddi_info ddi_formats[] =
{
    {WINED3DFMT_B8G8R8_UNORM,       D3DDDIFMT_R8G8B8},
    {WINED3DFMT_B8G8R8A8_UNORM,     D3DDDIFMT_A8R8G8B8},
    {WINED3DFMT_B8G8R8X8_UNORM,     D3DDDIFMT_X8R8G8B8},
    {WINED3DFMT_B5G6R5_UNORM,       D3DDDIFMT_R5G6B5},
    {WINED3DFMT_B5G5R5X1_UNORM,     D3DDDIFMT_X1R5G5B5},
    {WINED3DFMT_B5G5R5A1_UNORM,     D3DDDIFMT_A1R5G5B5},
    {WINED3DFMT_B4G4R4A4_UNORM,     D3DDDIFMT_A4R4G4B4},
    {WINED3DFMT_B4G4R4X4_UNORM,     D3DDDIFMT_X4R4G4B4},
    {WINED3DFMT_P8_UINT,            D3DDDIFMT_P8},
};

struct wined3d_format_base_flags
{
    enum wined3d_format_id id;
    DWORD flags;
};

/* The ATI2N format behaves like an uncompressed format in LockRect(), but
 * still needs to use the correct block based calculation for e.g. the
 * resource size. */
static const struct wined3d_format_base_flags format_base_flags[] =
{
    {WINED3DFMT_ATI1N,                WINED3DFMT_FLAG_MAPPABLE | WINED3DFMT_FLAG_BROKEN_PITCH},
    {WINED3DFMT_ATI2N,                WINED3DFMT_FLAG_MAPPABLE | WINED3DFMT_FLAG_BROKEN_PITCH},
    {WINED3DFMT_D16_LOCKABLE,         WINED3DFMT_FLAG_MAPPABLE},
    {WINED3DFMT_INTZ,                 WINED3DFMT_FLAG_MAPPABLE},
    {WINED3DFMT_R11G11B10_FLOAT,      WINED3DFMT_FLAG_FLOAT},
    {WINED3DFMT_D32_FLOAT,            WINED3DFMT_FLAG_FLOAT},
    {WINED3DFMT_S8_UINT_D24_FLOAT,    WINED3DFMT_FLAG_FLOAT},
    {WINED3DFMT_D32_FLOAT_S8X24_UINT, WINED3DFMT_FLAG_FLOAT},
    {WINED3DFMT_INST,                 WINED3DFMT_FLAG_EXTENSION},
    {WINED3DFMT_NULL,                 WINED3DFMT_FLAG_EXTENSION},
    {WINED3DFMT_NVDB,                 WINED3DFMT_FLAG_EXTENSION},
    {WINED3DFMT_RESZ,                 WINED3DFMT_FLAG_EXTENSION},
};

static void rgb888_from_rgb565(WORD rgb565, BYTE *r, BYTE *g, BYTE *b)
{
    BYTE c;

    /* (2⁸ - 1) / (2⁵ - 1) ≈ 2⁸ / 2⁵ + 2⁸ / 2¹⁰
     * (2⁸ - 1) / (2⁶ - 1) ≈ 2⁸ / 2⁶ + 2⁸ / 2¹² */
    c = rgb565 >> 11;
    *r = (c << 3) + (c >> 2);
    c = (rgb565 >> 5) & 0x3f;
    *g = (c << 2) + (c >> 4);
    c = rgb565 & 0x1f;
    *b = (c << 3) + (c >> 2);
}

static void build_dxtn_colour_table(WORD colour0, WORD colour1,
        DWORD colour_table[4], enum wined3d_format_id format_id)
{
    unsigned int i;
    struct
    {
        BYTE r, g, b;
    } c[4];

    rgb888_from_rgb565(colour0, &c[0].r, &c[0].g, &c[0].b);
    rgb888_from_rgb565(colour1, &c[1].r, &c[1].g, &c[1].b);

    if (format_id == WINED3DFMT_BC1_UNORM && colour0 <= colour1)
    {
        c[2].r = (c[0].r + c[1].r) / 2;
        c[2].g = (c[0].g + c[1].g) / 2;
        c[2].b = (c[0].b + c[1].b) / 2;

        c[3].r = 0;
        c[3].g = 0;
        c[3].b = 0;
    }
    else
    {
        for (i = 0; i < 2; ++i)
        {
            c[i + 2].r = (2 * c[i].r + c[1 - i].r) / 3;
            c[i + 2].g = (2 * c[i].g + c[1 - i].g) / 3;
            c[i + 2].b = (2 * c[i].b + c[1 - i].b) / 3;
        }
    }

    for (i = 0; i < 4; ++i)
    {
        colour_table[i] = (c[i].r << 16) | (c[i].g << 8) | c[i].b;
    }
}

static void build_bc3_alpha_table(BYTE alpha0, BYTE alpha1, BYTE alpha_table[8])
{
    unsigned int i;

    alpha_table[0] = alpha0;
    alpha_table[1] = alpha1;

    if (alpha0 > alpha1)
    {
        for (i = 0; i < 6; ++i)
        {
            alpha_table[2 + i] = ((6 - i) * alpha0 + (i + 1) * alpha1) / 7;
        }
        return;
    }
    else
    {
        for (i = 0; i < 4; ++i)
        {
            alpha_table[2 + i] = ((4 - i) * alpha0 + (i + 1) * alpha1) / 5;
        }
        alpha_table[6] = 0x00;
        alpha_table[7] = 0xff;
    }
}

static void decompress_dxtn_block(const BYTE *src, BYTE *dst, unsigned int width,
        unsigned int height, unsigned int dst_row_pitch, enum wined3d_format_id format_id)
{
    const UINT64 *s = (const UINT64 *)src;
    BOOL bc1_alpha = FALSE;
    DWORD colour_table[4];
    BYTE alpha_table[8];
    UINT64 alpha_bits;
    DWORD colour_bits;
    unsigned int x, y;
    BYTE colour_idx;
    DWORD *dst_row;
    BYTE alpha;

    if (format_id == WINED3DFMT_BC1_UNORM)
    {
        WORD colour0, colour1;

        alpha_bits = 0;

        colour0 = s[0] & 0xffff;
        colour1 = (s[0] >> 16) & 0xffff;
        colour_bits = (s[0] >> 32) & 0xffffffff;
        build_dxtn_colour_table(colour0, colour1, colour_table, format_id);
        if (colour0 <= colour1)
            bc1_alpha = TRUE;
    }
    else
    {
        alpha_bits = s[0];
        if (format_id == WINED3DFMT_BC3_UNORM)
        {
            build_bc3_alpha_table(alpha_bits & 0xff, (alpha_bits >> 8) & 0xff, alpha_table);
            alpha_bits >>= 16;
        }

        colour_bits = (s[1] >> 32) & 0xffffffff;
        build_dxtn_colour_table(s[1] & 0xffff, (s[1] >> 16) & 0xffff, colour_table, format_id);
    }

    for (y = 0; y < height; ++y)
    {
        dst_row = (DWORD *)&dst[y * dst_row_pitch];
        for (x = 0; x < width; ++x)
        {
            colour_idx = (colour_bits >> (y * 8 + x * 2)) & 0x3;
            switch (format_id)
            {
                case WINED3DFMT_BC1_UNORM:
                    alpha = bc1_alpha && colour_idx == 3 ? 0x00 : 0xff;
                    break;

                case WINED3DFMT_BC2_UNORM:
                    alpha = (alpha_bits >> (y * 16 + x * 4)) & 0xf;
                    /* (2⁸ - 1) / (2⁴ - 1) ≈ 2⁸ / 2⁴ + 2⁸ / 2⁸ */
                    alpha |= alpha << 4;
                    break;

                case WINED3DFMT_BC3_UNORM:
                    alpha = alpha_table[(alpha_bits >> (y * 12 + x * 3)) & 0x7];
                    break;

                default:
                    alpha = 0xff;
                    break;
            }
            dst_row[x] = (alpha << 24) | colour_table[colour_idx];
        }
    }
}

static void decompress_dxtn(const BYTE *src, BYTE *dst, unsigned int src_row_pitch,
        unsigned int src_slice_pitch, unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth, enum wined3d_format_id format_id)
{
    unsigned int block_byte_count, block_w, block_h;
    const BYTE *src_row, *src_slice = src;
    BYTE *dst_row, *dst_slice = dst;
    unsigned int x, y, z;

    block_byte_count = format_id == WINED3DFMT_BC1_UNORM ? 8 : 16;

    for (z = 0; z < depth; ++z)
    {
        src_row = src_slice;
        dst_row = dst_slice;
        for (y = 0; y < height; y += 4)
        {
            for (x = 0; x < width; x += 4)
            {
                block_w = min(width - x, 4);
                block_h = min(height - y, 4);
                decompress_dxtn_block(&src_row[x * (block_byte_count / 4)],
                        &dst_row[x * 4], block_w, block_h, dst_row_pitch, format_id);
            }
            src_row += src_row_pitch;
            dst_row += dst_row_pitch * 4;
        }
        src_slice += src_slice_pitch;
        dst_slice += dst_slice_pitch;
    }
}

static void decompress_bc3(const BYTE *src, BYTE *dst, unsigned int src_row_pitch,
        unsigned int src_slice_pitch, unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth)
{
    decompress_dxtn(src, dst, src_row_pitch, src_slice_pitch, dst_row_pitch,
            dst_slice_pitch, width, height, depth, WINED3DFMT_BC3_UNORM);
}

static void decompress_bc2(const BYTE *src, BYTE *dst, unsigned int src_row_pitch,
        unsigned int src_slice_pitch, unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth)
{
    decompress_dxtn(src, dst, src_row_pitch, src_slice_pitch, dst_row_pitch,
            dst_slice_pitch, width, height, depth, WINED3DFMT_BC2_UNORM);
}

static void decompress_bc1(const BYTE *src, BYTE *dst, unsigned int src_row_pitch,
        unsigned int src_slice_pitch, unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth)
{
    decompress_dxtn(src, dst, src_row_pitch, src_slice_pitch, dst_row_pitch,
            dst_slice_pitch, width, height, depth, WINED3DFMT_BC1_UNORM);
}

static const struct wined3d_format_decompress_info
{
    enum wined3d_format_id id;
    void (*decompress)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
}
format_decompress_info[] =
{
    {WINED3DFMT_DXT1,      decompress_bc1},
    {WINED3DFMT_DXT2,      decompress_bc2},
    {WINED3DFMT_DXT3,      decompress_bc2},
    {WINED3DFMT_DXT4,      decompress_bc3},
    {WINED3DFMT_DXT5,      decompress_bc3},
    {WINED3DFMT_BC1_UNORM, decompress_bc1},
    {WINED3DFMT_BC2_UNORM, decompress_bc2},
    {WINED3DFMT_BC3_UNORM, decompress_bc3},
};

struct wined3d_format_block_info
{
    enum wined3d_format_id id;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    unsigned int flags;
};

static const struct wined3d_format_block_info format_block_info[] =
{
    {WINED3DFMT_DXT1,      4,  4,  8,  WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_DXT2,      4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_DXT3,      4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_DXT4,      4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_DXT5,      4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC1_UNORM, 4,  4,  8,  WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC2_UNORM, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC3_UNORM, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC4_UNORM, 4,  4,  8,  WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC4_SNORM, 4,  4,  8,  WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC5_UNORM, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC5_SNORM, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC6H_UF16, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC6H_SF16, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_BC7_UNORM, 4,  4,  16, WINED3DFMT_FLAG_COMPRESSED},
    {WINED3DFMT_ATI1N,     4,  4,  8,  WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_BLOCKS_NO_VERIFY},
    {WINED3DFMT_ATI2N,     4,  4,  16, WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_BLOCKS_NO_VERIFY},
    {WINED3DFMT_YUY2,      2,  1,  4,  WINED3DFMT_FLAG_BLOCKS_NO_VERIFY},
    {WINED3DFMT_UYVY,      2,  1,  4,  WINED3DFMT_FLAG_BLOCKS_NO_VERIFY},
};

struct wined3d_format_vertex_info
{
    enum wined3d_format_id id;
    enum wined3d_ffp_emit_idx emit_idx;
    GLenum gl_vtx_type;
    enum wined3d_gl_extension extension;
};

static const struct wined3d_format_vertex_info format_vertex_info[] =
{
    {WINED3DFMT_R32_FLOAT,          WINED3D_FFP_EMIT_FLOAT1,    GL_FLOAT},
    {WINED3DFMT_R32G32_FLOAT,       WINED3D_FFP_EMIT_FLOAT2,    GL_FLOAT},
    {WINED3DFMT_R32G32B32_FLOAT,    WINED3D_FFP_EMIT_FLOAT3,    GL_FLOAT},
    {WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_FFP_EMIT_FLOAT4,    GL_FLOAT},
    {WINED3DFMT_B8G8R8A8_UNORM,     WINED3D_FFP_EMIT_D3DCOLOR,  GL_UNSIGNED_BYTE},
    {WINED3DFMT_R8G8B8A8_UINT,      WINED3D_FFP_EMIT_UBYTE4,    GL_UNSIGNED_BYTE},
    {WINED3DFMT_R16G16_UINT,        WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_SHORT},
    {WINED3DFMT_R16G16_SINT,        WINED3D_FFP_EMIT_SHORT2,    GL_SHORT},
    {WINED3DFMT_R16G16B16A16_SINT,  WINED3D_FFP_EMIT_SHORT4,    GL_SHORT},
    {WINED3DFMT_R8G8B8A8_UNORM,     WINED3D_FFP_EMIT_UBYTE4N,   GL_UNSIGNED_BYTE},
    {WINED3DFMT_R16G16_SNORM,       WINED3D_FFP_EMIT_SHORT2N,   GL_SHORT},
    {WINED3DFMT_R16G16B16A16_SNORM, WINED3D_FFP_EMIT_SHORT4N,   GL_SHORT},
    {WINED3DFMT_R16G16_UNORM,       WINED3D_FFP_EMIT_USHORT2N,  GL_UNSIGNED_SHORT},
    {WINED3DFMT_R16G16B16A16_UNORM, WINED3D_FFP_EMIT_USHORT4N,  GL_UNSIGNED_SHORT},
    {WINED3DFMT_R10G10B10X2_UINT,   WINED3D_FFP_EMIT_UDEC3,     GL_UNSIGNED_SHORT},
    {WINED3DFMT_R10G10B10X2_SNORM,  WINED3D_FFP_EMIT_DEC3N,     GL_SHORT},
    {WINED3DFMT_R10G10B10A2_UNORM,  WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_INT_2_10_10_10_REV,
            ARB_VERTEX_TYPE_2_10_10_10_REV},
    /* Without ARB_half_float_vertex we convert these on upload. */
    {WINED3DFMT_R16G16_FLOAT,       WINED3D_FFP_EMIT_FLOAT16_2, GL_FLOAT},
    {WINED3DFMT_R16G16_FLOAT,       WINED3D_FFP_EMIT_FLOAT16_2, GL_HALF_FLOAT, ARB_HALF_FLOAT_VERTEX},
    {WINED3DFMT_R16G16B16A16_FLOAT, WINED3D_FFP_EMIT_FLOAT16_4, GL_FLOAT},
    {WINED3DFMT_R16G16B16A16_FLOAT, WINED3D_FFP_EMIT_FLOAT16_4, GL_HALF_FLOAT, ARB_HALF_FLOAT_VERTEX},
    {WINED3DFMT_R8G8B8A8_SNORM,     WINED3D_FFP_EMIT_INVALID,   GL_BYTE},
    {WINED3DFMT_R8G8B8A8_SINT,      WINED3D_FFP_EMIT_INVALID,   GL_BYTE},
    {WINED3DFMT_R8G8_UNORM,         WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_BYTE},
    {WINED3DFMT_R16G16B16A16_UINT,  WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_SHORT},
    {WINED3DFMT_R8_UNORM,           WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_BYTE},
    {WINED3DFMT_R8_UINT,            WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_BYTE},
    {WINED3DFMT_R8_SINT,            WINED3D_FFP_EMIT_INVALID,   GL_BYTE},
    {WINED3DFMT_R16_UINT,           WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_SHORT},
    {WINED3DFMT_R16_SINT,           WINED3D_FFP_EMIT_INVALID,   GL_SHORT},
    {WINED3DFMT_R32_UINT,           WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_INT},
    {WINED3DFMT_R32_SINT,           WINED3D_FFP_EMIT_INVALID,   GL_INT},
    {WINED3DFMT_R32G32_UINT,        WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_INT},
    {WINED3DFMT_R32G32_SINT,        WINED3D_FFP_EMIT_INVALID,   GL_INT},
    {WINED3DFMT_R32G32B32_UINT,     WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_INT},
    {WINED3DFMT_R32G32B32A32_UINT,  WINED3D_FFP_EMIT_INVALID,   GL_UNSIGNED_INT},
    {WINED3DFMT_R32G32B32A32_SINT,  WINED3D_FFP_EMIT_INVALID,   GL_INT},
};

struct wined3d_format_texture_info
{
    enum wined3d_format_id id;
    GLint gl_internal;
    GLint gl_srgb_internal;
    GLint gl_rt_internal;
    GLint gl_format;
    GLint gl_type;
    unsigned int conv_byte_count;
    unsigned int flags;
    enum wined3d_gl_extension extension;
    void (*upload)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
    void (*download)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
    void (*decompress)(const BYTE *src, BYTE *dst, unsigned int src_row_pitch, unsigned int src_slice_pitch,
            unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
            unsigned int width, unsigned int height, unsigned int depth);
};

static void convert_l4a4_unorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    /* WINED3DFMT_L4A4_UNORM exists as an internal gl format, but for some reason there is not
     * format+type combination to load it. Thus convert it to A8L8, then load it
     * with A4L4 internal, but A8L8 format+type
     */
    unsigned int x, y, z;
    const unsigned char *Source;
    unsigned char *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = src + z * src_slice_pitch + y * src_row_pitch;
            Dest = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                unsigned char color = (*Source++);
                /* A */ Dest[1] = (color & 0xf0u) << 0;
                /* L */ Dest[0] = (color & 0x0fu) << 4;
                Dest += 2;
            }
        }
    }
}

static void convert_r5g5_snorm_l6_unorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    unsigned char r_in, g_in, l_in;
    const unsigned short *texel_in;
    unsigned short *texel_out;

    /* Emulating signed 5 bit values with unsigned 5 bit values has some precision problems by design:
     * E.g. the signed input value 0 becomes 16. GL normalizes it to 16 / 31 = 0.516. We convert it
     * back to a signed value by subtracting 0.5 and multiplying by 2.0. The resulting value is
     * ((16 / 31) - 0.5) * 2.0 = 0.032, which is quite different from the intended result 0.000. */
    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            texel_out = (unsigned short *) (dst + z * dst_slice_pitch + y * dst_row_pitch);
            texel_in = (const unsigned short *)(src + z * src_slice_pitch + y * src_row_pitch);
            for (x = 0; x < width; x++ )
            {
                l_in = (*texel_in & 0xfc00u) >> 10;
                g_in = (*texel_in & 0x03e0u) >> 5;
                r_in = *texel_in & 0x001fu;

                *texel_out = ((r_in + 16) << 11) | (l_in << 5) | (g_in + 16);
                texel_out++;
                texel_in++;
            }
        }
    }
}

static void convert_r5g5_snorm_l6_unorm_ext(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    unsigned char *texel_out, r_out, g_out, r_in, g_in, l_in;
    const unsigned short *texel_in;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            texel_in = (const unsigned short *)(src + z * src_slice_pitch + y * src_row_pitch);
            texel_out = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                l_in = (*texel_in & 0xfc00u) >> 10;
                g_in = (*texel_in & 0x03e0u) >> 5;
                r_in = *texel_in & 0x001fu;

                r_out = r_in << 3;
                if (!(r_in & 0x10)) /* r > 0 */
                    r_out |= r_in >> 1;

                g_out = g_in << 3;
                if (!(g_in & 0x10)) /* g > 0 */
                    g_out |= g_in >> 1;

                texel_out[0] = r_out;
                texel_out[1] = g_out;
                texel_out[2] = l_in << 1 | l_in >> 5;
                texel_out[3] = 0;

                texel_out += 4;
                texel_in++;
            }
        }
    }
}

static void convert_r5g5_snorm_l6_unorm_nv(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    unsigned char *texel_out, ds_out, dt_out, r_in, g_in, l_in;
    const unsigned short *texel_in;

    /* This makes the gl surface bigger(24 bit instead of 16), but it works with
     * fixed function and shaders without further conversion once the surface is
     * loaded.
     *
     * The difference between this function and convert_r5g5_snorm_l6_unorm_ext
     * is that convert_r5g5_snorm_l6_unorm_ext creates a 32 bit XRGB texture and
     * this function creates a 24 bit DSDT_MAG texture. Trying to load a DSDT_MAG
     * internal with a 32 bit DSDT_MAG_INTENSITY or DSDT_MAG_VIB format fails. */
    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            texel_in = (const unsigned short *)(src + z * src_slice_pitch + y * src_row_pitch);
            texel_out = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                l_in = (*texel_in & 0xfc00u) >> 10;
                g_in = (*texel_in & 0x03e0u) >> 5;
                r_in = *texel_in & 0x001fu;

                ds_out = r_in << 3;
                if (!(r_in & 0x10)) /* r > 0 */
                    ds_out |= r_in >> 1;

                dt_out = g_in << 3;
                if (!(g_in & 0x10)) /* g > 0 */
                    dt_out |= g_in >> 1;

                texel_out[0] = ds_out;
                texel_out[1] = dt_out;
                texel_out[2] = l_in << 1 | l_in >> 5;

                texel_out += 3;
                texel_in++;
            }
        }
    }
}

static void convert_r8g8_snorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const short *Source;
    unsigned char *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const short *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                const short color = (*Source++);
                /* B */ Dest[0] = 0xff;
                /* G */ Dest[1] = (color >> 8) + 128; /* V */
                /* R */ Dest[2] = (color & 0xff) + 128;      /* U */
                Dest += 3;
            }
        }
    }
}

static void convert_r8g8_snorm_l8x8_unorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const DWORD *Source;
    unsigned char *Dest;

    /* Doesn't work correctly with the fixed function pipeline, but can work in
     * shaders if the shader is adjusted. (There's no use for this format in gl's
     * standard fixed function pipeline anyway).
     */
    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                LONG color = (*Source++);
                /* B */ Dest[0] = ((color >> 16) & 0xff);       /* L */
                /* G */ Dest[1] = ((color >> 8 ) & 0xff) + 128; /* V */
                /* R */ Dest[2] = (color         & 0xff) + 128; /* U */
                Dest += 4;
            }
        }
    }
}

static void convert_r8g8_snorm_l8x8_unorm_nv(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const DWORD *Source;
    unsigned char *Dest;

    /* This implementation works with the fixed function pipeline and shaders
     * without further modification after converting the surface.
     */
    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                LONG color = (*Source++);
                /* L */ Dest[2] = ((color >> 16) & 0xff);   /* L */
                /* V */ Dest[1] = ((color >> 8 ) & 0xff);   /* V */
                /* U */ Dest[0] = (color         & 0xff);   /* U */
                /* I */ Dest[3] = 255;                      /* X */
                Dest += 4;
            }
        }
    }
}

static void convert_r8g8b8a8_snorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const DWORD *Source;
    unsigned char *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = dst + z * dst_slice_pitch + y * dst_row_pitch;
            for (x = 0; x < width; x++ )
            {
                LONG color = (*Source++);
                /* B */ Dest[0] = ((color >> 16) & 0xff) + 128; /* W */
                /* G */ Dest[1] = ((color >> 8 ) & 0xff) + 128; /* V */
                /* R */ Dest[2] = (color         & 0xff) + 128; /* U */
                /* A */ Dest[3] = ((color >> 24) & 0xff) + 128; /* Q */
                Dest += 4;
            }
        }
    }
}

static void convert_r16g16_snorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const DWORD *Source;
    unsigned short *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = (unsigned short *) (dst + z * dst_slice_pitch + y * dst_row_pitch);
            for (x = 0; x < width; x++ )
            {
                const DWORD color = (*Source++);
                /* B */ Dest[0] = 0xffff;
                /* G */ Dest[1] = (color >> 16) + 32768; /* V */
                /* R */ Dest[2] = (color & 0xffff) + 32768; /* U */
                Dest += 3;
            }
        }
    }
}

static void convert_r16g16(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const WORD *Source;
    WORD *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const WORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = (WORD *) (dst + z * dst_slice_pitch + y * dst_row_pitch);
            for (x = 0; x < width; x++ )
            {
                WORD green = (*Source++);
                WORD red = (*Source++);
                Dest[0] = green;
                Dest[1] = red;
                /* Strictly speaking not correct for R16G16F, but it doesn't matter because the
                 * shader overwrites it anyway */
                Dest[2] = 0xffff;
                Dest += 3;
            }
        }
    }
}

static void convert_r32g32_float(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;
    const float *Source;
    float *Dest;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; y++)
        {
            Source = (const float *)(src + z * src_slice_pitch + y * src_row_pitch);
            Dest = (float *) (dst + z * dst_slice_pitch + y * dst_row_pitch);
            for (x = 0; x < width; x++ )
            {
                float green = (*Source++);
                float red = (*Source++);
                Dest[0] = green;
                Dest[1] = red;
                Dest[2] = 1.0f;
                Dest += 3;
            }
        }
    }
}

static void convert_s1_uint_d15_unorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; ++y)
        {
            const WORD *source = (const WORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            DWORD *dest = (DWORD *)(dst + z * dst_slice_pitch + y * dst_row_pitch);

            for (x = 0; x < width; ++x)
            {
                /* The depth data is normalized, so needs to be scaled,
                * the stencil data isn't.  Scale depth data by
                *      (2^24-1)/(2^15-1) ~~ (2^9 + 2^-6). */
                WORD d15 = source[x] >> 1;
                DWORD d24 = (d15 << 9) + (d15 >> 6);
                dest[x] = (d24 << 8) | (source[x] & 0x1);
            }
        }
    }
}

static void convert_s4x4_uint_d24_unorm(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; ++y)
        {
            const DWORD *source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            DWORD *dest = (DWORD *)(dst + z * dst_slice_pitch + y * dst_row_pitch);

            for (x = 0; x < width; ++x)
            {
                /* Just need to clear out the X4 part. */
                dest[x] = source[x] & ~0xf0;
            }
        }
    }
}

static void convert_s8_uint_d24_float(const BYTE *src, BYTE *dst, UINT src_row_pitch, UINT src_slice_pitch,
        UINT dst_row_pitch, UINT dst_slice_pitch, UINT width, UINT height, UINT depth)
{
    unsigned int x, y, z;

    for (z = 0; z < depth; z++)
    {
        for (y = 0; y < height; ++y)
        {
            const DWORD *source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            float *dest_f = (float *)(dst + z * dst_slice_pitch + y * dst_row_pitch);
            DWORD *dest_s = (DWORD *)dest_f;

            for (x = 0; x < width; ++x)
            {
                dest_f[x * 2] = float_24_to_32((source[x] & 0xffffff00u) >> 8);
                dest_s[x * 2 + 1] = source[x] & 0xff;
            }
        }
    }
}

static void x8_d24_unorm_upload(const BYTE *src, BYTE *dst,
        unsigned int src_row_pitch, unsigned int src_slice_pitch,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth)
{
    unsigned int x, y, z;

    for (z = 0; z < depth; ++z)
    {
        for (y = 0; y < height; ++y)
        {
            const DWORD *source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            DWORD *dest = (DWORD *)(dst + z * dst_slice_pitch + y * dst_row_pitch);

            for (x = 0; x < width; ++x)
            {
                dest[x] = source[x] << 8 | ((source[x] >> 16) & 0xff);
            }
        }
    }
}

static void x8_d24_unorm_download(const BYTE *src, BYTE *dst,
        unsigned int src_row_pitch, unsigned int src_slice_pitch,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch,
        unsigned int width, unsigned int height, unsigned int depth)
{
    unsigned int x, y, z;

    for (z = 0; z < depth; ++z)
    {
        for (y = 0; y < height; ++y)
        {
            const DWORD *source = (const DWORD *)(src + z * src_slice_pitch + y * src_row_pitch);
            DWORD *dest = (DWORD *)(dst + z * dst_slice_pitch + y * dst_row_pitch);

            for (x = 0; x < width; ++x)
            {
                dest[x] = source[x] >> 8;
            }
        }
    }
}

static BOOL color_in_range(const struct wined3d_color_key *color_key, DWORD color)
{
    /* FIXME: Is this really how color keys are supposed to work? I think it
     * makes more sense to compare the individual channels. */
    return color >= color_key->color_space_low_value
            && color <= color_key->color_space_high_value;
}

static void convert_b5g6r5_unorm_b5g5r5a1_unorm_color_key(const BYTE *src, unsigned int src_pitch,
        BYTE *dst, unsigned int dst_pitch, unsigned int width, unsigned int height,
        const struct wined3d_color_key *color_key)
{
    const WORD *src_row;
    unsigned int x, y;
    WORD *dst_row;

    for (y = 0; y < height; ++y)
    {
        src_row = (WORD *)&src[src_pitch * y];
        dst_row = (WORD *)&dst[dst_pitch * y];
        for (x = 0; x < width; ++x)
        {
            WORD src_color = src_row[x];
            if (!color_in_range(color_key, src_color))
                dst_row[x] = 0x8000u | ((src_color & 0xffc0u) >> 1) | (src_color & 0x1fu);
            else
                dst_row[x] = ((src_color & 0xffc0u) >> 1) | (src_color & 0x1fu);
        }
    }
}

static void convert_b5g5r5x1_unorm_b5g5r5a1_unorm_color_key(const BYTE *src, unsigned int src_pitch,
        BYTE *dst, unsigned int dst_pitch, unsigned int width, unsigned int height,
        const struct wined3d_color_key *color_key)
{
    const WORD *src_row;
    unsigned int x, y;
    WORD *dst_row;

    for (y = 0; y < height; ++y)
    {
        src_row = (WORD *)&src[src_pitch * y];
        dst_row = (WORD *)&dst[dst_pitch * y];
        for (x = 0; x < width; ++x)
        {
            WORD src_color = src_row[x];
            if (color_in_range(color_key, src_color))
                dst_row[x] = src_color & ~0x8000;
            else
                dst_row[x] = src_color | 0x8000;
        }
    }
}

static void convert_b8g8r8_unorm_b8g8r8a8_unorm_color_key(const BYTE *src, unsigned int src_pitch,
        BYTE *dst, unsigned int dst_pitch, unsigned int width, unsigned int height,
        const struct wined3d_color_key *color_key)
{
    const BYTE *src_row;
    unsigned int x, y;
    DWORD *dst_row;

    for (y = 0; y < height; ++y)
    {
        src_row = &src[src_pitch * y];
        dst_row = (DWORD *)&dst[dst_pitch * y];
        for (x = 0; x < width; ++x)
        {
            DWORD src_color = (src_row[x * 3 + 2] << 16) | (src_row[x * 3 + 1] << 8) | src_row[x * 3];
            if (!color_in_range(color_key, src_color))
                dst_row[x] = src_color | 0xff000000;
        }
    }
}

static void convert_b8g8r8x8_unorm_b8g8r8a8_unorm_color_key(const BYTE *src, unsigned int src_pitch,
        BYTE *dst, unsigned int dst_pitch, unsigned int width, unsigned int height,
        const struct wined3d_color_key *color_key)
{
    const DWORD *src_row;
    unsigned int x, y;
    DWORD *dst_row;

    for (y = 0; y < height; ++y)
    {
        src_row = (DWORD *)&src[src_pitch * y];
        dst_row = (DWORD *)&dst[dst_pitch * y];
        for (x = 0; x < width; ++x)
        {
            DWORD src_color = src_row[x];
            if (color_in_range(color_key, src_color))
                dst_row[x] = src_color & ~0xff000000;
            else
                dst_row[x] = src_color | 0xff000000;
        }
    }
}

static void convert_b8g8r8a8_unorm_b8g8r8a8_unorm_color_key(const BYTE *src, unsigned int src_pitch,
        BYTE *dst, unsigned int dst_pitch, unsigned int width, unsigned int height,
        const struct wined3d_color_key *color_key)
{
    const DWORD *src_row;
    unsigned int x, y;
    DWORD *dst_row;

    for (y = 0; y < height; ++y)
    {
        src_row = (DWORD *)&src[src_pitch * y];
        dst_row = (DWORD *)&dst[dst_pitch * y];
        for (x = 0; x < width; ++x)
        {
            DWORD src_color = src_row[x];
            if (color_in_range(color_key, src_color))
                src_color &= ~0xff000000;
            dst_row[x] = src_color;
        }
    }
}

const struct wined3d_color_key_conversion * wined3d_format_get_color_key_conversion(
        const struct wined3d_texture *texture, BOOL need_alpha_ck)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int i;

    static const struct
    {
        enum wined3d_format_id src_format;
        struct wined3d_color_key_conversion conversion;
    }
    color_key_info[] =
    {
        {WINED3DFMT_B5G6R5_UNORM,   {WINED3DFMT_B5G5R5A1_UNORM, convert_b5g6r5_unorm_b5g5r5a1_unorm_color_key   }},
        {WINED3DFMT_B5G5R5X1_UNORM, {WINED3DFMT_B5G5R5A1_UNORM, convert_b5g5r5x1_unorm_b5g5r5a1_unorm_color_key }},
        {WINED3DFMT_B8G8R8_UNORM,   {WINED3DFMT_B8G8R8A8_UNORM, convert_b8g8r8_unorm_b8g8r8a8_unorm_color_key   }},
        {WINED3DFMT_B8G8R8X8_UNORM, {WINED3DFMT_B8G8R8A8_UNORM, convert_b8g8r8x8_unorm_b8g8r8a8_unorm_color_key }},
        {WINED3DFMT_B8G8R8A8_UNORM, {WINED3DFMT_B8G8R8A8_UNORM, convert_b8g8r8a8_unorm_b8g8r8a8_unorm_color_key }},
    };

    if (need_alpha_ck && (texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY))
    {
        for (i = 0; i < ARRAY_SIZE(color_key_info); ++i)
        {
            if (color_key_info[i].src_format == format->id)
                return &color_key_info[i].conversion;
        }

        FIXME("Color-keying not supported with format %s.\n", debug_d3dformat(format->id));
    }

    return NULL;
}

/* We intentionally don't support WINED3DFMT_D32_UNORM. No hardware driver
 * supports it, and applications get confused when we do.
 *
 * The following formats explicitly don't have WINED3DFMT_FLAG_TEXTURE set:
 *
 * These are never supported on native.
 *     WINED3DFMT_B8G8R8_UNORM
 *     WINED3DFMT_B2G3R3_UNORM
 *     WINED3DFMT_L4A4_UNORM
 *     WINED3DFMT_S1_UINT_D15_UNORM
 *     WINED3DFMT_S4X4_UINT_D24_UNORM
 *
 * Only some Geforce/Voodoo3/G400 cards offer 8-bit textures in case of ddraw.
 * Since it is not widely available, don't offer it. Further no Windows driver
 * offers WINED3DFMT_P8_UINT_A8_NORM, so don't offer it either.
 *     WINED3DFMT_P8_UINT
 *     WINED3DFMT_P8_UINT_A8_UNORM
 *
 * These formats seem to be similar to the HILO formats in
 * GL_NV_texture_shader. NVHU is said to be GL_UNSIGNED_HILO16,
 * NVHS GL_SIGNED_HILO16. Rumours say that D3D computes a 3rd channel
 * similarly to D3DFMT_CxV8U8 (So NVHS could be called D3DFMT_CxV16U16). ATI
 * refused to support formats which can easily be emulated with pixel shaders,
 * so applications have to deal with not having NVHS and NVHU.
 *     WINED3DFMT_NVHU
 *     WINED3DFMT_NVHS */
static const struct wined3d_format_texture_info format_texture_info[] =
{
    /* format id                        gl_internal                       gl_srgb_internal                      gl_rt_internal
            gl_format                   gl_type                           conv_byte_count
            flags
            extension                   upload                            download */
    /* FourCC formats */
    /* GL_APPLE_ycbcr_422 claims that its '2YUV' format, which is supported via the UNSIGNED_SHORT_8_8_REV_APPLE type
     * is equivalent to 'UYVY' format on Windows, and the 'YUVS' via UNSIGNED_SHORT_8_8_APPLE equates to 'YUY2'. The
     * d3d9 test however shows that the opposite is true. Since the extension is from 2002, it predates the x86 based
     * Macs, so probably the endianness differs. This could be tested as soon as we have a Windows and MacOS on a big
     * endian machine
     */
    {WINED3DFMT_UYVY,                   GL_RG8,                           GL_RG8,                                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_UYVY,                   GL_LUMINANCE8_ALPHA8,             GL_LUMINANCE8_ALPHA8,                   0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_UYVY,                   GL_RGB,                           GL_RGB,                                 0,
            GL_YCBCR_422_APPLE,         GL_UNSIGNED_SHORT_8_8_APPLE,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_FILTERING,
            APPLE_YCBCR_422,            NULL},
    {WINED3DFMT_YUY2,                   GL_RG8,                           GL_RG8,                                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_YUY2,                   GL_LUMINANCE8_ALPHA8,             GL_LUMINANCE8_ALPHA8,                   0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_YUY2,                   GL_RGB,                           GL_RGB,                                 0,
            GL_YCBCR_422_APPLE,         GL_UNSIGNED_SHORT_8_8_REV_APPLE,  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_FILTERING,
            APPLE_YCBCR_422,            NULL},
    {WINED3DFMT_YV12,                   GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_YV12,                   GL_ALPHA8,                        GL_ALPHA8,                              0,
            GL_ALPHA,                   GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_NV12,                   GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_NV12,                   GL_ALPHA8,                        GL_ALPHA8,                              0,
            GL_ALPHA,                   GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_DXT1,                   GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_DXT2,                   GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_DXT3,                   GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_DXT4,                   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_DXT5,                   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_BC1_UNORM,              GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_BC2_UNORM,              GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_BC3_UNORM,              GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            EXT_TEXTURE_COMPRESSION_S3TC, NULL},
    {WINED3DFMT_BC4_UNORM,              GL_COMPRESSED_RED_RGTC1,          GL_COMPRESSED_RED_RGTC1,                0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_BC4_SNORM,              GL_COMPRESSED_SIGNED_RED_RGTC1,   GL_COMPRESSED_SIGNED_RED_RGTC1,         0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_BC5_UNORM,              GL_COMPRESSED_RG_RGTC2,           GL_COMPRESSED_RG_RGTC2,                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_BC5_SNORM,              GL_COMPRESSED_SIGNED_RG_RGTC2,    GL_COMPRESSED_SIGNED_RG_RGTC2,          0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_BC6H_UF16,              GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, 0,
            GL_RGB,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_BPTC, NULL},
    {WINED3DFMT_BC6H_SF16,              GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, 0,
            GL_RGB,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_BPTC, NULL},
    {WINED3DFMT_BC7_UNORM,              GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, 0,
            GL_RGBA,                    GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_BPTC, NULL},
    /* IEEE formats */
    {WINED3DFMT_R32_FLOAT,              GL_RGB32F_ARB,                    GL_RGB32F_ARB,                          0,
            GL_RED,                     GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          NULL},
    {WINED3DFMT_R32_FLOAT,              GL_R32F,                          GL_R32F,                                0,
            GL_RED,                     GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R32G32_FLOAT,           GL_RGB32F_ARB,                    GL_RGB32F_ARB,                          0,
            GL_RGB,                     GL_FLOAT,                         12,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          convert_r32g32_float},
    {WINED3DFMT_R32G32_FLOAT,           GL_RG32F,                         GL_RG32F,                               0,
            GL_RG,                      GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R32G32B32_FLOAT,        GL_RGB32F,                        GL_RGB32F,                              0,
            GL_RGB,                     GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_FLOAT,          NULL},
    {WINED3DFMT_R32G32B32A32_FLOAT,     GL_RGBA32F_ARB,                   GL_RGBA32F_ARB,                         0,
            GL_RGBA,                    GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          NULL},
    /* Float */
    {WINED3DFMT_R16_FLOAT,              GL_RGB16F_ARB,                    GL_RGB16F_ARB,                          0,
            GL_RED,                     GL_HALF_FLOAT_ARB,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          NULL},
    {WINED3DFMT_R16_FLOAT,              GL_R16F,                          GL_R16F,                                0,
            GL_RED,                     GL_HALF_FLOAT_ARB,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16G16_FLOAT,           GL_RGB16F_ARB,                    GL_RGB16F_ARB,                          0,
            GL_RGB,                     GL_HALF_FLOAT_ARB,                6,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          convert_r16g16},
    {WINED3DFMT_R16G16_FLOAT,           GL_RG16F,                         GL_RG16F,                               0,
            GL_RG,                      GL_HALF_FLOAT_ARB,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16G16B16A16_FLOAT,     GL_RGBA16F_ARB,                   GL_RGBA16F_ARB,                         0,
            GL_RGBA,                    GL_HALF_FLOAT_ARB,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_RENDERTARGET
            | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_FLOAT,          NULL},
    {WINED3DFMT_R11G11B10_FLOAT,        GL_R11F_G11F_B10F,                GL_R11F_G11F_B10F,                      0,
            GL_RGB,                     GL_UNSIGNED_INT_10F_11F_11F_REV,  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_PACKED_FLOAT},
    /* Palettized formats */
    {WINED3DFMT_P8_UINT,                GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            0,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_P8_UINT,                GL_ALPHA8,                        GL_ALPHA8,                              0,
            GL_ALPHA,                   GL_UNSIGNED_BYTE,                 0,
            0,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    /* Standard ARGB formats */
    {WINED3DFMT_B8G8R8_UNORM,           GL_RGB8,                          GL_RGB8,                                0,
            GL_BGR,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B8G8R8A8_UNORM,         GL_RGBA8,                         GL_SRGB8_ALPHA8_EXT,                    0,
            GL_BGRA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE
            | WINED3DFMT_FLAG_VTF,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B8G8R8X8_UNORM,         GL_RGB8,                          GL_SRGB8_EXT,                           0,
            GL_BGRA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B5G6R5_UNORM,           GL_RGB5,                          GL_RGB5,                          GL_RGB8,
            GL_RGB,                     GL_UNSIGNED_SHORT_5_6_5,          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B5G6R5_UNORM,           GL_RGB565,                        GL_RGB565,                        GL_RGB8,
            GL_RGB,                     GL_UNSIGNED_SHORT_5_6_5,          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_ES2_COMPATIBILITY,      NULL},
    {WINED3DFMT_B5G5R5X1_UNORM,         GL_RGB5,                          GL_RGB5,                                0,
            GL_BGRA,                    GL_UNSIGNED_SHORT_1_5_5_5_REV,    0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B5G5R5A1_UNORM,         GL_RGB5_A1,                       GL_RGB5_A1,                             0,
            GL_BGRA,                    GL_UNSIGNED_SHORT_1_5_5_5_REV,    0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B4G4R4A4_UNORM,         GL_RGBA4,                         GL_SRGB8_ALPHA8_EXT,                    0,
            GL_BGRA,                    GL_UNSIGNED_SHORT_4_4_4_4_REV,    0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_B2G3R3_UNORM,           GL_R3_G3_B2,                      GL_R3_G3_B2,                            0,
            GL_RGB,                     GL_UNSIGNED_BYTE_3_3_2,           0,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R8_UNORM,               GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_VTF,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_A8_UNORM,               GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_A8_UNORM,               GL_ALPHA8,                        GL_ALPHA8,                              0,
            GL_ALPHA,                   GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_B4G4R4X4_UNORM,         GL_RGB4,                          GL_RGB4,                                0,
            GL_BGRA,                    GL_UNSIGNED_SHORT_4_4_4_4_REV,    0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R10G10B10A2_UINT,       GL_RGB10_A2UI,                    GL_RGB10_A2UI,                          0,
            GL_RGBA_INTEGER,            GL_UNSIGNED_INT_2_10_10_10_REV,   0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RGB10_A2UI,     NULL},
    {WINED3DFMT_R10G10B10A2_UNORM,      GL_RGB10_A2,                      GL_RGB10_A2,                            0,
            GL_RGBA,                    GL_UNSIGNED_INT_2_10_10_10_REV,   0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R8G8B8A8_UNORM,         GL_RGBA8,                         GL_SRGB8_ALPHA8_EXT,                    0,
            GL_RGBA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET |  WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE
            | WINED3DFMT_FLAG_VTF,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R8G8B8A8_UINT,          GL_RGBA8UI,                       GL_RGBA8UI,                             0,
            GL_RGBA_INTEGER,            GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RGB10_A2UI,     NULL},
    {WINED3DFMT_R8G8B8A8_SINT,          GL_RGBA8I,                        GL_RGBA8I,                              0,
            GL_RGBA_INTEGER,            GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_INTEGER,        NULL},
    {WINED3DFMT_R8G8B8X8_UNORM,         GL_RGB8,                          GL_RGB8,                                0,
            GL_RGBA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R16G16_UNORM,           GL_RGB16,                         GL_RGB16,                       GL_RGBA16,
            GL_RGB,                     GL_UNSIGNED_SHORT,                6,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_EXT_NONE,        convert_r16g16},
    {WINED3DFMT_R16G16_UNORM,           GL_RG16,                          GL_RG16,                                0,
            GL_RG,                      GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_B10G10R10A2_UNORM,      GL_RGB10_A2,                      GL_RGB10_A2,                            0,
            GL_BGRA,                    GL_UNSIGNED_INT_2_10_10_10_REV,   0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R16G16B16A16_UNORM,     GL_RGBA16,                        GL_RGBA16,                              0,
            GL_RGBA,                    GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_R8G8_UNORM,             GL_RG8,                           GL_RG8,                                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R8G8_UINT,              GL_RG8UI,                         GL_RG8UI,                               0,
            GL_RG_INTEGER,              GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R8G8_SINT,              GL_RG8I,                          GL_RG8I,                                0,
            GL_RG_INTEGER,              GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16G16B16A16_UINT,      GL_RGBA16UI,                      GL_RGBA16UI,                            0,
            GL_RGBA_INTEGER,            GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_INTEGER,        NULL},
    {WINED3DFMT_R16G16B16A16_SINT,      GL_RGBA16I,                       GL_RGBA16I,                             0,
            GL_RGBA_INTEGER,            GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_INTEGER,        NULL},
    {WINED3DFMT_R32G32_UINT,            GL_RG32UI,                        GL_RG32UI,                              0,
            GL_RG_INTEGER,              GL_UNSIGNED_INT,                  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R32G32_SINT,            GL_RG32I,                         GL_RG32I,                               0,
            GL_RG_INTEGER,              GL_INT,                           0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16G16_UINT,            GL_RG16UI,                        GL_RG16UI,                              0,
            GL_RG_INTEGER,              GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16G16_SINT,            GL_RG16I,                         GL_RG16I,                               0,
            GL_RG_INTEGER,              GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R32_UINT,               GL_R32UI,                         GL_R32UI,                               0,
            GL_RED_INTEGER,             GL_UNSIGNED_INT,                  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R32_SINT,               GL_R32I,                          GL_R32I,                                0,
            GL_RED_INTEGER,             GL_INT,                           0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16_UNORM,              GL_R16,                           GL_R16,                                 0,
            GL_RED,                     GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16_UINT,               GL_R16UI,                         GL_R16UI,                               0,
            GL_RED_INTEGER,             GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R16_SINT,               GL_R16I,                          GL_R16I,                                0,
            GL_RED_INTEGER,             GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R8_UINT,                GL_R8UI,                          GL_R8UI,                                0,
            GL_RED_INTEGER,             GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_R8_SINT,                GL_R8I,                           GL_R8I,                                 0,
            GL_RED_INTEGER,             GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    /* Luminance */
    {WINED3DFMT_L8_UNORM,               GL_LUMINANCE8,                    GL_SLUMINANCE8_EXT,                     0,
            GL_LUMINANCE,               GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_L8_UNORM,               GL_R8,                            GL_R8,                                  0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_L8A8_UNORM,             GL_RG8,                           GL_RG8,                                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             NULL},
    {WINED3DFMT_L8A8_UNORM,             GL_LUMINANCE8_ALPHA8,             GL_SLUMINANCE8_ALPHA8_EXT,              0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_SRGB_READ,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    {WINED3DFMT_L4A4_UNORM,             GL_RG8,                           GL_RG8,                                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 2,
            WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,             convert_l4a4_unorm},
    {WINED3DFMT_L4A4_UNORM,             GL_LUMINANCE4_ALPHA4,             GL_LUMINANCE4_ALPHA4,                   0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 2,
            WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  convert_l4a4_unorm},
    {WINED3DFMT_L16_UNORM,              GL_R16,                           GL_R16,                                 0,
            GL_RED,                     GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_RG,  NULL},
    {WINED3DFMT_L16_UNORM,              GL_LUMINANCE16,                   GL_LUMINANCE16,                         0,
            GL_LUMINANCE,               GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            WINED3D_GL_LEGACY_CONTEXT,  NULL},
    /* Bump mapping stuff */
    {WINED3DFMT_R8G8_SNORM,             GL_RGB8,                          GL_RGB8,                                0,
            GL_BGR,                     GL_UNSIGNED_BYTE,                 3,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            WINED3D_GL_EXT_NONE,        convert_r8g8_snorm},
    {WINED3DFMT_R8G8_SNORM,             GL_DSDT8_NV,                      GL_DSDT8_NV,                            0,
            GL_DSDT_NV,                 GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            NV_TEXTURE_SHADER,          NULL},
    {WINED3DFMT_R8G8_SNORM,             GL_RG8_SNORM,                     GL_RG8_SNORM,                           0,
            GL_RG,                      GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_BUMPMAP,
            EXT_TEXTURE_SNORM,          NULL},
    {WINED3DFMT_R5G5_SNORM_L6_UNORM,    GL_RGB5,                          GL_RGB5,                                0,
            GL_RGB,                     GL_UNSIGNED_SHORT_5_6_5,          2,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            WINED3D_GL_EXT_NONE,        convert_r5g5_snorm_l6_unorm},
    {WINED3DFMT_R5G5_SNORM_L6_UNORM,    GL_DSDT8_MAG8_NV,                 GL_DSDT8_MAG8_NV,                       0,
            GL_DSDT_MAG_NV,             GL_BYTE,                          3,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            NV_TEXTURE_SHADER,          convert_r5g5_snorm_l6_unorm_nv},
    {WINED3DFMT_R5G5_SNORM_L6_UNORM,    GL_RGB8_SNORM,                    GL_RGB8_SNORM,                          0,
            GL_RGBA,                    GL_BYTE,                          4,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            EXT_TEXTURE_SNORM,          convert_r5g5_snorm_l6_unorm_ext},
    {WINED3DFMT_R8G8_SNORM_L8X8_UNORM,  GL_RGB8,                          GL_RGB8,                                0,
            GL_BGRA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      4,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            WINED3D_GL_EXT_NONE,        convert_r8g8_snorm_l8x8_unorm},
    {WINED3DFMT_R8G8_SNORM_L8X8_UNORM,  GL_DSDT8_MAG8_INTENSITY8_NV,      GL_DSDT8_MAG8_INTENSITY8_NV,            0,
            GL_DSDT_MAG_VIB_NV,         GL_UNSIGNED_INT_8_8_S8_S8_REV_NV, 4,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            NV_TEXTURE_SHADER,          convert_r8g8_snorm_l8x8_unorm_nv},
    {WINED3DFMT_R8G8B8A8_SNORM,         GL_RGBA8,                         GL_RGBA8,                               0,
            GL_BGRA,                    GL_UNSIGNED_BYTE,                 4,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            WINED3D_GL_EXT_NONE,        convert_r8g8b8a8_snorm},
    {WINED3DFMT_R8G8B8A8_SNORM,         GL_SIGNED_RGBA8_NV,               GL_SIGNED_RGBA8_NV,                     0,
            GL_RGBA,                    GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            NV_TEXTURE_SHADER,          NULL},
    {WINED3DFMT_R8G8B8A8_SNORM,         GL_RGBA8_SNORM,                   GL_RGBA8_SNORM,                         0,
            GL_RGBA,                    GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_BUMPMAP,
            EXT_TEXTURE_SNORM,          NULL},
    {WINED3DFMT_R16G16_SNORM,           GL_RGB16,                         GL_RGB16,                               0,
            GL_BGR,                     GL_UNSIGNED_SHORT,                6,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            WINED3D_GL_EXT_NONE,        convert_r16g16_snorm},
    {WINED3DFMT_R16G16_SNORM,           GL_SIGNED_HILO16_NV,              GL_SIGNED_HILO16_NV,                    0,
            GL_HILO_NV,                 GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_BUMPMAP,
            NV_TEXTURE_SHADER,          NULL},
    {WINED3DFMT_R16G16_SNORM,           GL_RG16_SNORM,                    GL_RG16_SNORM,                          0,
            GL_RG,                      GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_BUMPMAP,
            EXT_TEXTURE_SNORM,          NULL},
    {WINED3DFMT_R16G16B16A16_SNORM,     GL_RGBA16_SNORM,                  GL_RGBA16_SNORM,                        0,
            GL_RGBA,                    GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_SNORM,          NULL},
    {WINED3DFMT_R16_SNORM,              GL_R16_SNORM,                     GL_R16_SNORM,                           0,
            GL_RED,                     GL_SHORT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_SNORM,          NULL},
    {WINED3DFMT_R8_SNORM,               GL_R8_SNORM,                      GL_R8_SNORM,                           0,
            GL_RED,                     GL_BYTE,                          0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_SNORM,          NULL},
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE,           GL_DEPTH_COMPONENT,               GL_DEPTH_COMPONENT,                     0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_DEPTH,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_D16_LOCKABLE,           GL_DEPTH_COMPONENT16,             GL_DEPTH_COMPONENT16,                   0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          NULL},
    {WINED3DFMT_S1_UINT_D15_UNORM,      GL_DEPTH_COMPONENT16,             GL_DEPTH_COMPONENT16,                   0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          NULL},
    {WINED3DFMT_S1_UINT_D15_UNORM,      GL_DEPTH24_STENCIL8_EXT,          GL_DEPTH24_STENCIL8_EXT,                0,
            GL_DEPTH_STENCIL_EXT,       GL_UNSIGNED_INT_24_8_EXT,         4,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            EXT_PACKED_DEPTH_STENCIL,   convert_s1_uint_d15_unorm},
    {WINED3DFMT_S1_UINT_D15_UNORM,      GL_DEPTH24_STENCIL8,              GL_DEPTH24_STENCIL8,                    0,
            GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,             4,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            ARB_FRAMEBUFFER_OBJECT,     convert_s1_uint_d15_unorm},
    {WINED3DFMT_D24_UNORM_S8_UINT,      GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_INT,                  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          NULL},
    {WINED3DFMT_D24_UNORM_S8_UINT,      GL_DEPTH24_STENCIL8_EXT,          GL_DEPTH24_STENCIL8_EXT,                0,
            GL_DEPTH_STENCIL_EXT,       GL_UNSIGNED_INT_24_8_EXT,         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            EXT_PACKED_DEPTH_STENCIL,   NULL},
    {WINED3DFMT_D24_UNORM_S8_UINT,      GL_DEPTH24_STENCIL8,              GL_DEPTH24_STENCIL8,                    0,
            GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,             0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            ARB_FRAMEBUFFER_OBJECT,     NULL},
    {WINED3DFMT_X8D24_UNORM,            GL_DEPTH_COMPONENT,               GL_DEPTH_COMPONENT,                     0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_INT,                  4,
            WINED3DFMT_FLAG_DEPTH,
            WINED3D_GL_EXT_NONE,        x8_d24_unorm_upload,              x8_d24_unorm_download},
    {WINED3DFMT_X8D24_UNORM,            GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_INT,                  4,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          x8_d24_unorm_upload,              x8_d24_unorm_download},
    {WINED3DFMT_S4X4_UINT_D24_UNORM,    GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_INT,                  0,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          NULL},
    {WINED3DFMT_S4X4_UINT_D24_UNORM,    GL_DEPTH24_STENCIL8_EXT,          GL_DEPTH24_STENCIL8_EXT,                0,
            GL_DEPTH_STENCIL_EXT,       GL_UNSIGNED_INT_24_8_EXT,         4,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            EXT_PACKED_DEPTH_STENCIL,   convert_s4x4_uint_d24_unorm},
    {WINED3DFMT_S4X4_UINT_D24_UNORM,    GL_DEPTH24_STENCIL8,              GL_DEPTH24_STENCIL8,                    0,
            GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,             4,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            ARB_FRAMEBUFFER_OBJECT,     convert_s4x4_uint_d24_unorm},
    {WINED3DFMT_D16_UNORM,              GL_DEPTH_COMPONENT,               GL_DEPTH_COMPONENT,                     0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_DEPTH,
            WINED3D_GL_EXT_NONE,        NULL},
    {WINED3DFMT_D16_UNORM,              GL_DEPTH_COMPONENT16,             GL_DEPTH_COMPONENT16,                   0,
            GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_TEXTURE,          NULL},
    {WINED3DFMT_D32_FLOAT,              GL_DEPTH_COMPONENT32F,            GL_DEPTH_COMPONENT32F,                  0,
            GL_DEPTH_COMPONENT,         GL_FLOAT,                         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_BUFFER_FLOAT,     NULL},
    {WINED3DFMT_D32_FLOAT_S8X24_UINT,   GL_DEPTH32F_STENCIL8,             GL_DEPTH32F_STENCIL8,                   0,
            GL_DEPTH_STENCIL,           GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_BUFFER_FLOAT,     NULL},
    {WINED3DFMT_S8_UINT_D24_FLOAT,      GL_DEPTH32F_STENCIL8,             GL_DEPTH32F_STENCIL8,                   0,
            GL_DEPTH_STENCIL,           GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL | WINED3DFMT_FLAG_SHADOW,
            ARB_DEPTH_BUFFER_FLOAT,     convert_s8_uint_d24_float},
    {WINED3DFMT_R32G32B32A32_UINT,      GL_RGBA32UI,                      GL_RGBA32UI,                            0,
            GL_RGBA_INTEGER,            GL_UNSIGNED_INT,                  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_INTEGER,        NULL},
    {WINED3DFMT_R32G32B32A32_SINT,      GL_RGBA32I,                       GL_RGBA32I,                             0,
            GL_RGBA_INTEGER,            GL_INT,                           0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET,
            EXT_TEXTURE_INTEGER,        NULL},
    /* Vendor-specific formats */
    {WINED3DFMT_ATI1N,                  GL_COMPRESSED_RED_RGTC1,          GL_COMPRESSED_RED_RGTC1,                0,
            GL_RED,                     GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_ATI2N,                  GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI, GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI, 0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ATI_TEXTURE_COMPRESSION_3DC, NULL},
    {WINED3DFMT_ATI2N,                  GL_COMPRESSED_RG_RGTC2,           GL_COMPRESSED_RG_RGTC2,                 0,
            GL_LUMINANCE_ALPHA,         GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            EXT_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_ATI2N,                  GL_COMPRESSED_RG_RGTC2,           GL_COMPRESSED_RG_RGTC2,                 0,
            GL_RG,                      GL_UNSIGNED_BYTE,                 0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            ARB_TEXTURE_COMPRESSION_RGTC, NULL},
    {WINED3DFMT_INTZ,                   GL_DEPTH24_STENCIL8_EXT,          GL_DEPTH24_STENCIL8_EXT,                0,
            GL_DEPTH_STENCIL_EXT,       GL_UNSIGNED_INT_24_8_EXT,         0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL,
            EXT_PACKED_DEPTH_STENCIL,   NULL},
    {WINED3DFMT_INTZ,                   GL_DEPTH24_STENCIL8,              GL_DEPTH24_STENCIL8,                    0,
            GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,             0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING
            | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL,
            ARB_FRAMEBUFFER_OBJECT,     NULL},
    {WINED3DFMT_NULL,                   0,                                0,                                      0,
            GL_RGBA,                    GL_UNSIGNED_INT_8_8_8_8_REV,      0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_FBO_ATTACHABLE,
            ARB_FRAMEBUFFER_OBJECT,     NULL},
    /* DirectX 10 HDR formats */
    {WINED3DFMT_R9G9B9E5_SHAREDEXP,     GL_RGB9_E5_EXT,                   GL_RGB9_E5_EXT,                            0,
            GL_RGB,                     GL_UNSIGNED_INT_5_9_9_9_REV_EXT,  0,
            WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING,
            EXT_TEXTURE_SHARED_EXPONENT, NULL},
};

struct wined3d_format_srgb_info
{
    enum wined3d_format_id srgb_format_id;
    enum wined3d_format_id base_format_id;
};

static const struct wined3d_format_srgb_info format_srgb_info[] =
{
    {WINED3DFMT_R8G8B8A8_UNORM_SRGB, WINED3DFMT_R8G8B8A8_UNORM},
    {WINED3DFMT_BC1_UNORM_SRGB,      WINED3DFMT_BC1_UNORM},
    {WINED3DFMT_BC2_UNORM_SRGB,      WINED3DFMT_BC2_UNORM},
    {WINED3DFMT_BC3_UNORM_SRGB,      WINED3DFMT_BC3_UNORM},
    {WINED3DFMT_B8G8R8A8_UNORM_SRGB, WINED3DFMT_B8G8R8A8_UNORM},
    {WINED3DFMT_B8G8R8X8_UNORM_SRGB, WINED3DFMT_B8G8R8X8_UNORM},
    {WINED3DFMT_BC7_UNORM_SRGB,      WINED3DFMT_BC7_UNORM},
};

static inline int get_format_idx(enum wined3d_format_id format_id)
{
    unsigned int i;

    if (format_id < WINED3D_FORMAT_FOURCC_BASE)
        return format_id;

    for (i = 0; i < ARRAY_SIZE(format_index_remap); ++i)
    {
        if (format_index_remap[i].id == format_id)
            return format_index_remap[i].idx;
    }

    return -1;
}

static struct wined3d_format_gl *wined3d_format_gl_mutable(struct wined3d_format *format)
{
    return CONTAINING_RECORD(format, struct wined3d_format_gl, f);
}

static struct wined3d_format_vk *wined3d_format_vk_mutable(struct wined3d_format *format)
{
    return CONTAINING_RECORD(format, struct wined3d_format_vk, f);
}

static struct wined3d_format *get_format_by_idx(const struct wined3d_adapter *adapter, int fmt_idx)
{
    return (struct wined3d_format *)((BYTE *)adapter->formats + fmt_idx * adapter->format_size);
}

static struct wined3d_format_gl *get_format_gl_by_idx(const struct wined3d_adapter *adapter, int fmt_idx)
{
    return wined3d_format_gl_mutable(get_format_by_idx(adapter, fmt_idx));
}

static struct wined3d_format *get_format_internal(const struct wined3d_adapter *adapter,
        enum wined3d_format_id format_id)
{
    int fmt_idx;

    if ((fmt_idx = get_format_idx(format_id)) == -1)
    {
        ERR("Format %s (%#x) not found.\n", debug_d3dformat(format_id), format_id);
        return NULL;
    }

    return get_format_by_idx(adapter, fmt_idx);
}

static struct wined3d_format_gl *get_format_gl_internal(const struct wined3d_adapter *adapter,
        enum wined3d_format_id format_id)
{
    struct wined3d_format *format;

    if ((format = get_format_internal(adapter, format_id)))
        return wined3d_format_gl_mutable(format);

    return NULL;
}

static void copy_format(const struct wined3d_adapter *adapter,
        struct wined3d_format *dst_format, const struct wined3d_format *src_format)
{
    enum wined3d_format_id id = dst_format->id;
    memcpy(dst_format, src_format, adapter->format_size);
    dst_format->id = id;
}

static void format_set_flag(struct wined3d_format *format, unsigned int flag)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format->flags); ++i)
        format->flags[i] |= flag;
}

static void format_clear_flag(struct wined3d_format *format, unsigned int flag)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format->flags); ++i)
        format->flags[i] &= ~flag;
}

static enum wined3d_channel_type map_channel_type(char t)
{
    switch (t)
    {
        case 'u':
            return WINED3D_CHANNEL_TYPE_UNORM;
        case 'i':
            return WINED3D_CHANNEL_TYPE_SNORM;
        case 'U':
            return WINED3D_CHANNEL_TYPE_UINT;
        case 'I':
            return WINED3D_CHANNEL_TYPE_SINT;
        case 'F':
            return WINED3D_CHANNEL_TYPE_FLOAT;
        case 'D':
            return WINED3D_CHANNEL_TYPE_DEPTH;
        case 'S':
            return WINED3D_CHANNEL_TYPE_STENCIL;
        case 'X':
            return WINED3D_CHANNEL_TYPE_UNUSED;
        default:
            ERR("Invalid channel type '%c'.\n", t);
            return WINED3D_CHANNEL_TYPE_NONE;
    }
}

static BOOL init_format_base_info(struct wined3d_adapter *adapter)
{
    struct wined3d_format *format;
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        if (!(format = get_format_internal(adapter, formats[i].id)))
            return FALSE;

        format->id = formats[i].id;
        format->red_size = formats[i].red_size;
        format->green_size = formats[i].green_size;
        format->blue_size = formats[i].blue_size;
        format->alpha_size = formats[i].alpha_size;
        format->red_offset = formats[i].red_offset;
        format->green_offset = formats[i].green_offset;
        format->blue_offset = formats[i].blue_offset;
        format->alpha_offset = formats[i].alpha_offset;
        format->byte_count = formats[i].bpp;
        format->depth_size = formats[i].depth_size;
        format->stencil_size = formats[i].stencil_size;
        format->block_width = 1;
        format->block_height = 1;
        format->block_byte_count = formats[i].bpp;
    }

    for (i = 0; i < ARRAY_SIZE(typed_formats); ++i)
    {
        struct wined3d_format *typeless_format;
        unsigned int component_count = 0;
        DWORD flags = 0;

        if (!(format = get_format_internal(adapter, typed_formats[i].id)))
            return FALSE;

        if (!(typeless_format = get_format_internal(adapter, typed_formats[i].typeless_id)))
            return FALSE;

        format->id = typed_formats[i].id;
        format->red_size = typeless_format->red_size;
        format->green_size = typeless_format->green_size;
        format->blue_size = typeless_format->blue_size;
        format->alpha_size = typeless_format->alpha_size;
        format->red_offset = typeless_format->red_offset;
        format->green_offset = typeless_format->green_offset;
        format->blue_offset = typeless_format->blue_offset;
        format->alpha_offset = typeless_format->alpha_offset;
        format->byte_count = typeless_format->byte_count;
        format->depth_size = typeless_format->depth_size;
        format->stencil_size = typeless_format->stencil_size;
        format->block_width = typeless_format->block_width;
        format->block_height = typeless_format->block_height;
        format->block_byte_count = typeless_format->block_byte_count;
        format->typeless_id = typeless_format->id;

        typeless_format->typeless_id = typeless_format->id;

        for (j = 0; j < strlen(typed_formats[i].channels); ++j)
        {
            enum wined3d_channel_type channel_type = map_channel_type(typed_formats[i].channels[j]);

            if (channel_type == WINED3D_CHANNEL_TYPE_UNORM || channel_type == WINED3D_CHANNEL_TYPE_SNORM)
                flags |= WINED3DFMT_FLAG_NORMALISED;
            if (channel_type == WINED3D_CHANNEL_TYPE_UINT || channel_type == WINED3D_CHANNEL_TYPE_SINT)
                flags |= WINED3DFMT_FLAG_INTEGER;
            if (channel_type == WINED3D_CHANNEL_TYPE_FLOAT)
                flags |= WINED3DFMT_FLAG_FLOAT;
            if (channel_type != WINED3D_CHANNEL_TYPE_UNUSED)
                ++component_count;

            if (channel_type == WINED3D_CHANNEL_TYPE_DEPTH && !format->depth_size)
            {
                format->depth_size = format->red_size;
                format->red_size = format->red_offset = 0;
            }
        }

        format->component_count = component_count;
        format_set_flag(format, flags);
    }

    for (i = 0; i < ARRAY_SIZE(ddi_formats); ++i)
    {
        if (!(format = get_format_internal(adapter, ddi_formats[i].id)))
            return FALSE;

        format->ddi_format = ddi_formats[i].ddi_format;
    }

    for (i = 0; i < ARRAY_SIZE(format_base_flags); ++i)
    {
        if (!(format = get_format_internal(adapter, format_base_flags[i].id)))
            return FALSE;

        format_set_flag(format, format_base_flags[i].flags);
    }

    return TRUE;
}

static BOOL init_format_block_info(struct wined3d_adapter *adapter)
{
    struct wined3d_format *format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format_block_info); ++i)
    {
        if (!(format = get_format_internal(adapter, format_block_info[i].id)))
            return FALSE;

        format->block_width = format_block_info[i].block_width;
        format->block_height = format_block_info[i].block_height;
        format->block_byte_count = format_block_info[i].block_byte_count;
        format_set_flag(format, WINED3DFMT_FLAG_BLOCKS | format_block_info[i].flags);
    }

    return TRUE;
}

/* Most compressed formats are not supported for 3D textures by OpenGL.
 *
 * In the case of the S3TC/DXTn formats, NV_texture_compression_vtc provides
 * these formats for 3D textures, but unfortunately the block layout is
 * different from the one used by Direct3D.
 *
 * Since applications either don't check format availability at all before
 * using these, or refuse to run without them, we decompress them on upload.
 *
 * Affected applications include "Heroes VI", "From Dust", "Halo Online" and
 * "Eldorado". */
static BOOL init_format_decompress_info(struct wined3d_adapter *adapter)
{
    struct wined3d_format *format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format_decompress_info); ++i)
    {
        if (!(format = get_format_internal(adapter, format_decompress_info[i].id)))
            return FALSE;

        format->flags[WINED3D_GL_RES_TYPE_TEX_3D] |= WINED3DFMT_FLAG_DECOMPRESS;
        format->decompress = format_decompress_info[i].decompress;
    }

    return TRUE;
}

static BOOL init_srgb_formats(struct wined3d_adapter *adapter)
{
    struct wined3d_format *format, *srgb_format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format_srgb_info); ++i)
    {
        if (!(srgb_format = get_format_internal(adapter, format_srgb_info[i].srgb_format_id)))
            return FALSE;
        if (!(format = get_format_internal(adapter, format_srgb_info[i].base_format_id)))
            return FALSE;

        copy_format(adapter, srgb_format, format);
    }

    return TRUE;
}

static GLenum wined3d_gl_type_to_enum(enum wined3d_gl_resource_type type)
{
    switch (type)
    {
        case WINED3D_GL_RES_TYPE_TEX_1D:
            return GL_TEXTURE_1D;
        case WINED3D_GL_RES_TYPE_TEX_2D:
            return GL_TEXTURE_2D;
        case WINED3D_GL_RES_TYPE_TEX_3D:
            return GL_TEXTURE_3D;
        case WINED3D_GL_RES_TYPE_TEX_CUBE:
            return GL_TEXTURE_CUBE_MAP_ARB;
        case WINED3D_GL_RES_TYPE_TEX_RECT:
            return GL_TEXTURE_RECTANGLE_ARB;
        case WINED3D_GL_RES_TYPE_BUFFER:
            return GL_TEXTURE_2D; /* TODO: GL_TEXTURE_BUFFER. */
        case WINED3D_GL_RES_TYPE_RB:
            return GL_RENDERBUFFER;
        case WINED3D_GL_RES_TYPE_COUNT:
            break;
    }
    ERR("Unexpected GL resource type %u.\n", type);
    return 0;
}

static void delete_fbo_attachment(const struct wined3d_gl_info *gl_info,
        enum wined3d_gl_resource_type d3d_type, GLuint object)
{
    switch (d3d_type)
    {
        case WINED3D_GL_RES_TYPE_TEX_1D:
        case WINED3D_GL_RES_TYPE_TEX_2D:
        case WINED3D_GL_RES_TYPE_TEX_RECT:
        case WINED3D_GL_RES_TYPE_TEX_3D:
        case WINED3D_GL_RES_TYPE_TEX_CUBE:
            gl_info->gl_ops.gl.p_glDeleteTextures(1, &object);
            break;

        case WINED3D_GL_RES_TYPE_RB:
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &object);
            break;

        case WINED3D_GL_RES_TYPE_BUFFER:
        case WINED3D_GL_RES_TYPE_COUNT:
            break;
    }
}

/* Context activation is done by the caller. */
static void create_and_bind_fbo_attachment(const struct wined3d_gl_info *gl_info, unsigned int flags,
        enum wined3d_gl_resource_type d3d_type, GLuint *object, GLenum internal, GLenum format, GLenum type)
{
    GLenum attach_type = flags & WINED3DFMT_FLAG_DEPTH ?
            GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;

    switch (d3d_type)
    {
        case WINED3D_GL_RES_TYPE_TEX_1D:
            gl_info->gl_ops.gl.p_glGenTextures(1, object);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, *object);
            gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, internal, 16, 0, format, type, NULL);
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            gl_info->fbo_ops.glFramebufferTexture1D(GL_FRAMEBUFFER, attach_type, GL_TEXTURE_1D,
                    *object, 0);
            if (flags & WINED3DFMT_FLAG_STENCIL)
                gl_info->fbo_ops.glFramebufferTexture1D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_1D,
                        *object, 0);
            break;

        case WINED3D_GL_RES_TYPE_TEX_2D:
        case WINED3D_GL_RES_TYPE_TEX_RECT:
            gl_info->gl_ops.gl.p_glGenTextures(1, object);
            gl_info->gl_ops.gl.p_glBindTexture(wined3d_gl_type_to_enum(d3d_type), *object);
            gl_info->gl_ops.gl.p_glTexImage2D(wined3d_gl_type_to_enum(d3d_type), 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexParameteri(wined3d_gl_type_to_enum(d3d_type), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl_info->gl_ops.gl.p_glTexParameteri(wined3d_gl_type_to_enum(d3d_type), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, attach_type,
                    wined3d_gl_type_to_enum(d3d_type), *object, 0);
            if (flags & WINED3DFMT_FLAG_STENCIL)
                gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                        wined3d_gl_type_to_enum(d3d_type), *object, 0);
            break;

        case WINED3D_GL_RES_TYPE_TEX_3D:
            gl_info->gl_ops.gl.p_glGenTextures(1, object);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, *object);
            GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, 0, internal, 16, 16, 16, 0, format, type, NULL));
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            gl_info->fbo_ops.glFramebufferTexture3D(GL_FRAMEBUFFER, attach_type,
                    GL_TEXTURE_3D, *object, 0, 0);
            if (flags & WINED3DFMT_FLAG_STENCIL)
                gl_info->fbo_ops.glFramebufferTexture3D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                        GL_TEXTURE_3D, *object, 0, 0);
            break;

        case WINED3D_GL_RES_TYPE_TEX_CUBE:
            gl_info->gl_ops.gl.p_glGenTextures(1, object);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, *object);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 0, internal, 16, 16, 0,
                    format, type, NULL);
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, attach_type,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, *object, 0);
            if (flags & WINED3DFMT_FLAG_STENCIL)
                gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, *object, 0);
            break;

        case WINED3D_GL_RES_TYPE_RB:
            gl_info->fbo_ops.glGenRenderbuffers(1, object);
            gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, *object);
            gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, internal, 16, 16);
            gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach_type, GL_RENDERBUFFER,
                    *object);
            if (flags & WINED3DFMT_FLAG_STENCIL)
                gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                        *object);
            break;

        case WINED3D_GL_RES_TYPE_BUFFER:
        case WINED3D_GL_RES_TYPE_COUNT:
            break;
    }

    /* Ideally we'd skip all formats already known not to work on textures
     * by checking for WINED3DFMT_FLAG_TEXTURE here. However, we want to
     * know if we can attach WINED3DFMT_P8_UINT textures to FBOs, and this
     * format never has WINED3DFMT_FLAG_TEXTURE set. Instead, swallow GL
     * errors generated by invalid formats. */
    while (gl_info->gl_ops.gl.p_glGetError());
}

static void draw_test_quad(struct wined3d_caps_gl_ctx *ctx, const struct wined3d_vec3 *geometry,
        const struct wined3d_color *color)
{
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    static const struct wined3d_vec3 default_geometry[] =
    {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };
    static const char vs_core_header[] =
        "#version 150\n"
        "in vec4 pos;\n"
        "in vec4 color;\n"
        "out vec4 out_color;\n"
        "\n";
    static const char vs_legacy_header[] =
        "#version 120\n"
        "attribute vec4 pos;\n"
        "attribute vec4 color;\n"
        "varying vec4 out_color;\n"
        "\n";
    static const char vs_body[] =
        "void main()\n"
        "{\n"
        "    gl_Position = pos;\n"
        "    out_color = color;\n"
        "}\n";
    static const char fs_core[] =
        "#version 150\n"
        "in vec4 out_color;\n"
        "out vec4 fragment_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fragment_color = out_color;\n"
        "}\n";
    static const char fs_legacy[] =
        "#version 120\n"
        "varying vec4 out_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_FragData[0] = out_color;\n"
        "}\n";
    const char *source[2];
    GLuint vs_id, fs_id;
    unsigned int i;

    if (!geometry)
        geometry = default_geometry;

    if (!gl_info->supported[ARB_VERTEX_BUFFER_OBJECT] || !gl_info->supported[ARB_VERTEX_SHADER]
            || !gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_LIGHTING);
        gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
        gl_info->gl_ops.gl.p_glLoadIdentity();
        gl_info->gl_ops.gl.p_glMatrixMode(GL_PROJECTION);
        gl_info->gl_ops.gl.p_glLoadIdentity();

        gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
        gl_info->gl_ops.gl.p_glColor4f(color->r, color->g, color->b, color->a);
        for (i = 0; i < 4; ++i)
            gl_info->gl_ops.gl.p_glVertex3fv(&geometry[i].x);
        gl_info->gl_ops.gl.p_glEnd();
        checkGLcall("draw quad");
        return;
    }

    if (!ctx->test_vbo)
        GL_EXTCALL(glGenBuffers(1, &ctx->test_vbo));
    GL_EXTCALL(glBindBuffer(GL_ARRAY_BUFFER, ctx->test_vbo));
    GL_EXTCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(struct wined3d_vec3) * 4, geometry, GL_STREAM_DRAW));
    GL_EXTCALL(glVertexAttribPointer(0, 3, GL_FLOAT, FALSE, 0, NULL));
    GL_EXTCALL(glVertexAttrib4f(1, color->r, color->g, color->b, color->a));
    GL_EXTCALL(glEnableVertexAttribArray(0));
    GL_EXTCALL(glDisableVertexAttribArray(1));

    if (!ctx->test_program_id)
    {
#ifdef __REACTOS__
        /* workaround CORE-15408 crash for many 3D applications.
           VBoxDisp with enabled 3D acceleration only supports OpenGL 2.1 (GLSL 120).
           Wine must not use shaders for OpenGL 3.2 (GLSL 150). */
        BOOL use_glsl_150 = FALSE;
#else
        BOOL use_glsl_150 = gl_info->glsl_version >= MAKEDWORD_VERSION(1, 50);
#endif

        ctx->test_program_id = GL_EXTCALL(glCreateProgram());

        vs_id = GL_EXTCALL(glCreateShader(GL_VERTEX_SHADER));
        source[0] = use_glsl_150 ? vs_core_header : vs_legacy_header;
        source[1] = vs_body;
        GL_EXTCALL(glShaderSource(vs_id, 2, source, NULL));
        GL_EXTCALL(glAttachShader(ctx->test_program_id, vs_id));
        GL_EXTCALL(glDeleteShader(vs_id));

        fs_id = GL_EXTCALL(glCreateShader(GL_FRAGMENT_SHADER));
        source[0] = use_glsl_150 ? fs_core : fs_legacy;
        GL_EXTCALL(glShaderSource(fs_id, 1, source, NULL));
        GL_EXTCALL(glAttachShader(ctx->test_program_id, fs_id));
        GL_EXTCALL(glDeleteShader(fs_id));

        GL_EXTCALL(glBindAttribLocation(ctx->test_program_id, 0, "pos"));
        GL_EXTCALL(glBindAttribLocation(ctx->test_program_id, 1, "color"));

        if (use_glsl_150)
            GL_EXTCALL(glBindFragDataLocation(ctx->test_program_id, 0, "fragment_color"));

        GL_EXTCALL(glCompileShader(vs_id));
        print_glsl_info_log(gl_info, vs_id, FALSE);
        GL_EXTCALL(glCompileShader(fs_id));
        print_glsl_info_log(gl_info, fs_id, FALSE);
        GL_EXTCALL(glLinkProgram(ctx->test_program_id));
        shader_glsl_validate_link(gl_info, ctx->test_program_id);
    }
    GL_EXTCALL(glUseProgram(ctx->test_program_id));

    gl_info->gl_ops.gl.p_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL_EXTCALL(glUseProgram(0));
    GL_EXTCALL(glDisableVertexAttribArray(0));
    GL_EXTCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    checkGLcall("draw quad");
}

/* Context activation is done by the caller. */
static void check_fbo_compat(struct wined3d_caps_gl_ctx *ctx, struct wined3d_format_gl *format)
{
    /* Check if the default internal format is supported as a frame buffer
     * target, otherwise fall back to the render target internal.
     *
     * Try to stick to the standard format if possible, this limits precision differences. */
    static const struct wined3d_color black = {0.0f, 0.0f, 0.0f, 1.0f};
    static const struct wined3d_color half_transparent_red = {1.0f, 0.0f, 0.0f, 0.5f};
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    GLenum status, rt_internal = format->rt_internal;
    GLuint object, color_rb;
    enum wined3d_gl_resource_type type;
    BOOL fallback_fmt_used = FALSE, regular_fmt_used = FALSE;

    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);

    for (type = 0; type < ARRAY_SIZE(format->f.flags); ++type)
    {
        const char *type_string = "color";

        if (type == WINED3D_GL_RES_TYPE_BUFFER)
            continue;

        create_and_bind_fbo_attachment(gl_info, format->f.flags[type], type,
                &object, format->internal, format->format, format->type);

        if (format->f.flags[type] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
        {
            gl_info->fbo_ops.glGenRenderbuffers(1, &color_rb);
            gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, color_rb);
            if (type == WINED3D_GL_RES_TYPE_TEX_1D)
                gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 16, 1);
            else
                gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 16, 16);

            gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_rb);
            checkGLcall("Create and attach color rb attachment");
            type_string = "depth / stencil";
        }

        status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
        checkGLcall("Framebuffer format check");

        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            TRACE("Format %s is supported as FBO %s attachment, type %u.\n",
                    debug_d3dformat(format->f.id), type_string, type);
            format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE;
            format->rt_internal = format->internal;
            regular_fmt_used = TRUE;
        }
        else
        {
            if (!rt_internal)
            {
                if (format->f.flags[type] & WINED3DFMT_FLAG_RENDERTARGET)
                {
                    WARN("Format %s with rendertarget flag is not supported as FBO color attachment (type %u),"
                            " and no fallback specified.\n", debug_d3dformat(format->f.id), type);
                    format->f.flags[type] &= ~WINED3DFMT_FLAG_RENDERTARGET;
                }
                else
                {
                    TRACE("Format %s is not supported as FBO %s attachment, type %u.\n",
                            debug_d3dformat(format->f.id), type_string, type);
                }
                format->rt_internal = format->internal;
            }
            else
            {
                TRACE("Format %s is not supported as FBO %s attachment (type %u),"
                        " trying rtInternal format as fallback.\n",
                        debug_d3dformat(format->f.id), type_string, type);

                while (gl_info->gl_ops.gl.p_glGetError());

                delete_fbo_attachment(gl_info, type, object);
                create_and_bind_fbo_attachment(gl_info, format->f.flags[type], type,
                        &object, format->rt_internal, format->format, format->type);

                status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
                checkGLcall("Framebuffer format check");

                if (status == GL_FRAMEBUFFER_COMPLETE)
                {
                    TRACE("Format %s rtInternal format is supported as FBO %s attachment, type %u.\n",
                            debug_d3dformat(format->f.id), type_string, type);
                    fallback_fmt_used = TRUE;
                }
                else
                {
                    WARN("Format %s rtInternal format is not supported as FBO %s attachment, type %u.\n",
                            debug_d3dformat(format->f.id), type_string, type);
                    format->f.flags[type] &= ~WINED3DFMT_FLAG_RENDERTARGET;
                }
            }
        }

        if (status == GL_FRAMEBUFFER_COMPLETE
                && ((format->f.flags[type] & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING)
                || !(gl_info->quirks & WINED3D_QUIRK_LIMITED_TEX_FILTERING))
                && !(format->f.flags[type] & WINED3DFMT_FLAG_INTEGER)
                && format->f.id != WINED3DFMT_NULL && format->f.id != WINED3DFMT_P8_UINT
                && format->format != GL_LUMINANCE && format->format != GL_LUMINANCE_ALPHA
                && (format->f.red_size || format->f.alpha_size))
        {
            DWORD readback[16 * 16 * 16], color, r_range, a_range;
            BYTE r, a;
            BOOL match = TRUE;
            GLuint rb;

            if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT]
                    || gl_info->supported[EXT_PACKED_DEPTH_STENCIL])
            {
                gl_info->fbo_ops.glGenRenderbuffers(1, &rb);
                gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, rb);
                if (type == WINED3D_GL_RES_TYPE_TEX_1D)
                    gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 16, 1);
                else
                    gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 16, 16);
                gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
                gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb);
                checkGLcall("RB attachment");
            }

            gl_info->gl_ops.gl.p_glEnable(GL_BLEND);
            gl_info->gl_ops.gl.p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT);
            if (gl_info->gl_ops.gl.p_glGetError() == GL_INVALID_FRAMEBUFFER_OPERATION)
            {
                while (gl_info->gl_ops.gl.p_glGetError());
                TRACE("Format %s doesn't support post-pixelshader blending, type %u.\n",
                        debug_d3dformat(format->f.id), type);
                format->f.flags[type] &= ~WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
            }
            else
            {
                gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
                if (type == WINED3D_GL_RES_TYPE_TEX_1D)
                    gl_info->gl_ops.gl.p_glViewport(0, 0, 16, 1);
                else
                    gl_info->gl_ops.gl.p_glViewport(0, 0, 16, 16);
                gl_info->gl_ops.gl.p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                draw_test_quad(ctx, NULL, &black);

                gl_info->gl_ops.gl.p_glEnable(GL_BLEND);

                draw_test_quad(ctx, NULL, &half_transparent_red);

                gl_info->gl_ops.gl.p_glDisable(GL_BLEND);

                switch (type)
                {
                    case WINED3D_GL_RES_TYPE_TEX_1D:
                        /* Rebinding texture to workaround a fglrx bug. */
                        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, object);
                        gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_1D, 0, GL_BGRA,
                                GL_UNSIGNED_INT_8_8_8_8_REV, readback);
                        color = readback[7];
                        break;

                    case WINED3D_GL_RES_TYPE_TEX_2D:
                    case WINED3D_GL_RES_TYPE_TEX_3D:
                    case WINED3D_GL_RES_TYPE_TEX_RECT:
                        /* Rebinding texture to workaround a fglrx bug. */
                        gl_info->gl_ops.gl.p_glBindTexture(wined3d_gl_type_to_enum(type), object);
                        gl_info->gl_ops.gl.p_glGetTexImage(wined3d_gl_type_to_enum(type), 0, GL_BGRA,
                                GL_UNSIGNED_INT_8_8_8_8_REV, readback);
                        color = readback[7 * 16 + 7];
                        break;

                    case WINED3D_GL_RES_TYPE_TEX_CUBE:
                        /* Rebinding texture to workaround a fglrx bug. */
                        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, object);
                        gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_BGRA,
                                GL_UNSIGNED_INT_8_8_8_8_REV, readback);
                        color = readback[7 * 16 + 7];
                        break;

                    case WINED3D_GL_RES_TYPE_RB:
                        gl_info->gl_ops.gl.p_glReadPixels(0, 0, 16, 16,
                                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, readback);
                        color = readback[7 * 16 + 7];
                        break;

                    case WINED3D_GL_RES_TYPE_BUFFER:
                    case WINED3D_GL_RES_TYPE_COUNT:
                        color = 0;
                        break;
                }
                checkGLcall("Post-pixelshader blending check");

                a = color >> 24;
                r = (color & 0x00ff0000u) >> 16;

                r_range = format->f.red_size < 8 ? 1u << (8 - format->f.red_size) : 1;
                a_range = format->f.alpha_size < 8 ? 1u << (8 - format->f.alpha_size) : 1;
                if (format->f.red_size && (r < 0x7f - r_range || r > 0x7f + r_range))
                    match = FALSE;
                else if (format->f.alpha_size > 1 && (a < 0xbf - a_range || a > 0xbf + a_range))
                    match = FALSE;
                if (!match)
                {
                    TRACE("Format %s doesn't support post-pixelshader blending, type %u.\n",
                            debug_d3dformat(format->f.id), type);
                    TRACE("Color output: %#x\n", color);
                    format->f.flags[type] &= ~WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
                }
                else
                {
                    TRACE("Format %s supports post-pixelshader blending, type %u.\n",
                            debug_d3dformat(format->f.id), type);
                    TRACE("Color output: %#x\n", color);
                    format->f.flags[type] |= WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
                }
            }

            if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT]
                    || gl_info->supported[EXT_PACKED_DEPTH_STENCIL])
            {
                gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
                gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
                gl_info->fbo_ops.glDeleteRenderbuffers(1, &rb);
                checkGLcall("RB cleanup");
            }
        }

        if (format->internal != format->srgb_internal)
        {
            delete_fbo_attachment(gl_info, type, object);
            create_and_bind_fbo_attachment(gl_info, format->f.flags[type], type, &object, format->srgb_internal,
                    format->format, format->type);

            status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
            checkGLcall("Framebuffer format check");

            if (status == GL_FRAMEBUFFER_COMPLETE)
            {
                TRACE("Format %s's sRGB format is FBO attachable, type %u.\n",
                        debug_d3dformat(format->f.id), type);
                format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB;
                if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
                    format->internal = format->srgb_internal;
            }
            else
            {
                WARN("Format %s's sRGB format is not FBO attachable, type %u.\n",
                        debug_d3dformat(format->f.id), type);
                format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_WRITE);
            }
        }
        else if (status == GL_FRAMEBUFFER_COMPLETE)
            format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB;

        if (format->f.flags[type] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
        {
            gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &color_rb);
        }

        delete_fbo_attachment(gl_info, type, object);
        checkGLcall("Framebuffer format check cleanup");
    }

    if (fallback_fmt_used && regular_fmt_used)
    {
        FIXME("Format %s needs different render target formats for different resource types.\n",
                debug_d3dformat(format->f.id));
        format_clear_flag(&format->f, WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_FBO_ATTACHABLE
                | WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING);
    }
}

static void query_format_flag(struct wined3d_gl_info *gl_info, struct wined3d_format_gl *format,
        GLint internal, GLenum pname, DWORD flag, const char *string)
{
    GLint value;
    enum wined3d_gl_resource_type type;

    for (type = 0; type < ARRAY_SIZE(format->f.flags); ++type)
    {
        gl_info->gl_ops.ext.p_glGetInternalformativ(wined3d_gl_type_to_enum(type), internal, pname, 1, &value);
        if (value == GL_FULL_SUPPORT)
        {
            TRACE("Format %s supports %s, resource type %u.\n", debug_d3dformat(format->f.id), string, type);
            format->f.flags[type] |= flag;
        }
        else
        {
            TRACE("Format %s doesn't support %s, resource type %u.\n", debug_d3dformat(format->f.id), string, type);
            format->f.flags[type] &= ~flag;
        }
    }
}

/* Context activation is done by the caller. */
static void init_format_fbo_compat_info(const struct wined3d_adapter *adapter,
        struct wined3d_caps_gl_ctx *ctx)
{
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    unsigned int i, type;
    GLuint fbo;

    if (gl_info->supported[ARB_INTERNALFORMAT_QUERY2])
    {
        for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
        {
            struct wined3d_format_gl *format = get_format_gl_by_idx(adapter, i);
            BOOL fallback_fmt_used = FALSE, regular_fmt_used = FALSE;
            GLenum rt_internal = format->rt_internal;
            GLint value;

            if (!format->internal)
                continue;

            for (type = 0; type < ARRAY_SIZE(format->f.flags); ++type)
            {
                gl_info->gl_ops.ext.p_glGetInternalformativ(wined3d_gl_type_to_enum(type),
                        format->internal, GL_FRAMEBUFFER_RENDERABLE, 1, &value);
                if (value == GL_FULL_SUPPORT)
                {
                    TRACE("Format %s is supported as FBO color attachment, resource type %u.\n",
                            debug_d3dformat(format->f.id), type);
                    format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE;
                    format->rt_internal = format->internal;
                    regular_fmt_used = TRUE;

                    gl_info->gl_ops.ext.p_glGetInternalformativ(wined3d_gl_type_to_enum(type),
                            format->internal, GL_FRAMEBUFFER_BLEND, 1, &value);
                    if (value == GL_FULL_SUPPORT)
                    {
                        TRACE("Format %s supports post-pixelshader blending, resource type %u.\n",
                                    debug_d3dformat(format->f.id), type);
                        format->f.flags[type] |= WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
                    }
                    else
                    {
                        TRACE("Format %s doesn't support post-pixelshader blending, resource typed %u.\n",
                                debug_d3dformat(format->f.id), type);
                        format->f.flags[type] &= ~WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
                    }
                }
                else
                {
                    if (!rt_internal)
                    {
                        if (format->f.flags[type] & WINED3DFMT_FLAG_RENDERTARGET)
                        {
                            WARN("Format %s with rendertarget flag is not supported as FBO color attachment"
                                    " and no fallback specified, resource type %u.\n",
                                    debug_d3dformat(format->f.id), type);
                            format->f.flags[type] &= ~WINED3DFMT_FLAG_RENDERTARGET;
                        }
                        else
                            TRACE("Format %s is not supported as FBO color attachment,"
                            " resource type %u.\n", debug_d3dformat(format->f.id), type);
                        format->rt_internal = format->internal;
                    }
                    else
                    {
                        gl_info->gl_ops.ext.p_glGetInternalformativ(wined3d_gl_type_to_enum(type),
                                rt_internal, GL_FRAMEBUFFER_RENDERABLE, 1, &value);
                        if (value == GL_FULL_SUPPORT)
                        {
                            TRACE("Format %s rtInternal format is supported as FBO color attachment,"
                                    " resource type %u.\n", debug_d3dformat(format->f.id), type);
                            fallback_fmt_used = TRUE;
                        }
                        else
                        {
                            WARN("Format %s rtInternal format is not supported as FBO color attachment,"
                                    " resource type %u.\n", debug_d3dformat(format->f.id), type);
                            format->f.flags[type] &= ~WINED3DFMT_FLAG_RENDERTARGET;
                        }
                    }
                }

                if (format->internal != format->srgb_internal)
                {
                    gl_info->gl_ops.ext.p_glGetInternalformativ(wined3d_gl_type_to_enum(type),
                            format->srgb_internal, GL_FRAMEBUFFER_RENDERABLE, 1, &value);
                    if (value == GL_FULL_SUPPORT)
                    {
                        TRACE("Format %s's sRGB format is FBO attachable, resource type %u.\n",
                                debug_d3dformat(format->f.id), type);
                        format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB;
                        if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
                            format->internal = format->srgb_internal;
                    }
                    else
                    {
                        WARN("Format %s's sRGB format is not FBO attachable, resource type %u.\n",
                                debug_d3dformat(format->f.id), type);
                        format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_WRITE);
                    }
                }
                else if (format->f.flags[type] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                    format->f.flags[type] |= WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB;
            }

            if (fallback_fmt_used && regular_fmt_used)
            {
                FIXME("Format %s needs different render target formats for different resource types.\n",
                        debug_d3dformat(format->f.id));
                format_clear_flag(&format->f, WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_FBO_ATTACHABLE
                        | WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB | WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING);
            }
        }
        return;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
        gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl_info->gl_ops.gl.p_glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl_info->gl_ops.gl.p_glReadBuffer(GL_COLOR_ATTACHMENT0);
    }

    for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
    {
        struct wined3d_format_gl *format = get_format_gl_by_idx(adapter, i);

        if (!format->internal)
            continue;

        if (format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
        {
            TRACE("Skipping format %s because it's a compressed format.\n",
                    debug_d3dformat(format->f.id));
            continue;
        }

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            TRACE("Checking if format %s is supported as FBO color attachment...\n", debug_d3dformat(format->f.id));
            check_fbo_compat(ctx, format);
        }
        else
        {
            format->rt_internal = format->internal;
        }
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
}

static GLenum lookup_gl_view_class(GLenum internal_format)
{
    static const struct
    {
        GLenum internal_format;
        GLenum view_class;
    }
    view_classes[] =
    {
        /* 128-bit */
        {GL_RGBA32F,        GL_VIEW_CLASS_128_BITS},
        {GL_RGBA32UI,       GL_VIEW_CLASS_128_BITS},
        {GL_RGBA32I,        GL_VIEW_CLASS_128_BITS},
        /* 96-bit */
        {GL_RGB32F,         GL_VIEW_CLASS_96_BITS},
        {GL_RGB32UI,        GL_VIEW_CLASS_96_BITS},
        {GL_RGB32I,         GL_VIEW_CLASS_96_BITS},
        /* 64-bit */
        {GL_RGBA16F,        GL_VIEW_CLASS_64_BITS},
        {GL_RG32F,          GL_VIEW_CLASS_64_BITS},
        {GL_RGBA16UI,       GL_VIEW_CLASS_64_BITS},
        {GL_RG32UI,         GL_VIEW_CLASS_64_BITS},
        {GL_RGBA16I,        GL_VIEW_CLASS_64_BITS},
        {GL_RG32I,          GL_VIEW_CLASS_64_BITS},
        {GL_RGBA16,         GL_VIEW_CLASS_64_BITS},
        {GL_RGBA16_SNORM,   GL_VIEW_CLASS_64_BITS},
        /* 48-bit */
        {GL_RGB16,          GL_VIEW_CLASS_48_BITS},
        {GL_RGB16_SNORM,    GL_VIEW_CLASS_48_BITS},
        {GL_RGB16F,         GL_VIEW_CLASS_48_BITS},
        {GL_RGB16UI,        GL_VIEW_CLASS_48_BITS},
        {GL_RGB16I,         GL_VIEW_CLASS_48_BITS},
        /* 32-bit */
        {GL_RG16F,          GL_VIEW_CLASS_32_BITS},
        {GL_R11F_G11F_B10F, GL_VIEW_CLASS_32_BITS},
        {GL_R32F,           GL_VIEW_CLASS_32_BITS},
        {GL_RGB10_A2UI,     GL_VIEW_CLASS_32_BITS},
        {GL_RGBA8UI,        GL_VIEW_CLASS_32_BITS},
        {GL_RG16UI,         GL_VIEW_CLASS_32_BITS},
        {GL_R32UI,          GL_VIEW_CLASS_32_BITS},
        {GL_RGBA8I,         GL_VIEW_CLASS_32_BITS},
        {GL_RG16I,          GL_VIEW_CLASS_32_BITS},
        {GL_R32I,           GL_VIEW_CLASS_32_BITS},
        {GL_RGB10_A2,       GL_VIEW_CLASS_32_BITS},
        {GL_RGBA8,          GL_VIEW_CLASS_32_BITS},
        {GL_RG16,           GL_VIEW_CLASS_32_BITS},
        {GL_RGBA8_SNORM,    GL_VIEW_CLASS_32_BITS},
        {GL_RG16_SNORM,     GL_VIEW_CLASS_32_BITS},
        {GL_SRGB8_ALPHA8,   GL_VIEW_CLASS_32_BITS},
        {GL_RGB9_E5,        GL_VIEW_CLASS_32_BITS},
        /* 24-bit */
        {GL_RGB8,           GL_VIEW_CLASS_24_BITS},
        {GL_RGB8_SNORM,     GL_VIEW_CLASS_24_BITS},
        {GL_SRGB8,          GL_VIEW_CLASS_24_BITS},
        {GL_RGB8UI,         GL_VIEW_CLASS_24_BITS},
        {GL_RGB8I,          GL_VIEW_CLASS_24_BITS},
        /* 16-bit */
        {GL_R16F,           GL_VIEW_CLASS_16_BITS},
        {GL_RG8UI,          GL_VIEW_CLASS_16_BITS},
        {GL_R16UI,          GL_VIEW_CLASS_16_BITS},
        {GL_RG8I,           GL_VIEW_CLASS_16_BITS},
        {GL_R16I,           GL_VIEW_CLASS_16_BITS},
        {GL_RG8,            GL_VIEW_CLASS_16_BITS},
        {GL_R16,            GL_VIEW_CLASS_16_BITS},
        {GL_RG8_SNORM,      GL_VIEW_CLASS_16_BITS},
        {GL_R16_SNORM,      GL_VIEW_CLASS_16_BITS},
        /* 8-bit */
        {GL_R8UI,           GL_VIEW_CLASS_8_BITS},
        {GL_R8I,            GL_VIEW_CLASS_8_BITS},
        {GL_R8,             GL_VIEW_CLASS_8_BITS},
        {GL_R8_SNORM,       GL_VIEW_CLASS_8_BITS},

        /* RGTC1 */
        {GL_COMPRESSED_RED_RGTC1,        GL_VIEW_CLASS_RGTC1_RED},
        {GL_COMPRESSED_SIGNED_RED_RGTC1, GL_VIEW_CLASS_RGTC1_RED},
        /* RGTC2 */
        {GL_COMPRESSED_RG_RGTC2,         GL_VIEW_CLASS_RGTC2_RG},
        {GL_COMPRESSED_SIGNED_RG_RGTC2,  GL_VIEW_CLASS_RGTC2_RG},

        /* BPTC unorm */
        {GL_COMPRESSED_RGBA_BPTC_UNORM,         GL_VIEW_CLASS_BPTC_UNORM},
        {GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,   GL_VIEW_CLASS_BPTC_UNORM},
        /* BPTC float */
        {GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,   GL_VIEW_CLASS_BPTC_FLOAT},
        {GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_VIEW_CLASS_BPTC_FLOAT},

        /* DXT1 RGB */
        {GL_COMPRESSED_RGB_S3TC_DXT1_EXT,        GL_VIEW_CLASS_S3TC_DXT1_RGB},
        {GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,       GL_VIEW_CLASS_S3TC_DXT1_RGB},
        /* DXT1 RGBA */
        {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,       GL_VIEW_CLASS_S3TC_DXT1_RGBA},
        {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_VIEW_CLASS_S3TC_DXT1_RGBA},
        /* DXT3 */
        {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,       GL_VIEW_CLASS_S3TC_DXT3_RGBA},
        {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_VIEW_CLASS_S3TC_DXT3_RGBA},
        /* DXT5 */
        {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,       GL_VIEW_CLASS_S3TC_DXT5_RGBA},
        {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_VIEW_CLASS_S3TC_DXT5_RGBA},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(view_classes); ++i)
    {
        if (view_classes[i].internal_format == internal_format)
            return view_classes[i].view_class;
    }

    return GL_NONE;
}

static void query_view_class(struct wined3d_format_gl *format)
{
    GLenum internal_view_class, gamma_view_class, rt_view_class;

    internal_view_class = lookup_gl_view_class(format->internal);
    gamma_view_class = lookup_gl_view_class(format->srgb_internal);
    rt_view_class = lookup_gl_view_class(format->rt_internal);

    if (internal_view_class == gamma_view_class || gamma_view_class == rt_view_class)
    {
        format->view_class = internal_view_class;
        TRACE("Format %s is member of GL view class %#x.\n",
                debug_d3dformat(format->f.id), format->view_class);
    }
    else
    {
        format->view_class = GL_NONE;
    }
}

static void query_internal_format(struct wined3d_adapter *adapter,
        struct wined3d_format_gl *format, const struct wined3d_format_texture_info *texture_info,
        struct wined3d_gl_info *gl_info, BOOL srgb_write_supported, BOOL srgb_format)
{
    GLint count, multisample_types[8];
    unsigned int i, max_log2;
    GLenum target;

    if (gl_info->supported[ARB_INTERNALFORMAT_QUERY2])
    {
        query_format_flag(gl_info, format, format->internal, GL_VERTEX_TEXTURE,
                WINED3DFMT_FLAG_VTF, "vertex texture usage");
        query_format_flag(gl_info, format, format->internal, GL_FILTER,
                WINED3DFMT_FLAG_FILTERING, "filtering");

        if (srgb_format || format->srgb_internal != format->internal)
        {
            query_format_flag(gl_info, format, format->srgb_internal, GL_SRGB_READ,
                    WINED3DFMT_FLAG_SRGB_READ, "sRGB read");

            if (srgb_write_supported)
                query_format_flag(gl_info, format, format->srgb_internal, GL_SRGB_WRITE,
                        WINED3DFMT_FLAG_SRGB_WRITE, "sRGB write");
            else
                format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_WRITE);

            if (!(format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D]
                    & (WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE)))
                format->srgb_internal = format->internal;
            else if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
                format->internal = format->srgb_internal;
        }
    }
    else
    {
        if (!gl_info->limits.samplers[WINED3D_SHADER_TYPE_VERTEX])
            format_clear_flag(&format->f, WINED3DFMT_FLAG_VTF);

        if (!(gl_info->quirks & WINED3D_QUIRK_LIMITED_TEX_FILTERING))
            format_set_flag(&format->f, WINED3DFMT_FLAG_FILTERING);
        else if (format->f.id != WINED3DFMT_R32G32B32A32_FLOAT && format->f.id != WINED3DFMT_R32_FLOAT)
            format_clear_flag(&format->f, WINED3DFMT_FLAG_VTF);

        if (srgb_format || format->srgb_internal != format->internal)
        {
            /* Filter sRGB capabilities if EXT_texture_sRGB is not supported. */
            if (!gl_info->supported[EXT_TEXTURE_SRGB])
            {
                format->srgb_internal = format->internal;
                format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE);
            }
            else if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
            {
                format->internal = format->srgb_internal;
            }
        }

        if ((format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_SRGB_WRITE) && !srgb_write_supported)
            format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_WRITE);

        if (!gl_info->supported[ARB_DEPTH_TEXTURE]
                && texture_info->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
        {
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_1D] &= ~WINED3DFMT_FLAG_TEXTURE;
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] &= ~WINED3DFMT_FLAG_TEXTURE;
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_CUBE] &= ~WINED3DFMT_FLAG_TEXTURE;
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_RECT] &= ~WINED3DFMT_FLAG_TEXTURE;
        }
    }

    query_view_class(format);

    if (format->internal && format->f.flags[WINED3D_GL_RES_TYPE_RB]
            & (WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
    {
        if (gl_info->supported[ARB_INTERNALFORMAT_QUERY])
        {
            target = gl_info->supported[ARB_TEXTURE_MULTISAMPLE] ? GL_TEXTURE_2D_MULTISAMPLE : GL_RENDERBUFFER;
            count = 0;
            GL_EXTCALL(glGetInternalformativ(target, format->internal,
                    GL_NUM_SAMPLE_COUNTS, 1, &count));
            if (count > ARRAY_SIZE(multisample_types))
                FIXME("Unexpectedly high number of multisample types %d.\n", count);
            count = min(count, ARRAY_SIZE(multisample_types));
            GL_EXTCALL(glGetInternalformativ(target, format->internal,
                    GL_SAMPLES, count, multisample_types));
            checkGLcall("query sample counts");
            for (i = 0; i < count; ++i)
            {
                if (multisample_types[i] > sizeof(format->f.multisample_types) * CHAR_BIT)
                    continue;
                format->f.multisample_types |= 1u << (multisample_types[i] - 1);
            }
        }
        else
        {
            max_log2 = wined3d_log2i(min(gl_info->limits.samples,
                    sizeof(format->f.multisample_types) * CHAR_BIT));
            for (i = 1; i <= max_log2; ++i)
                format->f.multisample_types |= 1u << ((1u << i) - 1);
        }
    }
}

static BOOL init_format_texture_info(struct wined3d_adapter *adapter, struct wined3d_gl_info *gl_info)
{
    struct wined3d_format_gl *format, *srgb_format;
    struct fragment_caps fragment_caps;
    struct shader_caps shader_caps;
    unsigned int i, j;
    BOOL srgb_write;

    adapter->fragment_pipe->get_caps(adapter, &fragment_caps);
    adapter->shader_backend->shader_get_caps(adapter, &shader_caps);
    srgb_write = (fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_SRGB_WRITE)
            && (shader_caps.wined3d_caps & WINED3D_SHADER_CAP_SRGB_WRITE);

    for (i = 0; i < ARRAY_SIZE(format_texture_info); ++i)
    {
        if (!(format = get_format_gl_internal(adapter, format_texture_info[i].id)))
            return FALSE;

        if (!gl_info->supported[format_texture_info[i].extension])
            continue;

        /* ARB_texture_rg defines floating point formats, but only if
         * ARB_texture_float is also supported. */
        if (!gl_info->supported[ARB_TEXTURE_FLOAT]
                && (format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT))
            continue;

        /* ARB_texture_rg defines integer formats if EXT_texture_integer is also supported. */
        if (!gl_info->supported[EXT_TEXTURE_INTEGER]
                && (format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_INTEGER))
            continue;

        format->internal = format_texture_info[i].gl_internal;
        format->srgb_internal = format_texture_info[i].gl_srgb_internal;
        format->rt_internal = format_texture_info[i].gl_rt_internal;
        format->format = format_texture_info[i].gl_format;
        format->type = format_texture_info[i].gl_type;
        format->f.color_fixup = COLOR_FIXUP_IDENTITY;
        format->f.height_scale.numerator = 1;
        format->f.height_scale.denominator = 1;

        format->f.flags[WINED3D_GL_RES_TYPE_TEX_1D] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;
        format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;
        format->f.flags[WINED3D_GL_RES_TYPE_BUFFER] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;

        /* GL_ARB_depth_texture does not support 3D textures. It also says "cube textures are
         * problematic", but doesn't explicitly mandate that an error is generated. */
        if (gl_info->supported[EXT_TEXTURE3D]
                && !(format_texture_info[i].flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_CUBE] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;

        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
            format->f.flags[WINED3D_GL_RES_TYPE_TEX_RECT] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;

        format->f.flags[WINED3D_GL_RES_TYPE_RB] |= format_texture_info[i].flags | WINED3DFMT_FLAG_BLIT;
        format->f.flags[WINED3D_GL_RES_TYPE_RB] &= ~WINED3DFMT_FLAG_TEXTURE;

        if (format->srgb_internal != format->internal
                && !(adapter->d3d_info.wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL))
        {
            format->srgb_internal = format->internal;
            format_clear_flag(&format->f, WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE);
        }

        if (!gl_info->supported[ARB_SHADOW] && (format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_SHADOW))
            format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);

        query_internal_format(adapter, format, &format_texture_info[i], gl_info, srgb_write, FALSE);

        /* Texture conversion stuff */
        format->f.conv_byte_count = format_texture_info[i].conv_byte_count;
        format->f.upload = format_texture_info[i].upload;
        format->f.download = format_texture_info[i].download;

        srgb_format = NULL;
        for (j = 0; j < ARRAY_SIZE(format_srgb_info); ++j)
        {
            if (format_srgb_info[j].base_format_id == format->f.id)
            {
                if (!(srgb_format = get_format_gl_internal(adapter, format_srgb_info[j].srgb_format_id)))
                    return FALSE;
                break;
            }
        }
        if (!srgb_format)
            continue;

        copy_format(adapter, &srgb_format->f, &format->f);

        if (gl_info->supported[EXT_TEXTURE_SRGB]
                && !(adapter->d3d_info.wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL))
        {
            srgb_format->internal = format_texture_info[i].gl_srgb_internal;
            srgb_format->srgb_internal = format_texture_info[i].gl_srgb_internal;
            format_set_flag(&srgb_format->f, WINED3DFMT_FLAG_SRGB_READ | WINED3DFMT_FLAG_SRGB_WRITE);
            query_internal_format(adapter, srgb_format, &format_texture_info[i], gl_info, srgb_write, TRUE);
        }
    }

    return TRUE;
}

static BOOL color_match(DWORD c1, DWORD c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    return TRUE;
}

/* A context is provided by the caller */
static BOOL check_filter(const struct wined3d_gl_info *gl_info, GLenum internal)
{
    static const DWORD data[] = {0x00000000, 0xffffffff};
    GLuint tex, fbo, buffer;
    DWORD readback[16 * 1];
    BOOL ret = FALSE;

    /* Render a filtered texture and see what happens. This is intended to detect the lack of
     * float16 filtering on ATI X1000 class cards. The drivers disable filtering instead of
     * falling back to software. If this changes in the future this code will get fooled and
     * apps might hit the software path due to incorrectly advertised caps.
     *
     * Its unlikely that this changes however. GL Games like Mass Effect depend on the filter
     * disable fallback, if Apple or ATI ever change the driver behavior they will break more
     * than Wine. The Linux binary <= r500 driver is not maintained any more anyway
     */

    while (gl_info->gl_ops.gl.p_glGetError());

    gl_info->gl_ops.gl.p_glGenTextures(1, &buffer);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, buffer);
    memset(readback, 0x7e, sizeof(readback));
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 1, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, readback);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    gl_info->gl_ops.gl.p_glGenTextures(1, &tex);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, tex);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, internal, 2, 1, 0,
            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer, 0);
    gl_info->gl_ops.gl.p_glDrawBuffer(GL_COLOR_ATTACHMENT0);

    gl_info->gl_ops.gl.p_glViewport(0, 0, 16, 1);
    gl_info->gl_ops.gl.p_glDisable(GL_LIGHTING);
    gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
    gl_info->gl_ops.gl.p_glLoadIdentity();
    gl_info->gl_ops.gl.p_glMatrixMode(GL_PROJECTION);
    gl_info->gl_ops.gl.p_glLoadIdentity();

    gl_info->gl_ops.gl.p_glClearColor(0, 1, 0, 0);
    gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT);

    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glTexCoord2f(0.0, 0.0);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord2f(1.0, 0.0);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord2f(0.0, 1.0);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glTexCoord2f(1.0, 1.0);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glEnd();

    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, buffer);
    memset(readback, 0x7f, sizeof(readback));
    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, readback);
    if (color_match(readback[6], 0xffffffff, 5) || color_match(readback[6], 0x00000000, 5)
            || color_match(readback[9], 0xffffffff, 5) || color_match(readback[9], 0x00000000, 5))
    {
        TRACE("Read back colors 0x%08x and 0x%08x close to unfiltered color, assuming no filtering\n",
              readback[6], readback[9]);
        ret = FALSE;
    }
    else
    {
        TRACE("Read back colors are 0x%08x and 0x%08x, assuming texture is filtered\n",
              readback[6], readback[9]);
        ret = TRUE;
    }

    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &buffer);
    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);

    if (gl_info->gl_ops.gl.p_glGetError())
    {
        FIXME("Error during filtering test for format %x, returning no filtering\n", internal);
        ret = FALSE;
    }

    return ret;
}

static void init_format_filter_info(struct wined3d_adapter *adapter,
        struct wined3d_gl_info *gl_info)
{
    enum wined3d_pci_vendor vendor = adapter->driver_info.vendor;
    struct wined3d_format_gl *format;
    unsigned int i;
    static const enum wined3d_format_id fmts16[] =
    {
        WINED3DFMT_R16_FLOAT,
        WINED3DFMT_R16G16_FLOAT,
        WINED3DFMT_R16G16B16A16_FLOAT,
    };
    BOOL filtered;

    /* This was already handled by init_format_texture_info(). */
    if (gl_info->supported[ARB_INTERNALFORMAT_QUERY2])
        return;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
            || !gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        WARN("No FBO support, or no FBO ORM, guessing filter info from GL caps\n");
        if (vendor == HW_VENDOR_NVIDIA && gl_info->supported[ARB_TEXTURE_FLOAT])
        {
            TRACE("Nvidia card with texture_float support: Assuming float16 blending\n");
            filtered = TRUE;
        }
        else if (gl_info->limits.glsl_varyings > 44)
        {
            TRACE("More than 44 GLSL varyings - assuming d3d10 card with float16 blending\n");
            filtered = TRUE;
        }
        else
        {
            TRACE("Assuming no float16 blending\n");
            filtered = FALSE;
        }

        if (filtered)
        {
            for (i = 0; i < ARRAY_SIZE(fmts16); ++i)
            {
                format = get_format_gl_internal(adapter, fmts16[i]);
                format_set_flag(&format->f, WINED3DFMT_FLAG_FILTERING);
            }
        }
        return;
    }

    for (i = 0; i < ARRAY_SIZE(fmts16); ++i)
    {
        format = get_format_gl_internal(adapter, fmts16[i]);
        if (!format->internal)
            continue; /* Not supported by GL */

        filtered = check_filter(gl_info, format->internal);
        if (filtered)
        {
            TRACE("Format %s supports filtering.\n", debug_d3dformat(format->f.id));
            format_set_flag(&format->f, WINED3DFMT_FLAG_FILTERING);
        }
        else
        {
            TRACE("Format %s does not support filtering.\n", debug_d3dformat(format->f.id));
        }
    }
}

static enum fixup_channel_source fixup_source_from_char(char c)
{
    switch (c)
    {
        default:
        case '0':
            return CHANNEL_SOURCE_ZERO;
        case '1':
            return CHANNEL_SOURCE_ONE;
        case 'x':
        case 'X':
            return CHANNEL_SOURCE_X;
        case 'y':
        case 'Y':
            return CHANNEL_SOURCE_Y;
        case 'z':
        case 'Z':
            return CHANNEL_SOURCE_Z;
        case 'w':
        case 'W':
            return CHANNEL_SOURCE_W;
    }
}

static unsigned int fixup_sign_from_char(char c)
{
    if (c == 'x' || c == 'y' || c == 'z' || c == 'w')
        return 1;
    return 0;
}

static struct color_fixup_desc create_color_fixup_desc_from_string(const char *s)
{
    struct color_fixup_desc fixup;

    if (strlen(s) != 4)
    {
        ERR("Invalid fixup string %s.\n", wine_dbgstr_a(s));
        return COLOR_FIXUP_IDENTITY;
    }

    fixup.x_sign_fixup = fixup_sign_from_char(s[0]);
    fixup.x_source = fixup_source_from_char(s[0]);
    fixup.y_sign_fixup = fixup_sign_from_char(s[1]);
    fixup.y_source = fixup_source_from_char(s[1]);
    fixup.z_sign_fixup = fixup_sign_from_char(s[2]);
    fixup.z_source = fixup_source_from_char(s[2]);
    fixup.w_sign_fixup = fixup_sign_from_char(s[3]);
    fixup.w_source = fixup_source_from_char(s[3]);

    return fixup;
}

static void apply_format_fixups(struct wined3d_adapter *adapter, struct wined3d_gl_info *gl_info)
{
    const struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    struct wined3d_format_gl *format;
    BOOL use_legacy_fixups;
    unsigned int i;

    static const struct
    {
        enum wined3d_format_id id;
        const char *fixup;
        BOOL legacy;
        enum wined3d_gl_extension extension;
    }
    fixups[] =
    {
        {WINED3DFMT_R16_FLOAT,             "X11W", TRUE,  WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R32_FLOAT,             "X11W", TRUE,  WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R16G16_UNORM,          "XY1W", TRUE,  WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R16G16_FLOAT,          "XY1W", TRUE,  WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R32G32_FLOAT,          "XY1W", TRUE,  WINED3D_GL_EXT_NONE},

        {WINED3DFMT_R8G8_SNORM,            "xy11", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R8G8_SNORM,            "XY11", TRUE,  NV_TEXTURE_SHADER},
        {WINED3DFMT_R8G8_SNORM,            "XY11", TRUE,  EXT_TEXTURE_SNORM},

        {WINED3DFMT_R16G16_SNORM,          "xy11", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R16G16_SNORM,          "XY11", TRUE,  NV_TEXTURE_SHADER},
        {WINED3DFMT_R16G16_SNORM,          "XY11", TRUE,  EXT_TEXTURE_SNORM},

        {WINED3DFMT_R8G8B8A8_SNORM,        "xyzw", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R8G8B8A8_SNORM,        "XYZW", FALSE, NV_TEXTURE_SHADER},
        {WINED3DFMT_R8G8B8A8_SNORM,        "XYZW", FALSE, EXT_TEXTURE_SNORM},

        {WINED3DFMT_R5G5_SNORM_L6_UNORM,   "xzY1", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R5G5_SNORM_L6_UNORM,   "XYZW", FALSE, NV_TEXTURE_SHADER},
        {WINED3DFMT_R5G5_SNORM_L6_UNORM,   "XYZW", FALSE, EXT_TEXTURE_SNORM},

        {WINED3DFMT_R8G8_SNORM_L8X8_UNORM, "xyZW", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_R8G8_SNORM_L8X8_UNORM, "XYZW", FALSE, NV_TEXTURE_SHADER},

        {WINED3DFMT_ATI1N,                 "XXXX", FALSE, EXT_TEXTURE_COMPRESSION_RGTC},
        {WINED3DFMT_ATI1N,                 "XXXX", FALSE, ARB_TEXTURE_COMPRESSION_RGTC},

        {WINED3DFMT_ATI2N,                 "XW11", FALSE, ATI_TEXTURE_COMPRESSION_3DC},
        {WINED3DFMT_ATI2N,                 "YX11", FALSE, EXT_TEXTURE_COMPRESSION_RGTC},
        {WINED3DFMT_ATI2N,                 "YX11", FALSE, ARB_TEXTURE_COMPRESSION_RGTC},

        {WINED3DFMT_A8_UNORM,              "000X", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_A8_UNORM,              "XYZW", FALSE, WINED3D_GL_LEGACY_CONTEXT},

        {WINED3DFMT_L8A8_UNORM,            "XXXY", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_L8A8_UNORM,            "XYZW", FALSE, WINED3D_GL_LEGACY_CONTEXT},

        {WINED3DFMT_L4A4_UNORM,            "XXXY", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_L4A4_UNORM,            "XYZW", FALSE, WINED3D_GL_LEGACY_CONTEXT},

        {WINED3DFMT_L16_UNORM,             "XXX1", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_L16_UNORM,             "XYZW", FALSE, WINED3D_GL_LEGACY_CONTEXT},

        {WINED3DFMT_INTZ,                  "XXXX", FALSE, WINED3D_GL_EXT_NONE},
        {WINED3DFMT_INTZ,                  "XYZW", FALSE, WINED3D_GL_LEGACY_CONTEXT},

        {WINED3DFMT_L8_UNORM,              "XXX1", FALSE, ARB_TEXTURE_RG},
    };

    use_legacy_fixups = d3d_info->wined3d_creation_flags & WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR;
    for (i = 0; i < ARRAY_SIZE(fixups); ++i)
    {
        if (fixups[i].legacy && !use_legacy_fixups)
            continue;

        if (!gl_info->supported[fixups[i].extension])
            continue;

        format = get_format_gl_internal(adapter, fixups[i].id);
        format->f.color_fixup = create_color_fixup_desc_from_string(fixups[i].fixup);
    }

    if (!gl_info->supported[APPLE_YCBCR_422] && (gl_info->supported[ARB_FRAGMENT_PROGRAM]
            || (gl_info->supported[ARB_FRAGMENT_SHADER] && gl_info->supported[ARB_VERTEX_SHADER])))
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_YUY2);
        format->f.color_fixup = create_complex_fixup_desc(COMPLEX_FIXUP_YUY2);

        format = get_format_gl_internal(adapter, WINED3DFMT_UYVY);
        format->f.color_fixup = create_complex_fixup_desc(COMPLEX_FIXUP_UYVY);
    }
    else if (!gl_info->supported[APPLE_YCBCR_422] && (!gl_info->supported[ARB_FRAGMENT_PROGRAM]
            && (!gl_info->supported[ARB_FRAGMENT_SHADER] || !gl_info->supported[ARB_VERTEX_SHADER])))
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_YUY2);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_BLIT);
        format->internal = 0;

        format = get_format_gl_internal(adapter, WINED3DFMT_UYVY);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_BLIT);
        format->internal = 0;
    }

    if (gl_info->supported[ARB_FRAGMENT_PROGRAM]
            || (gl_info->supported[ARB_FRAGMENT_SHADER] && gl_info->supported[ARB_VERTEX_SHADER]))
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_YV12);
        format_set_flag(&format->f, WINED3DFMT_FLAG_HEIGHT_SCALE);
        format->f.height_scale.numerator = 3;
        format->f.height_scale.denominator = 2;
        format->f.color_fixup = create_complex_fixup_desc(COMPLEX_FIXUP_YV12);

        format = get_format_gl_internal(adapter, WINED3DFMT_NV12);
        format_set_flag(&format->f, WINED3DFMT_FLAG_HEIGHT_SCALE);
        format->f.height_scale.numerator = 3;
        format->f.height_scale.denominator = 2;
        format->f.color_fixup = create_complex_fixup_desc(COMPLEX_FIXUP_NV12);
    }
    else
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_YV12);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_BLIT);
        format->internal = 0;

        format = get_format_gl_internal(adapter, WINED3DFMT_NV12);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_BLIT);
        format->internal = 0;
    }

    if (gl_info->supported[ARB_FRAGMENT_PROGRAM] || gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_P8_UINT);
        format->f.color_fixup = create_complex_fixup_desc(COMPLEX_FIXUP_P8);
    }

    if (!gl_info->supported[ARB_HALF_FLOAT_PIXEL])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_R16_FLOAT);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);

        format = get_format_gl_internal(adapter, WINED3DFMT_R16G16_FLOAT);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);

        format = get_format_gl_internal(adapter, WINED3DFMT_R16G16B16A16_FLOAT);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);
    }

    if (gl_info->quirks & WINED3D_QUIRK_BROKEN_RGBA16)
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_R16G16B16A16_UNORM);
        format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);
    }

    /* ATI instancing hack: Although ATI cards do not support Shader Model
     * 3.0, they support instancing. To query if the card supports instancing
     * CheckDeviceFormat() with the special format MAKEFOURCC('I','N','S','T')
     * is used. Should an application check for this, provide a proper return
     * value. We can do instancing with all shader versions, but we need
     * vertex shaders.
     *
     * Additionally applications have to set the D3DRS_POINTSIZE render state
     * to MAKEFOURCC('I','N','S','T') once to enable instancing. Wined3d
     * doesn't need that and just ignores it.
     *
     * With Shader Model 3.0 capable cards Instancing 'just works' in Windows. */
    /* FIXME: This should just check the shader backend caps. */
    if (gl_info->supported[ARB_VERTEX_PROGRAM] || gl_info->supported[ARB_VERTEX_SHADER])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_INST);
        format_set_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);
    }

    /* Depth bound test. To query if the card supports it CheckDeviceFormat()
     * with the special format MAKEFOURCC('N','V','D','B') is used. It is
     * enabled by setting D3DRS_ADAPTIVETESS_X render state to
     * MAKEFOURCC('N','V','D','B') and then controlled by setting
     * D3DRS_ADAPTIVETESS_Z (zMin) and D3DRS_ADAPTIVETESS_W (zMax) to test
     * value. */
    if (gl_info->supported[EXT_DEPTH_BOUNDS_TEST])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_NVDB);
        format_set_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);
    }

    /* RESZ aka AMD DX9-level hack for multisampled depth buffer resolve. You query for RESZ
     * support by checking for availability of MAKEFOURCC('R','E','S','Z') surfaces with
     * RENDERTARGET usage. */
    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_RESZ);
        format_set_flag(&format->f, WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_RENDERTARGET);
    }

    for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
    {
        format = get_format_gl_by_idx(adapter, i);

        if (!(format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_TEXTURE))
            continue;

        if (is_identity_fixup(format->f.color_fixup))
            continue;

        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(format->f.color_fixup);
        if (!adapter->shader_backend->shader_color_fixup_supported(format->f.color_fixup)
                || !adapter->fragment_pipe->color_fixup_supported(format->f.color_fixup))
        {
            TRACE("[FAILED]\n");
            format_clear_flag(&format->f, WINED3DFMT_FLAG_TEXTURE);
        }
        else
        {
            TRACE("[OK]\n");
        }
    }

    /* These formats are not supported for 3D textures. See also
     * WINED3DFMT_FLAG_DECOMPRESS. */
    format = get_format_gl_internal(adapter, WINED3DFMT_ATI1N);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
    format = get_format_gl_internal(adapter, WINED3DFMT_ATI2N);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
    format = get_format_gl_internal(adapter, WINED3DFMT_BC4_UNORM);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
    format = get_format_gl_internal(adapter, WINED3DFMT_BC4_SNORM);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
    format = get_format_gl_internal(adapter, WINED3DFMT_BC5_UNORM);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
    format = get_format_gl_internal(adapter, WINED3DFMT_BC5_SNORM);
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] &= ~WINED3DFMT_FLAG_TEXTURE;
}

static BOOL init_format_vertex_info(const struct wined3d_adapter *adapter,
        struct wined3d_gl_info *gl_info)
{
    struct wined3d_format_gl *format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format_vertex_info); ++i)
    {
        if (!(format = get_format_gl_internal(adapter, format_vertex_info[i].id)))
            return FALSE;

        if (!gl_info->supported[format_vertex_info[i].extension])
            continue;

        format->f.emit_idx = format_vertex_info[i].emit_idx;
        format->vtx_type = format_vertex_info[i].gl_vtx_type;
        format->vtx_format = format->f.component_count;
        format->f.flags[WINED3D_GL_RES_TYPE_BUFFER] |= WINED3DFMT_FLAG_VERTEX_ATTRIBUTE;
    }

    if (gl_info->supported[ARB_VERTEX_ARRAY_BGRA])
    {
        format = get_format_gl_internal(adapter, WINED3DFMT_B8G8R8A8_UNORM);
        format->vtx_format = GL_BGRA;
    }

    return TRUE;
}

static BOOL init_typeless_formats(const struct wined3d_adapter *adapter)
{
    unsigned int flags[WINED3D_GL_RES_TYPE_COUNT];
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(typed_formats); ++i)
    {
        struct wined3d_format *format, *typeless_format;

        if (!(format = get_format_internal(adapter, typed_formats[i].id)))
            return FALSE;
        if (!(typeless_format = get_format_internal(adapter, typed_formats[i].typeless_id)))
            return FALSE;

        memcpy(flags, typeless_format->flags, sizeof(flags));
        copy_format(adapter, typeless_format, format);
        for (j = 0; j < ARRAY_SIZE(typeless_format->flags); ++j)
            typeless_format->flags[j] |= flags[j];
    }

    for (i = 0; i < ARRAY_SIZE(typeless_depth_stencil_formats); ++i)
    {
        struct wined3d_format *typeless_format, *typeless_ds_format, *ds_format;
        struct wined3d_format *depth_view_format, *stencil_view_format;
        enum wined3d_format_id format_id;

        if (!(typeless_format = get_format_internal(adapter, typeless_depth_stencil_formats[i].typeless_id)))
            return FALSE;
        if (!(ds_format = get_format_internal(adapter, typeless_depth_stencil_formats[i].depth_stencil_id)))
            return FALSE;

        typeless_ds_format = get_format_by_idx(adapter, WINED3D_FORMAT_COUNT + i);
        typeless_ds_format->id = typeless_depth_stencil_formats[i].typeless_id;
        copy_format(adapter, typeless_ds_format, ds_format);
        for (j = 0; j < ARRAY_SIZE(typeless_ds_format->flags); ++j)
        {
            typeless_ds_format->flags[j] = typeless_format->flags[j];
            typeless_format->flags[j] &= ~(WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
        }

        if ((format_id = typeless_depth_stencil_formats[i].depth_view_id)
                && typeless_depth_stencil_formats[i].separate_depth_view_format)
        {
            if (!(depth_view_format = get_format_internal(adapter, format_id)))
                return FALSE;
            copy_format(adapter, depth_view_format, ds_format);
        }
        if ((format_id = typeless_depth_stencil_formats[i].stencil_view_id))
        {
            if (!(stencil_view_format = get_format_internal(adapter, format_id)))
                return FALSE;
            copy_format(adapter, stencil_view_format, ds_format);
        }
    }

    return TRUE;
}

static void init_format_gen_mipmap_info(const struct wined3d_adapter *adapter,
        struct wined3d_gl_info *gl_info)
{
    unsigned int i, j;

    if (!gl_info->fbo_ops.glGenerateMipmap)
        return;

    for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
    {
        struct wined3d_format *format = get_format_by_idx(adapter, i);

        for (j = 0; j < ARRAY_SIZE(format->flags); ++j)
            if (!(~format->flags[j] & (WINED3DFMT_FLAG_RENDERTARGET | WINED3DFMT_FLAG_FILTERING)))
                format->flags[j] |= WINED3DFMT_FLAG_GEN_MIPMAP;
    }
}

BOOL wined3d_caps_gl_ctx_test_viewport_subpixel_bits(struct wined3d_caps_gl_ctx *ctx)
{
    static const struct wined3d_color red = {1.0f, 0.0f, 0.0f, 1.0f};
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    static const float offset = -63.0f / 128.0f;
    GLuint texture, fbo;
    DWORD readback[4];
    unsigned int i;

    gl_info->gl_ops.gl.p_glGenTextures(1, &texture);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, texture);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ARRAY_SIZE(readback), 1, 0,
            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, texture, 0);
    checkGLcall("create resources");

    gl_info->gl_ops.gl.p_glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT);
    GL_EXTCALL(glViewportIndexedf(0, offset, offset, 4.0f, 1.0f));
    draw_test_quad(ctx, NULL, &red);
    checkGLcall("draw");

    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, texture);
    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0,
            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, readback);
    checkGLcall("readback");

    TRACE("Readback colors are 0x%08x, 0x%08x, 0x%08x, 0x%08x.\n",
            readback[0], readback[1], readback[2], readback[3]);

    gl_info->gl_ops.gl.p_glDeleteTextures(1, &texture);
    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLcall("delete resources");

    for (i = 0; i < ARRAY_SIZE(readback); ++i)
    {
        if (readback[i] != 0xffff0000)
            return FALSE;
    }
    return TRUE;
}

static float wined3d_adapter_find_polyoffset_scale(struct wined3d_caps_gl_ctx *ctx, GLenum format)
{
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    static const struct wined3d_color blue = {0.0f, 0.0f, 1.0f, 1.0f};
    GLuint fbo, color, depth;
    unsigned int low = 0, high = 32, cur;
    DWORD readback[256];
    static const struct wined3d_vec3 geometry[] =
    {
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  0.0f},
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  0.0f},
    };

    /* Most drivers want 2^23 for fixed point depth buffers, including r300g, r600g,
     * Nvidia. Use this as a fallback if the detection fails. */
    unsigned int fallback = 23;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
    {
        FIXME("No FBOs, assuming polyoffset scale of 2^%u.\n", fallback);
        return (float)(1u << fallback);
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &color);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, color);
    gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);

    gl_info->fbo_ops.glGenRenderbuffers(1, &depth);
    gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, depth);
    gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format, 256, 1);

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    checkGLcall("Setup framebuffer");

    gl_info->gl_ops.gl.p_glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    gl_info->gl_ops.gl.p_glClearDepth(0.5f);
    gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
    gl_info->gl_ops.gl.p_glEnable(GL_POLYGON_OFFSET_FILL);
    gl_info->gl_ops.gl.p_glViewport(0, 0, 256, 1);
    checkGLcall("Misc parameters");

    for (;;)
    {
        if (high - low <= 1)
        {
            ERR("PolygonOffset scale factor detection failed, using fallback value 2^%u.\n", fallback);
            cur = fallback;
            break;
        }
        cur = (low + high) / 2;

        gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* The post viewport transform Z of the geometry runs from 0.0 to 0.5. We want to push it another
         * 0.25 so that the Z buffer content (0.5) cuts the quad off at half the screen. */
        gl_info->gl_ops.gl.p_glPolygonOffset(0.0f, (float)(1u << cur) * 0.25f);
        draw_test_quad(ctx, geometry, &blue);
        checkGLcall("Test draw");

        /* Rebinding texture to workaround a fglrx bug. */
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, color);
        gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, readback);
        checkGLcall("readback");

        TRACE("low %02u, high %02u, cur %2u, 0=0x%08x, 125=0x%08x, 131=0x%08x, 255=0x%08x\n",
                low, high, cur, readback[0], readback[125], readback[131], readback[255]);

        if ((readback[125] & 0xff) < 0xa0)
            high = cur;
        else if ((readback[131] & 0xff) > 0xa0)
            low = cur;
        else
        {
            TRACE("Found scale factor 2^%u for format %x.\n", cur, format);
            break;
        }
    }

    gl_info->gl_ops.gl.p_glDeleteTextures(1, &color);
    gl_info->fbo_ops.glDeleteRenderbuffers(1, &depth);
    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLcall("Delete framebuffer");

    gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_POLYGON_OFFSET_FILL);
    return (float)(1u << cur);
}

static void init_format_depth_bias_scale(struct wined3d_adapter *adapter,
        struct wined3d_caps_gl_ctx *ctx)
{
    const struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    unsigned int i;

    for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
    {
        struct wined3d_format_gl *format = get_format_gl_by_idx(adapter, i);

        if (format->f.flags[WINED3D_GL_RES_TYPE_RB] & WINED3DFMT_FLAG_DEPTH)
        {
            TRACE("Testing depth bias scale for format %s.\n", debug_d3dformat(format->f.id));
            format->f.depth_bias_scale = wined3d_adapter_find_polyoffset_scale(ctx, format->internal);

            if (!(d3d_info->wined3d_creation_flags & WINED3D_NORMALIZED_DEPTH_BIAS))
            {
                /* The single-precision binary floating-point format has
                 * a significand precision of 24 bits.
                 */
                if (format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
                    format->f.depth_bias_scale /= 1u << 24;
                else
                    format->f.depth_bias_scale /= 1u << format->f.depth_size;
            }
        }
    }
}

static BOOL wined3d_adapter_init_format_info(struct wined3d_adapter *adapter, size_t format_size)
{
    unsigned int count = WINED3D_FORMAT_COUNT + ARRAY_SIZE(typeless_depth_stencil_formats);

    if (!(adapter->formats = heap_calloc(count, format_size)))
    {
        ERR("Failed to allocate memory.\n");
        return FALSE;
    }
    adapter->format_size = format_size;

    if (!init_format_base_info(adapter))
        goto fail;
    if (!init_format_block_info(adapter))
        goto fail;
    if (!init_format_decompress_info(adapter))
        goto fail;
    if (!init_srgb_formats(adapter))
        goto fail;

    return TRUE;

fail:
    heap_free(adapter->formats);
    adapter->formats = NULL;
    return FALSE;
}

BOOL wined3d_adapter_no3d_init_format_info(struct wined3d_adapter *adapter)
{
    struct wined3d_format *format;
    unsigned int i;

    static const enum wined3d_format_id blit_formats[] =
    {
        WINED3DFMT_B8G8R8A8_UNORM,
        WINED3DFMT_B8G8R8X8_UNORM,
        WINED3DFMT_B5G6R5_UNORM,
        WINED3DFMT_B5G5R5X1_UNORM,
        WINED3DFMT_B5G5R5A1_UNORM,
        WINED3DFMT_B4G4R4A4_UNORM,
        WINED3DFMT_B2G3R3_UNORM,
        WINED3DFMT_A8_UNORM,
        WINED3DFMT_B2G3R3A8_UNORM,
        WINED3DFMT_B4G4R4X4_UNORM,
        WINED3DFMT_R10G10B10A2_UNORM,
        WINED3DFMT_R8G8B8A8_UNORM,
        WINED3DFMT_R8G8B8X8_UNORM,
        WINED3DFMT_R16G16_UNORM,
        WINED3DFMT_B10G10R10A2_UNORM,
        WINED3DFMT_R16G16B16A16_UNORM,
        WINED3DFMT_P8_UINT,
    };

    if (!wined3d_adapter_init_format_info(adapter, sizeof(struct wined3d_format)))
        return FALSE;

    for (i = 0; i < ARRAY_SIZE(blit_formats); ++i)
    {
        if (!(format = get_format_internal(adapter, blit_formats[i])))
            return FALSE;

        format->flags[WINED3D_GL_RES_TYPE_TEX_2D] |= WINED3DFMT_FLAG_BLIT;
        format->flags[WINED3D_GL_RES_TYPE_RB] |= WINED3DFMT_FLAG_BLIT;
    }

    return TRUE;
}

/* Context activation is done by the caller. */
BOOL wined3d_adapter_gl_init_format_info(struct wined3d_adapter *adapter, struct wined3d_caps_gl_ctx *ctx)
{
    struct wined3d_gl_info *gl_info = &adapter->gl_info;

    if (!wined3d_adapter_init_format_info(adapter, sizeof(struct wined3d_format_gl)))
        return FALSE;

    if (!init_format_texture_info(adapter, gl_info)) goto fail;
    if (!init_format_vertex_info(adapter, gl_info)) goto fail;

    apply_format_fixups(adapter, gl_info);
    init_format_fbo_compat_info(adapter, ctx);
    init_format_filter_info(adapter, gl_info);
    init_format_gen_mipmap_info(adapter, gl_info);
    init_format_depth_bias_scale(adapter, ctx);

    if (!init_typeless_formats(adapter)) goto fail;

    return TRUE;

fail:
    heap_free(adapter->formats);
    adapter->formats = NULL;
    return FALSE;
}

static void init_vulkan_format_info(struct wined3d_format_vk *format,
        const struct wined3d_vk_info *vk_info, VkPhysicalDevice vk_physical_device)
{
    static const struct
    {
        enum wined3d_format_id id;
        VkFormat vk_format;
    }
    vulkan_formats[] =
    {
        {WINED3DFMT_R32G32B32A32_FLOAT,    VK_FORMAT_R32G32B32A32_SFLOAT,     },
        {WINED3DFMT_R32G32B32A32_UINT,     VK_FORMAT_R32G32B32A32_UINT,       },
        {WINED3DFMT_R32G32B32A32_SINT,     VK_FORMAT_R32G32B32A32_SINT,       },
        {WINED3DFMT_R32G32B32_FLOAT,       VK_FORMAT_R32G32B32_SFLOAT,        },
        {WINED3DFMT_R32G32B32_UINT,        VK_FORMAT_R32G32B32_UINT,          },
        {WINED3DFMT_R32G32B32_SINT,        VK_FORMAT_R32G32B32_SINT,          },
        {WINED3DFMT_R16G16B16A16_FLOAT,    VK_FORMAT_R16G16B16A16_SFLOAT,     },
        {WINED3DFMT_R16G16B16A16_UNORM,    VK_FORMAT_R16G16B16A16_UNORM,      },
        {WINED3DFMT_R16G16B16A16_UINT,     VK_FORMAT_R16G16B16A16_UINT,       },
        {WINED3DFMT_R16G16B16A16_SNORM,    VK_FORMAT_R16G16B16A16_SNORM,      },
        {WINED3DFMT_R16G16B16A16_SINT,     VK_FORMAT_R16G16B16A16_SINT,       },
        {WINED3DFMT_R32G32_FLOAT,          VK_FORMAT_R32G32_SFLOAT,           },
        {WINED3DFMT_R32G32_UINT,           VK_FORMAT_R32G32_UINT,             },
        {WINED3DFMT_R32G32_SINT,           VK_FORMAT_R32G32_SINT,             },
        {WINED3DFMT_R10G10B10A2_UNORM,     VK_FORMAT_A2B10G10R10_UNORM_PACK32,},
        {WINED3DFMT_R11G11B10_FLOAT,       VK_FORMAT_B10G11R11_UFLOAT_PACK32, },
        {WINED3DFMT_R8G8_UNORM,            VK_FORMAT_R8G8_UNORM,              },
        {WINED3DFMT_R8G8_UINT,             VK_FORMAT_R8G8_UINT,               },
        {WINED3DFMT_R8G8_SNORM,            VK_FORMAT_R8G8_SNORM,              },
        {WINED3DFMT_R8G8_SINT,             VK_FORMAT_R8G8_SINT,               },
        {WINED3DFMT_R8G8B8A8_UNORM,        VK_FORMAT_R8G8B8A8_UNORM,          },
        {WINED3DFMT_R8G8B8A8_UNORM_SRGB,   VK_FORMAT_R8G8B8A8_SRGB,           },
        {WINED3DFMT_R8G8B8A8_UINT,         VK_FORMAT_R8G8B8A8_UINT,           },
        {WINED3DFMT_R8G8B8A8_SNORM,        VK_FORMAT_R8G8B8A8_SNORM,          },
        {WINED3DFMT_R8G8B8A8_SINT,         VK_FORMAT_R8G8B8A8_SINT,           },
        {WINED3DFMT_R16G16_FLOAT,          VK_FORMAT_R16G16_SFLOAT,           },
        {WINED3DFMT_R16G16_UNORM,          VK_FORMAT_R16G16_UNORM,            },
        {WINED3DFMT_R16G16_UINT,           VK_FORMAT_R16G16_UINT,             },
        {WINED3DFMT_R16G16_SNORM,          VK_FORMAT_R16G16_SNORM,            },
        {WINED3DFMT_R16G16_SINT,           VK_FORMAT_R16G16_SINT,             },
        {WINED3DFMT_D32_FLOAT,             VK_FORMAT_D32_SFLOAT,              },
        {WINED3DFMT_R32_FLOAT,             VK_FORMAT_R32_SFLOAT,              },
        {WINED3DFMT_R32_UINT,              VK_FORMAT_R32_UINT,                },
        {WINED3DFMT_R32_SINT,              VK_FORMAT_R32_SINT,                },
        {WINED3DFMT_R16_FLOAT,             VK_FORMAT_R16_SFLOAT,              },
        {WINED3DFMT_D16_UNORM,             VK_FORMAT_D16_UNORM,               },
        {WINED3DFMT_R16_UNORM,             VK_FORMAT_R16_UNORM,               },
        {WINED3DFMT_R16_UINT,              VK_FORMAT_R16_UINT,                },
        {WINED3DFMT_R16_SNORM,             VK_FORMAT_R16_SNORM,               },
        {WINED3DFMT_R16_SINT,              VK_FORMAT_R16_SINT,                },
        {WINED3DFMT_R8_UNORM,              VK_FORMAT_R8_UNORM,                },
        {WINED3DFMT_R8_UINT,               VK_FORMAT_R8_UINT,                 },
        {WINED3DFMT_R8_SNORM,              VK_FORMAT_R8_SNORM,                },
        {WINED3DFMT_R8_SINT,               VK_FORMAT_R8_SINT,                 },
        {WINED3DFMT_A8_UNORM,              VK_FORMAT_R8_UNORM,                },
        {WINED3DFMT_B8G8R8A8_UNORM,        VK_FORMAT_B8G8R8A8_UNORM,          },
        {WINED3DFMT_B8G8R8A8_UNORM_SRGB,   VK_FORMAT_B8G8R8A8_SRGB,           },
        {WINED3DFMT_BC1_UNORM,             VK_FORMAT_BC1_RGBA_UNORM_BLOCK,    },
        {WINED3DFMT_BC1_UNORM_SRGB,        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,     },
        {WINED3DFMT_BC2_UNORM,             VK_FORMAT_BC2_UNORM_BLOCK,         },
        {WINED3DFMT_BC2_UNORM_SRGB,        VK_FORMAT_BC2_SRGB_BLOCK,          },
        {WINED3DFMT_BC3_UNORM,             VK_FORMAT_BC3_UNORM_BLOCK,         },
        {WINED3DFMT_BC3_UNORM_SRGB,        VK_FORMAT_BC3_SRGB_BLOCK,          },
        {WINED3DFMT_BC4_UNORM,             VK_FORMAT_BC4_UNORM_BLOCK,         },
        {WINED3DFMT_BC4_SNORM,             VK_FORMAT_BC4_SNORM_BLOCK,         },
        {WINED3DFMT_BC5_UNORM,             VK_FORMAT_BC5_UNORM_BLOCK,         },
        {WINED3DFMT_BC5_SNORM,             VK_FORMAT_BC5_SNORM_BLOCK,         },
        {WINED3DFMT_BC6H_UF16,             VK_FORMAT_BC6H_UFLOAT_BLOCK,       },
        {WINED3DFMT_BC6H_SF16,             VK_FORMAT_BC6H_SFLOAT_BLOCK,       },
        {WINED3DFMT_BC7_UNORM,             VK_FORMAT_BC7_UNORM_BLOCK,         },
        {WINED3DFMT_BC7_UNORM_SRGB,        VK_FORMAT_BC7_SRGB_BLOCK,          },
    };
    VkFormat vk_format = VK_FORMAT_UNDEFINED;
    VkImageFormatProperties image_properties;
    VkFormatFeatureFlags texture_flags;
    VkFormatProperties properties;
    VkImageUsageFlags vk_usage;
    unsigned int flags;
    unsigned int i;
    VkResult vr;

    for (i = 0; i < ARRAY_SIZE(vulkan_formats); ++i)
    {
        if (vulkan_formats[i].id == format->f.id)
        {
            vk_format = vulkan_formats[i].vk_format;
            break;
        }
    }
    if (!vk_format)
    {
        WARN("Unsupported format %s.\n", debug_d3dformat(format->f.id));
        return;
    }

    format->vk_format = vk_format;

    VK_CALL(vkGetPhysicalDeviceFormatProperties(vk_physical_device, vk_format, &properties));

    if (properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
        format->f.flags[WINED3D_GL_RES_TYPE_BUFFER] |= WINED3DFMT_FLAG_VERTEX_ATTRIBUTE;
    if (properties.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
        format->f.flags[WINED3D_GL_RES_TYPE_BUFFER] |= WINED3DFMT_FLAG_TEXTURE;

    flags = 0;
    texture_flags = properties.linearTilingFeatures | properties.optimalTilingFeatures;
    if (texture_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
    {
        flags |= WINED3DFMT_FLAG_TEXTURE | WINED3DFMT_FLAG_VTF;
    }
    if (texture_flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
    {
        flags |= WINED3DFMT_FLAG_RENDERTARGET;
    }
    if (texture_flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
    {
        flags |= WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
    }
    if (texture_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
    {
        flags |= WINED3DFMT_FLAG_FILTERING;
    }

    format->f.flags[WINED3D_GL_RES_TYPE_TEX_1D] |= flags;
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_2D] |= flags;
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_3D] |= flags;
    format->f.flags[WINED3D_GL_RES_TYPE_TEX_CUBE] |= flags;

    vk_usage = 0;
    if (texture_flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        vk_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if (texture_flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        vk_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (vk_usage)
    {
        if ((vr = VK_CALL(vkGetPhysicalDeviceImageFormatProperties(vk_physical_device, vk_format,
                VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, vk_usage, 0, &image_properties))) < 0)
        {
            ERR("Failed to get image format properties, vr %s.\n", wined3d_debug_vkresult(vr));
            return;
        }
        format->f.multisample_types = image_properties.sampleCounts;
    }
}

BOOL wined3d_adapter_vk_init_format_info(struct wined3d_adapter_vk *adapter_vk,
        const struct wined3d_vk_info *vk_info)
{
    VkPhysicalDevice vk_physical_device = adapter_vk->physical_device;
    struct wined3d_adapter *adapter = &adapter_vk->a;
    struct wined3d_format_vk *format;
    unsigned int i;

    if (!wined3d_adapter_init_format_info(adapter, sizeof(*format)))
        return FALSE;

    for (i = 0; i < WINED3D_FORMAT_COUNT; ++i)
    {
        format = wined3d_format_vk_mutable(get_format_by_idx(adapter, i));

        if (format->f.id)
            init_vulkan_format_info(format, vk_info, vk_physical_device);
    }

    if (!init_typeless_formats(adapter)) goto fail;

    return TRUE;

fail:
    heap_free(adapter->formats);
    adapter->formats = NULL;
    return FALSE;
}

const struct wined3d_format *wined3d_get_format(const struct wined3d_adapter *adapter,
        enum wined3d_format_id format_id, unsigned int bind_flags)
{
    const struct wined3d_format *format;
    int idx = get_format_idx(format_id);
    unsigned int i;

    if (idx == -1)
    {
        FIXME("Can't find format %s (%#x) in the format lookup table.\n",
                debug_d3dformat(format_id), format_id);
        return get_format_internal(adapter, WINED3DFMT_UNKNOWN);
    }

    format = get_format_by_idx(adapter, idx);

    if (bind_flags & WINED3D_BIND_DEPTH_STENCIL && wined3d_format_is_typeless(format))
    {
        for (i = 0; i < ARRAY_SIZE(typeless_depth_stencil_formats); ++i)
        {
            if (typeless_depth_stencil_formats[i].typeless_id == format_id)
                return get_format_by_idx(adapter, WINED3D_FORMAT_COUNT + i);
        }

        FIXME("Cannot find depth/stencil typeless format %s (%#x).\n",
                debug_d3dformat(format_id), format_id);
        return get_format_internal(adapter, WINED3DFMT_UNKNOWN);
    }

    return format;
}

BOOL wined3d_format_is_depth_view(enum wined3d_format_id resource_format_id,
        enum wined3d_format_id view_format_id)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(typeless_depth_stencil_formats); ++i)
    {
        if (typeless_depth_stencil_formats[i].typeless_id == resource_format_id)
            return typeless_depth_stencil_formats[i].depth_view_id == view_format_id;
    }
    return FALSE;
}

void wined3d_format_calculate_pitch(const struct wined3d_format *format, unsigned int alignment,
        unsigned int width, unsigned int height, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    /* For block based formats, pitch means the amount of bytes to the next
     * row of blocks rather than the next row of pixels. */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_BLOCKS)
    {
        unsigned int row_block_count = (width + format->block_width - 1) / format->block_width;
        unsigned int slice_block_count = (height + format->block_height - 1) / format->block_height;
        *row_pitch = row_block_count * format->block_byte_count;
        *row_pitch = (*row_pitch + alignment - 1) & ~(alignment - 1);
        *slice_pitch = *row_pitch * slice_block_count;
    }
    else
    {
        *row_pitch = format->byte_count * width;  /* Bytes / row */
        *row_pitch = (*row_pitch + alignment - 1) & ~(alignment - 1);
        *slice_pitch = *row_pitch * height;
    }

    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_HEIGHT_SCALE)
    {
        /* The D3D format requirements make sure that the resulting format is an integer again */
        *slice_pitch *= format->height_scale.numerator;
        *slice_pitch /= format->height_scale.denominator;
    }

    TRACE("Returning row pitch %u, slice pitch %u.\n", *row_pitch, *slice_pitch);
}

UINT wined3d_format_calculate_size(const struct wined3d_format *format, UINT alignment,
        UINT width, UINT height, UINT depth)
{
    unsigned int row_pitch, slice_pitch;

    if (format->id == WINED3DFMT_UNKNOWN)
        return 0;

    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_BROKEN_PITCH)
        return width * height * depth * format->byte_count;

    wined3d_format_calculate_pitch(format, alignment, width, height, &row_pitch, &slice_pitch);

    return slice_pitch * depth;
}

BOOL wined3d_formats_are_srgb_variants(enum wined3d_format_id format1, enum wined3d_format_id format2)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(format_srgb_info); ++i)
    {
        if (format1 == format_srgb_info[i].srgb_format_id)
            return format2 == format_srgb_info[i].base_format_id;
        if (format1 == format_srgb_info[i].base_format_id)
            return format2 == format_srgb_info[i].srgb_format_id;
    }
    return FALSE;
}

/*****************************************************************************
 * Trace formatting of useful values
 */
const char *debug_box(const struct wined3d_box *box)
{
    if (!box)
        return "(null)";
    return wine_dbg_sprintf("(%u, %u, %u)-(%u, %u, %u)",
            box->left, box->top, box->front,
            box->right, box->bottom, box->back);
}

const char *debug_color(const struct wined3d_color *color)
{
    if (!color)
        return "(null)";
    return wine_dbg_sprintf("{%.8e, %.8e, %.8e, %.8e}",
            color->r, color->g, color->b, color->a);
}

const char *debug_ivec4(const struct wined3d_ivec4 *v)
{
    if (!v)
        return "(null)";
    return wine_dbg_sprintf("{%d, %d, %d, %d}",
            v->x, v->y, v->z, v->w);
}

const char *debug_uvec4(const struct wined3d_uvec4 *v)
{
    if (!v)
        return "(null)";
    return wine_dbg_sprintf("{%u, %u, %u, %u}",
            v->x, v->y, v->z, v->w);
}

const char *debug_vec4(const struct wined3d_vec4 *v)
{
    if (!v)
        return "(null)";
    return wine_dbg_sprintf("{%.8e, %.8e, %.8e, %.8e}",
            v->x, v->y, v->z, v->w);
}

const char *debug_const_bo_address(const struct wined3d_const_bo_address *address)
{
    if (!address)
        return "(null)";
    return wine_dbg_sprintf("{%#lx:%p}", address->buffer_object, address->addr);
}

const char *debug_bo_address(const struct wined3d_bo_address *address)
{
    return debug_const_bo_address((const struct wined3d_const_bo_address *)address);
}

const char *debug_d3dformat(enum wined3d_format_id format_id)
{
    switch (format_id)
    {
#define FMT_TO_STR(format_id) case format_id: return #format_id
        FMT_TO_STR(WINED3DFMT_UNKNOWN);
        FMT_TO_STR(WINED3DFMT_B8G8R8_UNORM);
        FMT_TO_STR(WINED3DFMT_B5G5R5X1_UNORM);
        FMT_TO_STR(WINED3DFMT_B4G4R4A4_UNORM);
        FMT_TO_STR(WINED3DFMT_B2G3R3_UNORM);
        FMT_TO_STR(WINED3DFMT_B2G3R3A8_UNORM);
        FMT_TO_STR(WINED3DFMT_B4G4R4X4_UNORM);
        FMT_TO_STR(WINED3DFMT_R8G8B8X8_UNORM);
        FMT_TO_STR(WINED3DFMT_B10G10R10A2_UNORM);
        FMT_TO_STR(WINED3DFMT_P8_UINT_A8_UNORM);
        FMT_TO_STR(WINED3DFMT_P8_UINT);
        FMT_TO_STR(WINED3DFMT_L8_UNORM);
        FMT_TO_STR(WINED3DFMT_L8A8_UNORM);
        FMT_TO_STR(WINED3DFMT_L4A4_UNORM);
        FMT_TO_STR(WINED3DFMT_R5G5_SNORM_L6_UNORM);
        FMT_TO_STR(WINED3DFMT_R8G8_SNORM_L8X8_UNORM);
        FMT_TO_STR(WINED3DFMT_R10G11B11_SNORM);
        FMT_TO_STR(WINED3DFMT_R10G10B10X2_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R10G10B10X2_UINT);
        FMT_TO_STR(WINED3DFMT_R10G10B10X2_SNORM);
        FMT_TO_STR(WINED3DFMT_R10G10B10_SNORM_A2_UNORM);
        FMT_TO_STR(WINED3DFMT_D16_LOCKABLE);
        FMT_TO_STR(WINED3DFMT_D32_UNORM);
        FMT_TO_STR(WINED3DFMT_S1_UINT_D15_UNORM);
        FMT_TO_STR(WINED3DFMT_X8D24_UNORM);
        FMT_TO_STR(WINED3DFMT_S4X4_UINT_D24_UNORM);
        FMT_TO_STR(WINED3DFMT_L16_UNORM);
        FMT_TO_STR(WINED3DFMT_S8_UINT_D24_FLOAT);
        FMT_TO_STR(WINED3DFMT_R8G8_SNORM_Cx);
        FMT_TO_STR(WINED3DFMT_R32G32B32A32_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R32G32B32A32_FLOAT);
        FMT_TO_STR(WINED3DFMT_R32G32B32A32_UINT);
        FMT_TO_STR(WINED3DFMT_R32G32B32A32_SINT);
        FMT_TO_STR(WINED3DFMT_R32G32B32_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R32G32B32_FLOAT);
        FMT_TO_STR(WINED3DFMT_R32G32B32_UINT);
        FMT_TO_STR(WINED3DFMT_R32G32B32_SINT);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_FLOAT);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_UNORM);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_UINT);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_SNORM);
        FMT_TO_STR(WINED3DFMT_R16G16B16A16_SINT);
        FMT_TO_STR(WINED3DFMT_R32G32_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R32G32_FLOAT);
        FMT_TO_STR(WINED3DFMT_R32G32_UINT);
        FMT_TO_STR(WINED3DFMT_R32G32_SINT);
        FMT_TO_STR(WINED3DFMT_R32G8X24_TYPELESS);
        FMT_TO_STR(WINED3DFMT_D32_FLOAT_S8X24_UINT);
        FMT_TO_STR(WINED3DFMT_R32_FLOAT_X8X24_TYPELESS);
        FMT_TO_STR(WINED3DFMT_X32_TYPELESS_G8X24_UINT);
        FMT_TO_STR(WINED3DFMT_R10G10B10A2_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R10G10B10A2_UNORM);
        FMT_TO_STR(WINED3DFMT_R10G10B10A2_UINT);
        FMT_TO_STR(WINED3DFMT_R10G10B10A2_SNORM);
        FMT_TO_STR(WINED3DFMT_R10G10B10_XR_BIAS_A2_UNORM);
        FMT_TO_STR(WINED3DFMT_R11G11B10_FLOAT);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_UNORM);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_UINT);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_SNORM);
        FMT_TO_STR(WINED3DFMT_R8G8B8A8_SINT);
        FMT_TO_STR(WINED3DFMT_R16G16_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R16G16_FLOAT);
        FMT_TO_STR(WINED3DFMT_R16G16_UNORM);
        FMT_TO_STR(WINED3DFMT_R16G16_UINT);
        FMT_TO_STR(WINED3DFMT_R16G16_SNORM);
        FMT_TO_STR(WINED3DFMT_R16G16_SINT);
        FMT_TO_STR(WINED3DFMT_R32_TYPELESS);
        FMT_TO_STR(WINED3DFMT_D32_FLOAT);
        FMT_TO_STR(WINED3DFMT_R32_FLOAT);
        FMT_TO_STR(WINED3DFMT_R32_UINT);
        FMT_TO_STR(WINED3DFMT_R32_SINT);
        FMT_TO_STR(WINED3DFMT_R24G8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_D24_UNORM_S8_UINT);
        FMT_TO_STR(WINED3DFMT_R24_UNORM_X8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_X24_TYPELESS_G8_UINT);
        FMT_TO_STR(WINED3DFMT_R8G8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R8G8_UNORM);
        FMT_TO_STR(WINED3DFMT_R8G8_UINT);
        FMT_TO_STR(WINED3DFMT_R8G8_SNORM);
        FMT_TO_STR(WINED3DFMT_R8G8_SINT);
        FMT_TO_STR(WINED3DFMT_R16_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R16_FLOAT);
        FMT_TO_STR(WINED3DFMT_D16_UNORM);
        FMT_TO_STR(WINED3DFMT_R16_UNORM);
        FMT_TO_STR(WINED3DFMT_R16_UINT);
        FMT_TO_STR(WINED3DFMT_R16_SNORM);
        FMT_TO_STR(WINED3DFMT_R16_SINT);
        FMT_TO_STR(WINED3DFMT_R8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_R8_UNORM);
        FMT_TO_STR(WINED3DFMT_R8_UINT);
        FMT_TO_STR(WINED3DFMT_R8_SNORM);
        FMT_TO_STR(WINED3DFMT_R8_SINT);
        FMT_TO_STR(WINED3DFMT_A8_UNORM);
        FMT_TO_STR(WINED3DFMT_R1_UNORM);
        FMT_TO_STR(WINED3DFMT_R9G9B9E5_SHAREDEXP);
        FMT_TO_STR(WINED3DFMT_R8G8_B8G8_UNORM);
        FMT_TO_STR(WINED3DFMT_G8R8_G8B8_UNORM);
        FMT_TO_STR(WINED3DFMT_BC1_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC1_UNORM);
        FMT_TO_STR(WINED3DFMT_BC1_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_BC2_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC2_UNORM);
        FMT_TO_STR(WINED3DFMT_BC2_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_BC3_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC3_UNORM);
        FMT_TO_STR(WINED3DFMT_BC3_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_BC4_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC4_UNORM);
        FMT_TO_STR(WINED3DFMT_BC4_SNORM);
        FMT_TO_STR(WINED3DFMT_BC5_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC5_UNORM);
        FMT_TO_STR(WINED3DFMT_BC5_SNORM);
        FMT_TO_STR(WINED3DFMT_B5G6R5_UNORM);
        FMT_TO_STR(WINED3DFMT_B5G5R5A1_UNORM);
        FMT_TO_STR(WINED3DFMT_B8G8R8A8_UNORM);
        FMT_TO_STR(WINED3DFMT_B8G8R8X8_UNORM);
        FMT_TO_STR(WINED3DFMT_B8G8R8A8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_B8G8R8A8_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_B8G8R8X8_TYPELESS);
        FMT_TO_STR(WINED3DFMT_B8G8R8X8_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_BC6H_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC6H_UF16);
        FMT_TO_STR(WINED3DFMT_BC6H_SF16);
        FMT_TO_STR(WINED3DFMT_BC7_TYPELESS);
        FMT_TO_STR(WINED3DFMT_BC7_UNORM);
        FMT_TO_STR(WINED3DFMT_BC7_UNORM_SRGB);
        FMT_TO_STR(WINED3DFMT_UYVY);
        FMT_TO_STR(WINED3DFMT_YUY2);
        FMT_TO_STR(WINED3DFMT_YV12);
        FMT_TO_STR(WINED3DFMT_DXT1);
        FMT_TO_STR(WINED3DFMT_DXT2);
        FMT_TO_STR(WINED3DFMT_DXT3);
        FMT_TO_STR(WINED3DFMT_DXT4);
        FMT_TO_STR(WINED3DFMT_DXT5);
        FMT_TO_STR(WINED3DFMT_MULTI2_ARGB8);
        FMT_TO_STR(WINED3DFMT_G8R8_G8B8);
        FMT_TO_STR(WINED3DFMT_R8G8_B8G8);
        FMT_TO_STR(WINED3DFMT_ATI1N);
        FMT_TO_STR(WINED3DFMT_ATI2N);
        FMT_TO_STR(WINED3DFMT_INST);
        FMT_TO_STR(WINED3DFMT_NVDB);
        FMT_TO_STR(WINED3DFMT_NVHU);
        FMT_TO_STR(WINED3DFMT_NVHS);
        FMT_TO_STR(WINED3DFMT_INTZ);
        FMT_TO_STR(WINED3DFMT_RESZ);
        FMT_TO_STR(WINED3DFMT_NULL);
        FMT_TO_STR(WINED3DFMT_R16);
        FMT_TO_STR(WINED3DFMT_AL16);
        FMT_TO_STR(WINED3DFMT_NV12);
#undef FMT_TO_STR
        default:
        {
            char fourcc[5];
            fourcc[0] = (char)(format_id);
            fourcc[1] = (char)(format_id >> 8);
            fourcc[2] = (char)(format_id >> 16);
            fourcc[3] = (char)(format_id >> 24);
            fourcc[4] = 0;
            if (isprint(fourcc[0]) && isprint(fourcc[1]) && isprint(fourcc[2]) && isprint(fourcc[3]))
                FIXME("Unrecognized %#x (as fourcc: %s) WINED3DFORMAT!\n", format_id, fourcc);
            else
                FIXME("Unrecognized %#x WINED3DFORMAT!\n", format_id);
        }
        return "unrecognized";
    }
}

const char *debug_d3ddevicetype(enum wined3d_device_type device_type)
{
    switch (device_type)
    {
#define DEVTYPE_TO_STR(dev) case dev: return #dev
        DEVTYPE_TO_STR(WINED3D_DEVICE_TYPE_HAL);
        DEVTYPE_TO_STR(WINED3D_DEVICE_TYPE_REF);
        DEVTYPE_TO_STR(WINED3D_DEVICE_TYPE_SW);
#undef DEVTYPE_TO_STR
        default:
            FIXME("Unrecognized device type %#x.\n", device_type);
            return "unrecognized";
    }
}

struct debug_buffer
{
    char str[200]; /* wine_dbg_sprintf() limits string size to 200 */
    char *ptr;
    int size;
};

static void init_debug_buffer(struct debug_buffer *buffer, const char *default_string)
{
    snprintf(buffer->str, sizeof(buffer->str), "%s", default_string);
    buffer->ptr = buffer->str;
    buffer->size = sizeof(buffer->str);
}

static void debug_append(struct debug_buffer *buffer, const char *str, const char *separator)
{
    int size;

    if (!separator || buffer->ptr == buffer->str)
        separator = "";
    size = snprintf(buffer->ptr, buffer->size, "%s%s", separator, str);
    if (size == -1 || size >= buffer->size)
    {
        buffer->size = 0;
        strcpy(&buffer->str[sizeof(buffer->str) - 4], "...");
        return;
    }

    buffer->ptr += size;
    buffer->size -= size;
}

const char *wined3d_debug_resource_access(DWORD access)
{
    struct debug_buffer buffer;

    init_debug_buffer(&buffer, "0");
#define ACCESS_TO_STR(x) if (access & x) { debug_append(&buffer, #x, " | "); access &= ~x; }
    ACCESS_TO_STR(WINED3D_RESOURCE_ACCESS_GPU);
    ACCESS_TO_STR(WINED3D_RESOURCE_ACCESS_CPU);
    ACCESS_TO_STR(WINED3D_RESOURCE_ACCESS_MAP_R);
    ACCESS_TO_STR(WINED3D_RESOURCE_ACCESS_MAP_W);
#undef ACCESS_TO_STR
    if (access)
        FIXME("Unrecognised access flag(s) %#x.\n", access);

    return wine_dbg_sprintf("%s", buffer.str);
}

const char *wined3d_debug_bind_flags(DWORD bind_flags)
{
    struct debug_buffer buffer;

    init_debug_buffer(&buffer, "0");
#define BIND_FLAG_TO_STR(x) if (bind_flags & x) { debug_append(&buffer, #x, " | "); bind_flags &= ~x; }
    BIND_FLAG_TO_STR(WINED3D_BIND_VERTEX_BUFFER);
    BIND_FLAG_TO_STR(WINED3D_BIND_INDEX_BUFFER);
    BIND_FLAG_TO_STR(WINED3D_BIND_CONSTANT_BUFFER);
    BIND_FLAG_TO_STR(WINED3D_BIND_SHADER_RESOURCE);
    BIND_FLAG_TO_STR(WINED3D_BIND_STREAM_OUTPUT);
    BIND_FLAG_TO_STR(WINED3D_BIND_RENDER_TARGET);
    BIND_FLAG_TO_STR(WINED3D_BIND_DEPTH_STENCIL);
    BIND_FLAG_TO_STR(WINED3D_BIND_UNORDERED_ACCESS);
#undef BIND_FLAG_TO_STR
    if (bind_flags)
        FIXME("Unrecognised bind flag(s) %#x.\n", bind_flags);

    return wine_dbg_sprintf("%s", buffer.str);
}

const char *wined3d_debug_view_desc(const struct wined3d_view_desc *d, const struct wined3d_resource *resource)
{
    struct debug_buffer buffer;
    unsigned int flags = d->flags;

    init_debug_buffer(&buffer, "0");
#define VIEW_FLAG_TO_STR(x) if (flags & x) { debug_append(&buffer, #x, " | "); flags &= ~x; }
    VIEW_FLAG_TO_STR(WINED3D_VIEW_BUFFER_RAW);
    VIEW_FLAG_TO_STR(WINED3D_VIEW_BUFFER_APPEND);
    VIEW_FLAG_TO_STR(WINED3D_VIEW_BUFFER_COUNTER);
    VIEW_FLAG_TO_STR(WINED3D_VIEW_TEXTURE_CUBE);
    VIEW_FLAG_TO_STR(WINED3D_VIEW_TEXTURE_ARRAY);
#undef VIEW_FLAG_TO_STR
    if (flags)
        FIXME("Unrecognised view flag(s) %#x.\n", flags);

    if (resource->type == WINED3D_RTYPE_BUFFER)
        return wine_dbg_sprintf("format %s, flags %s, start_idx %u, count %u",
                debug_d3dformat(d->format_id), buffer.str, d->u.buffer.start_idx, d->u.buffer.count);
    else
        return wine_dbg_sprintf("format %s, flags %s, level_idx %u, level_count %u, layer_idx %u, layer_count %u",
                debug_d3dformat(d->format_id), buffer.str, d->u.texture.level_idx, d->u.texture.level_count,
                d->u.texture.layer_idx, d->u.texture.layer_count);
}

const char *debug_d3dusage(DWORD usage)
{
    struct debug_buffer buffer;

    init_debug_buffer(&buffer, "0");
#define WINED3DUSAGE_TO_STR(x) if (usage & x) { debug_append(&buffer, #x, " | "); usage &= ~x; }
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_SOFTWAREPROCESSING);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DONOTCLIP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_POINTS);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RTPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_NPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DYNAMIC);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RESTRICTED_CONTENT);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RESTRICT_SHARED_RESOURCE);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DMAP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_TEXTAPI);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_LEGACY_CUBEMAP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_OWNDC);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_STATICDECL);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_OVERLAY);
#undef WINED3DUSAGE_TO_STR
    if (usage)
        FIXME("Unrecognized usage flag(s) %#x.\n", usage);

    return wine_dbg_sprintf("%s", buffer.str);
}

const char *debug_d3dusagequery(DWORD usage)
{
    struct debug_buffer buffer;

    init_debug_buffer(&buffer, "0");
#define WINED3DUSAGEQUERY_TO_STR(x) if (usage & x) { debug_append(&buffer, #x, " | "); usage &= ~x; }
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_FILTER);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_GENMIPMAP);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_LEGACYBUMPMAP);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBREAD);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBWRITE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_VERTEXTEXTURE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_WRAPANDMIP);
#undef WINED3DUSAGEQUERY_TO_STR
    if (usage)
        FIXME("Unrecognized usage query flag(s) %#x.\n", usage);

    return wine_dbg_sprintf("%s", buffer.str);
}

const char *debug_d3ddeclmethod(enum wined3d_decl_method method)
{
    switch (method)
    {
#define WINED3DDECLMETHOD_TO_STR(u) case u: return #u
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_DEFAULT);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_PARTIAL_U);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_PARTIAL_V);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_CROSS_UV);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_UV);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_LOOKUP);
        WINED3DDECLMETHOD_TO_STR(WINED3D_DECL_METHOD_LOOKUP_PRESAMPLED);
#undef WINED3DDECLMETHOD_TO_STR
        default:
            FIXME("Unrecognized declaration method %#x.\n", method);
            return "unrecognized";
    }
}

const char *debug_d3ddeclusage(enum wined3d_decl_usage usage)
{
    switch (usage)
    {
#define WINED3DDECLUSAGE_TO_STR(u) case u: return #u
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_POSITION);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_BLEND_WEIGHT);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_BLEND_INDICES);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_NORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_PSIZE);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_TEXCOORD);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_TANGENT);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_BINORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_TESS_FACTOR);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_POSITIONT);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_COLOR);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_FOG);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_DEPTH);
        WINED3DDECLUSAGE_TO_STR(WINED3D_DECL_USAGE_SAMPLE);
#undef WINED3DDECLUSAGE_TO_STR
        default:
            FIXME("Unrecognized %u declaration usage!\n", usage);
            return "unrecognized";
    }
}

const char *debug_d3dinput_classification(enum wined3d_input_classification classification)
{
    switch (classification)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(WINED3D_INPUT_PER_VERTEX_DATA);
        WINED3D_TO_STR(WINED3D_INPUT_PER_INSTANCE_DATA);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized input classification %#x.\n", classification);
            return "unrecognized";
    }
}

const char *debug_d3dresourcetype(enum wined3d_resource_type resource_type)
{
    switch (resource_type)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(WINED3D_RTYPE_NONE);
        WINED3D_TO_STR(WINED3D_RTYPE_BUFFER);
        WINED3D_TO_STR(WINED3D_RTYPE_TEXTURE_1D);
        WINED3D_TO_STR(WINED3D_RTYPE_TEXTURE_2D);
        WINED3D_TO_STR(WINED3D_RTYPE_TEXTURE_3D);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized resource type %#x.\n", resource_type);
            return "unrecognized";
    }
}

const char *debug_d3dprimitivetype(enum wined3d_primitive_type primitive_type)
{
    switch (primitive_type)
    {
#define PRIM_TO_STR(prim) case prim: return #prim
        PRIM_TO_STR(WINED3D_PT_UNDEFINED);
        PRIM_TO_STR(WINED3D_PT_POINTLIST);
        PRIM_TO_STR(WINED3D_PT_LINELIST);
        PRIM_TO_STR(WINED3D_PT_LINESTRIP);
        PRIM_TO_STR(WINED3D_PT_TRIANGLELIST);
        PRIM_TO_STR(WINED3D_PT_TRIANGLESTRIP);
        PRIM_TO_STR(WINED3D_PT_TRIANGLEFAN);
        PRIM_TO_STR(WINED3D_PT_LINELIST_ADJ);
        PRIM_TO_STR(WINED3D_PT_LINESTRIP_ADJ);
        PRIM_TO_STR(WINED3D_PT_TRIANGLELIST_ADJ);
        PRIM_TO_STR(WINED3D_PT_TRIANGLESTRIP_ADJ);
        PRIM_TO_STR(WINED3D_PT_PATCH);
#undef  PRIM_TO_STR
        default:
            FIXME("Unrecognized primitive type %#x.\n", primitive_type);
            return "unrecognized";
    }
}

const char *debug_d3drenderstate(enum wined3d_render_state state)
{
    switch (state)
    {
#define D3DSTATE_TO_STR(u) case u: return #u
        D3DSTATE_TO_STR(WINED3D_RS_ANTIALIAS);
        D3DSTATE_TO_STR(WINED3D_RS_TEXTUREPERSPECTIVE);
        D3DSTATE_TO_STR(WINED3D_RS_WRAPU);
        D3DSTATE_TO_STR(WINED3D_RS_WRAPV);
        D3DSTATE_TO_STR(WINED3D_RS_ZENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_FILLMODE);
        D3DSTATE_TO_STR(WINED3D_RS_SHADEMODE);
        D3DSTATE_TO_STR(WINED3D_RS_LINEPATTERN);
        D3DSTATE_TO_STR(WINED3D_RS_MONOENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_ROP2);
        D3DSTATE_TO_STR(WINED3D_RS_PLANEMASK);
        D3DSTATE_TO_STR(WINED3D_RS_ZWRITEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_ALPHATESTENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_LASTPIXEL);
        D3DSTATE_TO_STR(WINED3D_RS_SRCBLEND);
        D3DSTATE_TO_STR(WINED3D_RS_DESTBLEND);
        D3DSTATE_TO_STR(WINED3D_RS_CULLMODE);
        D3DSTATE_TO_STR(WINED3D_RS_ZFUNC);
        D3DSTATE_TO_STR(WINED3D_RS_ALPHAREF);
        D3DSTATE_TO_STR(WINED3D_RS_ALPHAFUNC);
        D3DSTATE_TO_STR(WINED3D_RS_DITHERENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_ALPHABLENDENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_FOGENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_SPECULARENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_ZVISIBLE);
        D3DSTATE_TO_STR(WINED3D_RS_SUBPIXEL);
        D3DSTATE_TO_STR(WINED3D_RS_SUBPIXELX);
        D3DSTATE_TO_STR(WINED3D_RS_STIPPLEDALPHA);
        D3DSTATE_TO_STR(WINED3D_RS_FOGCOLOR);
        D3DSTATE_TO_STR(WINED3D_RS_FOGTABLEMODE);
        D3DSTATE_TO_STR(WINED3D_RS_FOGSTART);
        D3DSTATE_TO_STR(WINED3D_RS_FOGEND);
        D3DSTATE_TO_STR(WINED3D_RS_FOGDENSITY);
        D3DSTATE_TO_STR(WINED3D_RS_STIPPLEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_EDGEANTIALIAS);
        D3DSTATE_TO_STR(WINED3D_RS_COLORKEYENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_MIPMAPLODBIAS);
        D3DSTATE_TO_STR(WINED3D_RS_RANGEFOGENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_ANISOTROPY);
        D3DSTATE_TO_STR(WINED3D_RS_FLUSHBATCH);
        D3DSTATE_TO_STR(WINED3D_RS_TRANSLUCENTSORTINDEPENDENT);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILFAIL);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILZFAIL);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILPASS);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILFUNC);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILREF);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILMASK);
        D3DSTATE_TO_STR(WINED3D_RS_STENCILWRITEMASK);
        D3DSTATE_TO_STR(WINED3D_RS_TEXTUREFACTOR);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP0);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP1);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP2);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP3);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP4);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP5);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP6);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP7);
        D3DSTATE_TO_STR(WINED3D_RS_CLIPPING);
        D3DSTATE_TO_STR(WINED3D_RS_LIGHTING);
        D3DSTATE_TO_STR(WINED3D_RS_EXTENTS);
        D3DSTATE_TO_STR(WINED3D_RS_AMBIENT);
        D3DSTATE_TO_STR(WINED3D_RS_FOGVERTEXMODE);
        D3DSTATE_TO_STR(WINED3D_RS_COLORVERTEX);
        D3DSTATE_TO_STR(WINED3D_RS_LOCALVIEWER);
        D3DSTATE_TO_STR(WINED3D_RS_NORMALIZENORMALS);
        D3DSTATE_TO_STR(WINED3D_RS_COLORKEYBLENDENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_DIFFUSEMATERIALSOURCE);
        D3DSTATE_TO_STR(WINED3D_RS_SPECULARMATERIALSOURCE);
        D3DSTATE_TO_STR(WINED3D_RS_AMBIENTMATERIALSOURCE);
        D3DSTATE_TO_STR(WINED3D_RS_EMISSIVEMATERIALSOURCE);
        D3DSTATE_TO_STR(WINED3D_RS_VERTEXBLEND);
        D3DSTATE_TO_STR(WINED3D_RS_CLIPPLANEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_SOFTWAREVERTEXPROCESSING);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSIZE);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSIZE_MIN);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSPRITEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSCALEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSCALE_A);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSCALE_B);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSCALE_C);
        D3DSTATE_TO_STR(WINED3D_RS_MULTISAMPLEANTIALIAS);
        D3DSTATE_TO_STR(WINED3D_RS_MULTISAMPLEMASK);
        D3DSTATE_TO_STR(WINED3D_RS_PATCHEDGESTYLE);
        D3DSTATE_TO_STR(WINED3D_RS_PATCHSEGMENTS);
        D3DSTATE_TO_STR(WINED3D_RS_DEBUGMONITORTOKEN);
        D3DSTATE_TO_STR(WINED3D_RS_POINTSIZE_MAX);
        D3DSTATE_TO_STR(WINED3D_RS_INDEXEDVERTEXBLENDENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_COLORWRITEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_TWEENFACTOR);
        D3DSTATE_TO_STR(WINED3D_RS_BLENDOP);
        D3DSTATE_TO_STR(WINED3D_RS_POSITIONDEGREE);
        D3DSTATE_TO_STR(WINED3D_RS_NORMALDEGREE);
        D3DSTATE_TO_STR(WINED3D_RS_SCISSORTESTENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_SLOPESCALEDEPTHBIAS);
        D3DSTATE_TO_STR(WINED3D_RS_ANTIALIASEDLINEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_MINTESSELLATIONLEVEL);
        D3DSTATE_TO_STR(WINED3D_RS_MAXTESSELLATIONLEVEL);
        D3DSTATE_TO_STR(WINED3D_RS_ADAPTIVETESS_X);
        D3DSTATE_TO_STR(WINED3D_RS_ADAPTIVETESS_Y);
        D3DSTATE_TO_STR(WINED3D_RS_ADAPTIVETESS_Z);
        D3DSTATE_TO_STR(WINED3D_RS_ADAPTIVETESS_W);
        D3DSTATE_TO_STR(WINED3D_RS_ENABLEADAPTIVETESSELLATION);
        D3DSTATE_TO_STR(WINED3D_RS_TWOSIDEDSTENCILMODE);
        D3DSTATE_TO_STR(WINED3D_RS_BACK_STENCILFAIL);
        D3DSTATE_TO_STR(WINED3D_RS_BACK_STENCILZFAIL);
        D3DSTATE_TO_STR(WINED3D_RS_BACK_STENCILPASS);
        D3DSTATE_TO_STR(WINED3D_RS_BACK_STENCILFUNC);
        D3DSTATE_TO_STR(WINED3D_RS_COLORWRITEENABLE1);
        D3DSTATE_TO_STR(WINED3D_RS_COLORWRITEENABLE2);
        D3DSTATE_TO_STR(WINED3D_RS_COLORWRITEENABLE3);
        D3DSTATE_TO_STR(WINED3D_RS_SRGBWRITEENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_DEPTHBIAS);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP8);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP9);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP10);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP11);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP12);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP13);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP14);
        D3DSTATE_TO_STR(WINED3D_RS_WRAP15);
        D3DSTATE_TO_STR(WINED3D_RS_SEPARATEALPHABLENDENABLE);
        D3DSTATE_TO_STR(WINED3D_RS_SRCBLENDALPHA);
        D3DSTATE_TO_STR(WINED3D_RS_DESTBLENDALPHA);
        D3DSTATE_TO_STR(WINED3D_RS_BLENDOPALPHA);
#undef D3DSTATE_TO_STR
        default:
            FIXME("Unrecognized %u render state!\n", state);
            return "unrecognized";
    }
}

const char *debug_d3dsamplerstate(enum wined3d_sampler_state state)
{
    switch (state)
    {
#define D3DSTATE_TO_STR(u) case u: return #u
        D3DSTATE_TO_STR(WINED3D_SAMP_BORDER_COLOR);
        D3DSTATE_TO_STR(WINED3D_SAMP_ADDRESS_U);
        D3DSTATE_TO_STR(WINED3D_SAMP_ADDRESS_V);
        D3DSTATE_TO_STR(WINED3D_SAMP_ADDRESS_W);
        D3DSTATE_TO_STR(WINED3D_SAMP_MAG_FILTER);
        D3DSTATE_TO_STR(WINED3D_SAMP_MIN_FILTER);
        D3DSTATE_TO_STR(WINED3D_SAMP_MIP_FILTER);
        D3DSTATE_TO_STR(WINED3D_SAMP_MIPMAP_LOD_BIAS);
        D3DSTATE_TO_STR(WINED3D_SAMP_MAX_MIP_LEVEL);
        D3DSTATE_TO_STR(WINED3D_SAMP_MAX_ANISOTROPY);
        D3DSTATE_TO_STR(WINED3D_SAMP_SRGB_TEXTURE);
        D3DSTATE_TO_STR(WINED3D_SAMP_ELEMENT_INDEX);
        D3DSTATE_TO_STR(WINED3D_SAMP_DMAP_OFFSET);
#undef D3DSTATE_TO_STR
        default:
            FIXME("Unrecognized %u sampler state!\n", state);
            return "unrecognized";
    }
}

const char *debug_d3dtexturefiltertype(enum wined3d_texture_filter_type filter_type)
{
    switch (filter_type)
    {
#define D3DTEXTUREFILTERTYPE_TO_STR(u) case u: return #u
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_NONE);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_POINT);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_LINEAR);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_ANISOTROPIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_FLAT_CUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_GAUSSIAN_CUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_PYRAMIDAL_QUAD);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3D_TEXF_GAUSSIAN_QUAD);
#undef D3DTEXTUREFILTERTYPE_TO_STR
        default:
            FIXME("Unrecognized texture filter type 0x%08x.\n", filter_type);
            return "unrecognized";
    }
}

const char *debug_d3dtexturestate(enum wined3d_texture_stage_state state)
{
    switch (state)
    {
#define D3DSTATE_TO_STR(u) case u: return #u
        D3DSTATE_TO_STR(WINED3D_TSS_COLOR_OP);
        D3DSTATE_TO_STR(WINED3D_TSS_COLOR_ARG1);
        D3DSTATE_TO_STR(WINED3D_TSS_COLOR_ARG2);
        D3DSTATE_TO_STR(WINED3D_TSS_ALPHA_OP);
        D3DSTATE_TO_STR(WINED3D_TSS_ALPHA_ARG1);
        D3DSTATE_TO_STR(WINED3D_TSS_ALPHA_ARG2);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_MAT00);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_MAT01);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_MAT10);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_MAT11);
        D3DSTATE_TO_STR(WINED3D_TSS_TEXCOORD_INDEX);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_LSCALE);
        D3DSTATE_TO_STR(WINED3D_TSS_BUMPENV_LOFFSET);
        D3DSTATE_TO_STR(WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS);
        D3DSTATE_TO_STR(WINED3D_TSS_COLOR_ARG0);
        D3DSTATE_TO_STR(WINED3D_TSS_ALPHA_ARG0);
        D3DSTATE_TO_STR(WINED3D_TSS_RESULT_ARG);
        D3DSTATE_TO_STR(WINED3D_TSS_CONSTANT);
#undef D3DSTATE_TO_STR
        default:
            FIXME("Unrecognized %u texture state!\n", state);
            return "unrecognized";
    }
}

const char *debug_d3dtop(enum wined3d_texture_op d3dtop)
{
    switch (d3dtop)
    {
#define D3DTOP_TO_STR(u) case u: return #u
        D3DTOP_TO_STR(WINED3D_TOP_DISABLE);
        D3DTOP_TO_STR(WINED3D_TOP_SELECT_ARG1);
        D3DTOP_TO_STR(WINED3D_TOP_SELECT_ARG2);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_2X);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_4X);
        D3DTOP_TO_STR(WINED3D_TOP_ADD);
        D3DTOP_TO_STR(WINED3D_TOP_ADD_SIGNED);
        D3DTOP_TO_STR(WINED3D_TOP_ADD_SIGNED_2X);
        D3DTOP_TO_STR(WINED3D_TOP_SUBTRACT);
        D3DTOP_TO_STR(WINED3D_TOP_ADD_SMOOTH);
        D3DTOP_TO_STR(WINED3D_TOP_BLEND_DIFFUSE_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_BLEND_TEXTURE_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_BLEND_FACTOR_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM);
        D3DTOP_TO_STR(WINED3D_TOP_BLEND_CURRENT_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_PREMODULATE);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR);
        D3DTOP_TO_STR(WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA);
        D3DTOP_TO_STR(WINED3D_TOP_BUMPENVMAP);
        D3DTOP_TO_STR(WINED3D_TOP_BUMPENVMAP_LUMINANCE);
        D3DTOP_TO_STR(WINED3D_TOP_DOTPRODUCT3);
        D3DTOP_TO_STR(WINED3D_TOP_MULTIPLY_ADD);
        D3DTOP_TO_STR(WINED3D_TOP_LERP);
#undef D3DTOP_TO_STR
        default:
            FIXME("Unrecognized texture op %#x.\n", d3dtop);
            return "unrecognized";
    }
}

const char *debug_d3dtstype(enum wined3d_transform_state tstype)
{
    switch (tstype)
    {
#define TSTYPE_TO_STR(tstype) case tstype: return #tstype
    TSTYPE_TO_STR(WINED3D_TS_VIEW);
    TSTYPE_TO_STR(WINED3D_TS_PROJECTION);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE0);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE1);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE2);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE3);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE4);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE5);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE6);
    TSTYPE_TO_STR(WINED3D_TS_TEXTURE7);
    TSTYPE_TO_STR(WINED3D_TS_WORLD_MATRIX(0));
    TSTYPE_TO_STR(WINED3D_TS_WORLD_MATRIX(1));
    TSTYPE_TO_STR(WINED3D_TS_WORLD_MATRIX(2));
    TSTYPE_TO_STR(WINED3D_TS_WORLD_MATRIX(3));
#undef TSTYPE_TO_STR
    default:
        if (tstype > 256 && tstype < 512)
        {
            FIXME("WINED3D_TS_WORLD_MATRIX(%u). 1..255 not currently supported.\n", tstype);
            return ("WINED3D_TS_WORLD_MATRIX > 0");
        }
        FIXME("Unrecognized transform state %#x.\n", tstype);
        return "unrecognized";
    }
}

const char *debug_shader_type(enum wined3d_shader_type type)
{
    switch(type)
    {
#define WINED3D_TO_STR(type) case type: return #type
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_PIXEL);
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_VERTEX);
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_GEOMETRY);
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_HULL);
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_DOMAIN);
        WINED3D_TO_STR(WINED3D_SHADER_TYPE_COMPUTE);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized shader type %#x.\n", type);
            return "unrecognized";
    }
}

const char *debug_d3dstate(DWORD state)
{
    if (STATE_IS_RENDER(state))
        return wine_dbg_sprintf("STATE_RENDER(%s)", debug_d3drenderstate(state - STATE_RENDER(0)));
    if (STATE_IS_TEXTURESTAGE(state))
    {
        DWORD texture_stage = (state - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
        DWORD texture_state = state - STATE_TEXTURESTAGE(texture_stage, 0);
        return wine_dbg_sprintf("STATE_TEXTURESTAGE(%#x, %s)",
                texture_stage, debug_d3dtexturestate(texture_state));
    }
    if (STATE_IS_SAMPLER(state))
        return wine_dbg_sprintf("STATE_SAMPLER(%#x)", state - STATE_SAMPLER(0));
    if (STATE_IS_COMPUTE_SHADER(state))
        return wine_dbg_sprintf("STATE_SHADER(%s)", debug_shader_type(WINED3D_SHADER_TYPE_COMPUTE));
    if (STATE_IS_GRAPHICS_SHADER(state))
        return wine_dbg_sprintf("STATE_SHADER(%s)", debug_shader_type(state - STATE_SHADER(0)));
    if (STATE_IS_COMPUTE_CONSTANT_BUFFER(state))
        return wine_dbg_sprintf("STATE_CONSTANT_BUFFER(%s)", debug_shader_type(WINED3D_SHADER_TYPE_COMPUTE));
    if (STATE_IS_GRAPHICS_CONSTANT_BUFFER(state))
        return wine_dbg_sprintf("STATE_CONSTANT_BUFFER(%s)", debug_shader_type(state - STATE_CONSTANT_BUFFER(0)));
    if (STATE_IS_COMPUTE_SHADER_RESOURCE_BINDING(state))
        return "STATE_COMPUTE_SHADER_RESOURCE_BINDING";
    if (STATE_IS_GRAPHICS_SHADER_RESOURCE_BINDING(state))
        return "STATE_GRAPHICS_SHADER_RESOURCE_BINDING";
    if (STATE_IS_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING(state))
        return "STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING";
    if (STATE_IS_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING(state))
        return "STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING";
    if (STATE_IS_TRANSFORM(state))
        return wine_dbg_sprintf("STATE_TRANSFORM(%s)", debug_d3dtstype(state - STATE_TRANSFORM(0)));
    if (STATE_IS_STREAMSRC(state))
        return "STATE_STREAMSRC";
    if (STATE_IS_INDEXBUFFER(state))
        return "STATE_INDEXBUFFER";
    if (STATE_IS_VDECL(state))
        return "STATE_VDECL";
    if (STATE_IS_VIEWPORT(state))
        return "STATE_VIEWPORT";
    if (STATE_IS_LIGHT_TYPE(state))
        return "STATE_LIGHT_TYPE";
    if (STATE_IS_ACTIVELIGHT(state))
        return wine_dbg_sprintf("STATE_ACTIVELIGHT(%#x)", state - STATE_ACTIVELIGHT(0));
    if (STATE_IS_SCISSORRECT(state))
        return "STATE_SCISSORRECT";
    if (STATE_IS_CLIPPLANE(state))
        return wine_dbg_sprintf("STATE_CLIPPLANE(%#x)", state - STATE_CLIPPLANE(0));
    if (STATE_IS_MATERIAL(state))
        return "STATE_MATERIAL";
    if (STATE_IS_RASTERIZER(state))
        return "STATE_RASTERIZER";
    if (STATE_IS_POINTSPRITECOORDORIGIN(state))
        return "STATE_POINTSPRITECOORDORIGIN";
    if (STATE_IS_BASEVERTEXINDEX(state))
        return "STATE_BASEVERTEXINDEX";
    if (STATE_IS_FRAMEBUFFER(state))
        return "STATE_FRAMEBUFFER";
    if (STATE_IS_POINT_ENABLE(state))
        return "STATE_POINT_ENABLE";
    if (STATE_IS_COLOR_KEY(state))
        return "STATE_COLOR_KEY";
    if (STATE_IS_STREAM_OUTPUT(state))
        return "STATE_STREAM_OUTPUT";
    if (STATE_IS_BLEND(state))
        return "STATE_BLEND";
    if (STATE_IS_BLEND_FACTOR(state))
        return "STATE_BLEND_FACTOR";

    return wine_dbg_sprintf("UNKNOWN_STATE(%#x)", state);
}

const char *debug_fboattachment(GLenum attachment)
{
    switch(attachment)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT0);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT1);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT2);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT3);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT4);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT5);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT6);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT7);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT8);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT9);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT10);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT11);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT12);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT13);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT14);
        WINED3D_TO_STR(GL_COLOR_ATTACHMENT15);
        WINED3D_TO_STR(GL_DEPTH_ATTACHMENT);
        WINED3D_TO_STR(GL_STENCIL_ATTACHMENT);
#undef WINED3D_TO_STR
        default:
            return wine_dbg_sprintf("Unknown FBO attachment %#x", attachment);
    }
}

const char *debug_fbostatus(GLenum status) {
    switch(status) {
#define FBOSTATUS_TO_STR(u) case u: return #u
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_COMPLETE);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_UNSUPPORTED);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_UNDEFINED);
#undef FBOSTATUS_TO_STR
        default:
            FIXME("Unrecognized FBO status 0x%08x.\n", status);
            return "unrecognized";
    }
}

const char *debug_glerror(GLenum error) {
    switch(error) {
#define GLERROR_TO_STR(u) case u: return #u
        GLERROR_TO_STR(GL_NO_ERROR);
        GLERROR_TO_STR(GL_INVALID_ENUM);
        GLERROR_TO_STR(GL_INVALID_VALUE);
        GLERROR_TO_STR(GL_INVALID_OPERATION);
        GLERROR_TO_STR(GL_STACK_OVERFLOW);
        GLERROR_TO_STR(GL_STACK_UNDERFLOW);
        GLERROR_TO_STR(GL_OUT_OF_MEMORY);
        GLERROR_TO_STR(GL_INVALID_FRAMEBUFFER_OPERATION);
#undef GLERROR_TO_STR
        default:
            FIXME("Unrecognized GL error 0x%08x.\n", error);
            return "unrecognized";
    }
}

const char *wined3d_debug_vkresult(VkResult vr)
{
    switch (vr)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
        WINED3D_TO_STR(VK_ERROR_NOT_PERMITTED_EXT);
        WINED3D_TO_STR(VK_ERROR_FRAGMENTATION_EXT);
        WINED3D_TO_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        WINED3D_TO_STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        WINED3D_TO_STR(VK_ERROR_INVALID_SHADER_NV);
        WINED3D_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
        WINED3D_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        WINED3D_TO_STR(VK_ERROR_SURFACE_LOST_KHR);
        WINED3D_TO_STR(VK_ERROR_FRAGMENTED_POOL);
        WINED3D_TO_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        WINED3D_TO_STR(VK_ERROR_TOO_MANY_OBJECTS);
        WINED3D_TO_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        WINED3D_TO_STR(VK_ERROR_FEATURE_NOT_PRESENT);
        WINED3D_TO_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        WINED3D_TO_STR(VK_ERROR_LAYER_NOT_PRESENT);
        WINED3D_TO_STR(VK_ERROR_MEMORY_MAP_FAILED);
        WINED3D_TO_STR(VK_ERROR_DEVICE_LOST);
        WINED3D_TO_STR(VK_ERROR_INITIALIZATION_FAILED);
        WINED3D_TO_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        WINED3D_TO_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        WINED3D_TO_STR(VK_SUCCESS);
        WINED3D_TO_STR(VK_NOT_READY);
        WINED3D_TO_STR(VK_TIMEOUT);
        WINED3D_TO_STR(VK_EVENT_SET);
        WINED3D_TO_STR(VK_EVENT_RESET);
        WINED3D_TO_STR(VK_INCOMPLETE);
        WINED3D_TO_STR(VK_SUBOPTIMAL_KHR);
#undef WINED3D_TO_STR
        default:
            return wine_dbg_sprintf("unrecognised(%d)", vr);
    }
}

static const char *debug_fixup_channel_source(enum fixup_channel_source source)
{
    switch(source)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(CHANNEL_SOURCE_ZERO);
        WINED3D_TO_STR(CHANNEL_SOURCE_ONE);
        WINED3D_TO_STR(CHANNEL_SOURCE_X);
        WINED3D_TO_STR(CHANNEL_SOURCE_Y);
        WINED3D_TO_STR(CHANNEL_SOURCE_Z);
        WINED3D_TO_STR(CHANNEL_SOURCE_W);
        WINED3D_TO_STR(CHANNEL_SOURCE_COMPLEX0);
        WINED3D_TO_STR(CHANNEL_SOURCE_COMPLEX1);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized fixup_channel_source %#x\n", source);
            return "unrecognized";
    }
}

static const char *debug_complex_fixup(enum complex_fixup fixup)
{
    switch(fixup)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(COMPLEX_FIXUP_YUY2);
        WINED3D_TO_STR(COMPLEX_FIXUP_UYVY);
        WINED3D_TO_STR(COMPLEX_FIXUP_YV12);
        WINED3D_TO_STR(COMPLEX_FIXUP_NV12);
        WINED3D_TO_STR(COMPLEX_FIXUP_P8);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized complex fixup %#x\n", fixup);
            return "unrecognized";
    }
}

void dump_color_fixup_desc(struct color_fixup_desc fixup)
{
    if (is_complex_fixup(fixup))
    {
        TRACE("\tComplex: %s\n", debug_complex_fixup(get_complex_fixup(fixup)));
        return;
    }

    TRACE("\tX: %s%s\n", debug_fixup_channel_source(fixup.x_source), fixup.x_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tY: %s%s\n", debug_fixup_channel_source(fixup.y_source), fixup.y_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tZ: %s%s\n", debug_fixup_channel_source(fixup.z_source), fixup.z_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tW: %s%s\n", debug_fixup_channel_source(fixup.w_source), fixup.w_sign_fixup ? ", SIGN_FIXUP" : "");
}

BOOL is_invalid_op(const struct wined3d_state *state, int stage,
        enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3)
{
    if (op == WINED3D_TOP_DISABLE)
        return FALSE;
    if (state->textures[stage])
        return FALSE;

    if ((arg1 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3D_TOP_SELECT_ARG2)
        return TRUE;
    if ((arg2 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3D_TOP_SELECT_ARG1)
        return TRUE;
    if ((arg3 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && (op == WINED3D_TOP_MULTIPLY_ADD || op == WINED3D_TOP_LERP))
        return TRUE;

    return FALSE;
}

void get_identity_matrix(struct wined3d_matrix *mat)
{
    static const struct wined3d_matrix identity =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    *mat = identity;
}

void get_modelview_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int index, struct wined3d_matrix *mat)
{
    if (context->last_was_rhw)
        get_identity_matrix(mat);
    else
        multiply_matrix(mat, &state->transforms[WINED3D_TS_VIEW], &state->transforms[WINED3D_TS_WORLD_MATRIX(index)]);
}

void get_projection_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        struct wined3d_matrix *mat)
{
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    BOOL clip_control, flip;
    float center_offset;

    /* There are a couple of additional things we have to take into account
     * here besides the projection transformation itself:
     *   - We need to flip along the y-axis in case of offscreen rendering.
     *   - OpenGL Z range is {-Wc,...,Wc} while D3D Z range is {0,...,Wc}.
     *   - <= D3D9 coordinates refer to pixel centers while GL coordinates
     *     refer to pixel corners.
     *   - D3D has a top-left filling convention. We need to maintain this
     *     even after the y-flip mentioned above.
     * In order to handle the last two points, we translate by
     * (63.0 / 128.0) / VPw and (63.0 / 128.0) / VPh. This is equivalent to
     * translating slightly less than half a pixel. We want the difference to
     * be large enough that it doesn't get lost due to rounding inside the
     * driver, but small enough to prevent it from interfering with any
     * anti-aliasing. */

    clip_control = d3d_info->clip_control;
    flip = !clip_control && context->render_offscreen;
    if (!clip_control && d3d_info->wined3d_creation_flags & WINED3D_PIXEL_CENTER_INTEGER)
        center_offset = 63.0f / 64.0f;
    else
        center_offset = -1.0f / 64.0f;

    if (context->last_was_rhw)
    {
        /* Transform D3D RHW coordinates to OpenGL clip coordinates. */
        float x = state->viewports[0].x;
        float y = state->viewports[0].y;
        float w = state->viewports[0].width;
        float h = state->viewports[0].height;
        float x_scale = 2.0f / w;
        float x_offset = (center_offset - (2.0f * x) - w) / w;
        float y_scale = flip ? 2.0f / h : 2.0f / -h;
        float y_offset = flip
                ? (center_offset - (2.0f * y) - h) / h
                : (center_offset - (2.0f * y) - h) / -h;
        enum wined3d_depth_buffer_type zenable = state->fb->depth_stencil ?
                state->render_states[WINED3D_RS_ZENABLE] : WINED3D_ZB_FALSE;
        float z_scale = zenable ? clip_control ? 1.0f : 2.0f : 0.0f;
        float z_offset = zenable ? clip_control ? 0.0f : -1.0f : 0.0f;
        const struct wined3d_matrix projection =
        {
             x_scale,     0.0f,      0.0f, 0.0f,
                0.0f,  y_scale,      0.0f, 0.0f,
                0.0f,     0.0f,   z_scale, 0.0f,
            x_offset, y_offset,  z_offset, 1.0f,
        };

        *mat = projection;
    }
    else
    {
        float y_scale = flip ? -1.0f : 1.0f;
        float x_offset = center_offset / state->viewports[0].width;
        float y_offset = flip
                ? center_offset / state->viewports[0].height
                : -center_offset / state->viewports[0].height;
        float z_scale = clip_control ? 1.0f : 2.0f;
        float z_offset = clip_control ? 0.0f : -1.0f;
        const struct wined3d_matrix projection =
        {
                1.0f,     0.0f,     0.0f, 0.0f,
                0.0f,  y_scale,     0.0f, 0.0f,
                0.0f,     0.0f,  z_scale, 0.0f,
            x_offset, y_offset, z_offset, 1.0f,
        };

        multiply_matrix(mat, &projection, &state->transforms[WINED3D_TS_PROJECTION]);
    }
}

/* Setup this textures matrix according to the texture flags. */
static void compute_texture_matrix(const struct wined3d_matrix *matrix, uint32_t flags, BOOL calculated_coords,
        BOOL transformed, enum wined3d_format_id format_id, BOOL ffp_proj_control, struct wined3d_matrix *out_matrix)
{
    struct wined3d_matrix mat;

    if (flags == WINED3D_TTFF_DISABLE || flags == WINED3D_TTFF_COUNT1 || transformed)
    {
        get_identity_matrix(out_matrix);
        return;
    }

    if (flags == (WINED3D_TTFF_COUNT1 | WINED3D_TTFF_PROJECTED))
    {
        ERR("Invalid texture transform flags: WINED3D_TTFF_COUNT1 | WINED3D_TTFF_PROJECTED.\n");
        return;
    }

    mat = *matrix;

    if (flags & WINED3D_TTFF_PROJECTED)
    {
        if (!ffp_proj_control)
        {
            switch (flags & ~WINED3D_TTFF_PROJECTED)
            {
                case WINED3D_TTFF_COUNT2:
                    mat._14 = mat._12;
                    mat._24 = mat._22;
                    mat._34 = mat._32;
                    mat._44 = mat._42;
                    mat._12 = mat._22 = mat._32 = mat._42 = 0.0f;
                    break;
                case WINED3D_TTFF_COUNT3:
                    mat._14 = mat._13;
                    mat._24 = mat._23;
                    mat._34 = mat._33;
                    mat._44 = mat._43;
                    mat._13 = mat._23 = mat._33 = mat._43 = 0.0f;
                    break;
            }
        }
    }
    else
    {
        /* Under Direct3D the R/Z coord can be used for translation, under
         * OpenGL we use the Q coord instead. */
        if (!calculated_coords)
        {
            switch (format_id)
            {
                /* Direct3D passes the default 1.0 in the 2nd coord, while GL
                 * passes it in the 4th. Swap 2nd and 4th coord. No need to
                 * store the value of mat._41 in mat._21 because the input
                 * value to the transformation will be 0, so the matrix value
                 * is irrelevant. */
                case WINED3DFMT_R32_FLOAT:
                    mat._41 = mat._21;
                    mat._42 = mat._22;
                    mat._43 = mat._23;
                    mat._44 = mat._24;
                    break;
                /* See above, just 3rd and 4th coord. */
                case WINED3DFMT_R32G32_FLOAT:
                    mat._41 = mat._31;
                    mat._42 = mat._32;
                    mat._43 = mat._33;
                    mat._44 = mat._34;
                    break;
                case WINED3DFMT_R32G32B32_FLOAT: /* Opengl defaults match dx defaults */
                case WINED3DFMT_R32G32B32A32_FLOAT: /* No defaults apply, all app defined */

                /* This is to prevent swapping the matrix lines and put the default 4th coord = 1.0
                 * into a bad place. The division elimination below will apply to make sure the
                 * 1.0 doesn't do anything bad. The caller will set this value if the stride is 0
                 */
                case WINED3DFMT_UNKNOWN: /* No texture coords, 0/0/0/1 defaults are passed */
                    break;
                default:
                    FIXME("Unexpected fixed function texture coord input\n");
            }
        }
        if (!ffp_proj_control)
        {
            switch (flags & ~WINED3D_TTFF_PROJECTED)
            {
                /* case WINED3D_TTFF_COUNT1: Won't ever get here. */
                case WINED3D_TTFF_COUNT2:
                    mat._13 = mat._23 = mat._33 = mat._43 = 0.0f;
                /* OpenGL divides the first 3 vertex coordinates by the 4th by
                 * default, which is essentially the same as D3DTTFF_PROJECTED.
                 * Make sure that the 4th coordinate evaluates to 1.0 to
                 * eliminate that.
                 *
                 * If the fixed function pipeline is used, the 4th value
                 * remains unused, so there is no danger in doing this. With
                 * vertex shaders we have a problem. Should an application hit
                 * that problem, the code here would have to check for pixel
                 * shaders, and the shader has to undo the default GL divide.
                 *
                 * A more serious problem occurs if the application passes 4
                 * coordinates in, and the 4th is != 1.0 (OpenGL default).
                 * This would have to be fixed with immediate mode draws. */
                default:
                    mat._14 = mat._24 = mat._34 = 0.0f; mat._44 = 1.0f;
            }
        }
    }

    *out_matrix = mat;
}

void get_texture_matrix(const struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int tex, struct wined3d_matrix *mat)
{
    const struct wined3d_device *device = context->device;
    BOOL generated = (state->texture_states[tex][WINED3D_TSS_TEXCOORD_INDEX] & 0xffff0000)
            != WINED3DTSS_TCI_PASSTHRU;
    unsigned int coord_idx = min(state->texture_states[tex][WINED3D_TSS_TEXCOORD_INDEX & 0x0000ffff],
            WINED3D_MAX_TEXTURES - 1);

    compute_texture_matrix(&state->transforms[WINED3D_TS_TEXTURE0 + tex],
            state->texture_states[tex][WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS],
            generated, context->last_was_rhw,
            context->stream_info.use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx))
            ? context->stream_info.elements[WINED3D_FFP_TEXCOORD0 + coord_idx].format->id
            : WINED3DFMT_UNKNOWN,
            device->shader_backend->shader_has_ffp_proj_control(device->shader_priv), mat);

    if ((context->lastWasPow2Texture & (1u << tex)) && state->textures[tex])
    {
        if (generated)
            FIXME("Non-power-of-two texture being used with generated texture coords.\n");
        /* NP2 texcoord fixup is implemented for pixelshaders so only enable the
         * fixed-function-pipeline fixup via pow2Matrix when no PS is used. */
        if (!use_ps(state))
        {
            TRACE("Non-power-of-two texture matrix multiply fixup.\n");
            multiply_matrix(mat, mat, (struct wined3d_matrix *)state->textures[tex]->pow2_matrix);
        }
    }
}

void get_pointsize_minmax(const struct wined3d_context *context, const struct wined3d_state *state,
        float *out_min, float *out_max)
{
    union
    {
        DWORD d;
        float f;
    } min, max;

    min.d = state->render_states[WINED3D_RS_POINTSIZE_MIN];
    max.d = state->render_states[WINED3D_RS_POINTSIZE_MAX];

    if (min.f > max.f)
        min.f = max.f;

    *out_min = min.f;
    *out_max = max.f;
}

void get_pointsize(const struct wined3d_context *context, const struct wined3d_state *state,
        float *out_pointsize, float *out_att)
{
    /* POINTSCALEENABLE controls how point size value is treated. If set to
     * true, the point size is scaled with respect to height of viewport.
     * When set to false point size is in pixels. */
    union
    {
        DWORD d;
        float f;
    } pointsize, a, b, c;

    out_att[0] = 1.0f;
    out_att[1] = 0.0f;
    out_att[2] = 0.0f;

    pointsize.d = state->render_states[WINED3D_RS_POINTSIZE];
    a.d = state->render_states[WINED3D_RS_POINTSCALE_A];
    b.d = state->render_states[WINED3D_RS_POINTSCALE_B];
    c.d = state->render_states[WINED3D_RS_POINTSCALE_C];

    /* Always use first viewport, this path does not apply to d3d10/11 multiple viewports case. */
    if (state->render_states[WINED3D_RS_POINTSCALEENABLE])
    {
        float scale_factor = state->viewports[0].height * state->viewports[0].height;

        out_att[0] = a.f / scale_factor;
        out_att[1] = b.f / scale_factor;
        out_att[2] = c.f / scale_factor;
    }
    *out_pointsize = pointsize.f;
}

void get_fog_start_end(const struct wined3d_context *context, const struct wined3d_state *state,
        float *start, float *end)
{
    union
    {
        DWORD d;
        float f;
    } tmpvalue;

    switch (context->fog_source)
    {
        case FOGSOURCE_VS:
            *start = 1.0f;
            *end = 0.0f;
            break;

        case FOGSOURCE_COORD:
            *start = 255.0f;
            *end = 0.0f;
            break;

        case FOGSOURCE_FFP:
            tmpvalue.d = state->render_states[WINED3D_RS_FOGSTART];
            *start = tmpvalue.f;
            tmpvalue.d = state->render_states[WINED3D_RS_FOGEND];
            *end = tmpvalue.f;
            /* Special handling for fog_start == fog_end. In d3d with vertex
             * fog, everything is fogged. With table fog, everything with
             * fog_coord < fog_start is unfogged, and fog_coord > fog_start
             * is fogged. Windows drivers disagree when fog_coord == fog_start. */
            if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE && *start == *end)
            {
                *start = -INFINITY;
                *end = 0.0f;
            }
            break;

        default:
            /* This should not happen, context->fog_source is set in wined3d, not the app. */
            ERR("Unexpected fog coordinate source.\n");
            *start = 0.0f;
            *end = 0.0f;
    }
}

/* Note: It's the caller's responsibility to ensure values can be expressed
 * in the requested format. UNORM formats for example can only express values
 * in the range 0.0f -> 1.0f. */
DWORD wined3d_format_convert_from_float(const struct wined3d_format *format, const struct wined3d_color *color)
{
    static const struct
    {
        enum wined3d_format_id format_id;
        struct wined3d_vec4 mul;
        struct wined3d_uvec4 shift;
    }
    float_conv[] =
    {
        {WINED3DFMT_B8G8R8A8_UNORM,    {       255.0f,  255.0f,  255.0f, 255.0f}, {16,  8,  0, 24}},
        {WINED3DFMT_B8G8R8X8_UNORM,    {       255.0f,  255.0f,  255.0f, 255.0f}, {16,  8,  0, 24}},
        {WINED3DFMT_B8G8R8_UNORM,      {       255.0f,  255.0f,  255.0f, 255.0f}, {16,  8,  0, 24}},
        {WINED3DFMT_B5G6R5_UNORM,      {        31.0f,   63.0f,   31.0f,   0.0f}, {11,  5,  0,  0}},
        {WINED3DFMT_B5G5R5A1_UNORM,    {        31.0f,   31.0f,   31.0f,   1.0f}, {10,  5,  0, 15}},
        {WINED3DFMT_B5G5R5X1_UNORM,    {        31.0f,   31.0f,   31.0f,   1.0f}, {10,  5,  0, 15}},
        {WINED3DFMT_R8_UNORM,          {       255.0f,    0.0f,    0.0f,   0.0f}, { 0,  0,  0,  0}},
        {WINED3DFMT_A8_UNORM,          {         0.0f,    0.0f,    0.0f, 255.0f}, { 0,  0,  0,  0}},
        {WINED3DFMT_B4G4R4A4_UNORM,    {        15.0f,   15.0f,   15.0f,  15.0f}, { 8,  4,  0, 12}},
        {WINED3DFMT_B4G4R4X4_UNORM,    {        15.0f,   15.0f,   15.0f,  15.0f}, { 8,  4,  0, 12}},
        {WINED3DFMT_B2G3R3_UNORM,      {         7.0f,    7.0f,    3.0f,   0.0f}, { 5,  2,  0,  0}},
        {WINED3DFMT_R8G8B8A8_UNORM,    {       255.0f,  255.0f,  255.0f, 255.0f}, { 0,  8, 16, 24}},
        {WINED3DFMT_R8G8B8X8_UNORM,    {       255.0f,  255.0f,  255.0f, 255.0f}, { 0,  8, 16, 24}},
        {WINED3DFMT_B10G10R10A2_UNORM, {      1023.0f, 1023.0f, 1023.0f,   3.0f}, {20, 10,  0, 30}},
        {WINED3DFMT_R10G10B10A2_UNORM, {      1023.0f, 1023.0f, 1023.0f,   3.0f}, { 0, 10, 20, 30}},
        {WINED3DFMT_P8_UINT,           {         0.0f,    0.0f,    0.0f, 255.0f}, { 0,  0,  0,  0}},
        {WINED3DFMT_S1_UINT_D15_UNORM, {     32767.0f,    0.0f,    0.0f,   0.0f}, { 0,  0,  0,  0}},
        {WINED3DFMT_D16_UNORM,         {     65535.0f,    0.0f,    0.0f,   0.0f}, { 0,  0,  0,  0}},
    };
    static const struct
    {
        enum wined3d_format_id format_id;
        struct wined3d_dvec4 mul;
        struct wined3d_uvec4 shift;
    }
    double_conv[] =
    {
        {WINED3DFMT_D24_UNORM_S8_UINT, {  16777215.0, 1.0, 0.0, 0.0}, {8, 0, 0, 0}},
        {WINED3DFMT_X8D24_UNORM,       {  16777215.0, 0.0, 0.0, 0.0}, {0, 0, 0, 0}},
        {WINED3DFMT_D32_UNORM,         {4294967295.0, 0.0, 0.0, 0.0}, {0, 0, 0, 0}},
    };
    enum wined3d_format_id format_id = format->id;
    struct wined3d_color colour_srgb;
    unsigned int i;
    DWORD ret;

    TRACE("Converting colour %s to format %s.\n", debug_color(color), debug_d3dformat(format_id));

    for (i = 0; i < ARRAY_SIZE(format_srgb_info); ++i)
    {
        if (format_id != format_srgb_info[i].srgb_format_id)
            continue;

        wined3d_colour_srgb_from_linear(&colour_srgb, color);
        format_id = format_srgb_info[i].base_format_id;
        color = &colour_srgb;
        break;
    }

    for (i = 0; i < ARRAY_SIZE(float_conv); ++i)
    {
        if (format_id != float_conv[i].format_id)
            continue;

        ret = ((DWORD)((color->r * float_conv[i].mul.x) + 0.5f)) << float_conv[i].shift.x;
        ret |= ((DWORD)((color->g * float_conv[i].mul.y) + 0.5f)) << float_conv[i].shift.y;
        ret |= ((DWORD)((color->b * float_conv[i].mul.z) + 0.5f)) << float_conv[i].shift.z;
        ret |= ((DWORD)((color->a * float_conv[i].mul.w) + 0.5f)) << float_conv[i].shift.w;

        TRACE("Returning 0x%08x.\n", ret);

        return ret;
    }

    for (i = 0; i < ARRAY_SIZE(double_conv); ++i)
    {
        if (format_id != double_conv[i].format_id)
            continue;

        ret = ((DWORD)((color->r * double_conv[i].mul.x) + 0.5)) << double_conv[i].shift.x;
        ret |= ((DWORD)((color->g * double_conv[i].mul.y) + 0.5)) << double_conv[i].shift.y;
        ret |= ((DWORD)((color->b * double_conv[i].mul.z) + 0.5)) << double_conv[i].shift.z;
        ret |= ((DWORD)((color->a * double_conv[i].mul.w) + 0.5)) << double_conv[i].shift.w;

        TRACE("Returning 0x%08x.\n", ret);

        return ret;
    }

    FIXME("Conversion for format %s not implemented.\n", debug_d3dformat(format_id));

    return 0;
}

static float color_to_float(DWORD color, DWORD size, DWORD offset)
{
    DWORD mask = size < 32 ? (1u << size) - 1 : ~0u;

    if (!size)
        return 1.0f;

    color >>= offset;
    color &= mask;

    return (float)color / (float)mask;
}

void wined3d_format_get_float_color_key(const struct wined3d_format *format,
        const struct wined3d_color_key *key, struct wined3d_color *float_colors)
{
    struct wined3d_color slop;

    switch (format->id)
    {
        case WINED3DFMT_B8G8R8_UNORM:
        case WINED3DFMT_B8G8R8A8_UNORM:
        case WINED3DFMT_B8G8R8X8_UNORM:
        case WINED3DFMT_B5G6R5_UNORM:
        case WINED3DFMT_B5G5R5X1_UNORM:
        case WINED3DFMT_B5G5R5A1_UNORM:
        case WINED3DFMT_B4G4R4A4_UNORM:
        case WINED3DFMT_B2G3R3_UNORM:
        case WINED3DFMT_R8_UNORM:
        case WINED3DFMT_A8_UNORM:
        case WINED3DFMT_B2G3R3A8_UNORM:
        case WINED3DFMT_B4G4R4X4_UNORM:
        case WINED3DFMT_R10G10B10A2_UNORM:
        case WINED3DFMT_R10G10B10A2_SNORM:
        case WINED3DFMT_R8G8B8A8_UNORM:
        case WINED3DFMT_R8G8B8X8_UNORM:
        case WINED3DFMT_R16G16_UNORM:
        case WINED3DFMT_B10G10R10A2_UNORM:
            slop.r = 0.5f / ((1u << format->red_size) - 1);
            slop.g = 0.5f / ((1u << format->green_size) - 1);
            slop.b = 0.5f / ((1u << format->blue_size) - 1);
            slop.a = 0.5f / ((1u << format->alpha_size) - 1);

            float_colors[0].r = color_to_float(key->color_space_low_value, format->red_size, format->red_offset)
                    - slop.r;
            float_colors[0].g = color_to_float(key->color_space_low_value, format->green_size, format->green_offset)
                    - slop.g;
            float_colors[0].b = color_to_float(key->color_space_low_value, format->blue_size, format->blue_offset)
                    - slop.b;
            float_colors[0].a = color_to_float(key->color_space_low_value, format->alpha_size, format->alpha_offset)
                    - slop.a;

            float_colors[1].r = color_to_float(key->color_space_high_value, format->red_size, format->red_offset)
                    + slop.r;
            float_colors[1].g = color_to_float(key->color_space_high_value, format->green_size, format->green_offset)
                    + slop.g;
            float_colors[1].b = color_to_float(key->color_space_high_value, format->blue_size, format->blue_offset)
                    + slop.b;
            float_colors[1].a = color_to_float(key->color_space_high_value, format->alpha_size, format->alpha_offset)
                    + slop.a;
            break;

        case WINED3DFMT_P8_UINT:
            float_colors[0].r = 0.0f;
            float_colors[0].g = 0.0f;
            float_colors[0].b = 0.0f;
            float_colors[0].a = (key->color_space_low_value - 0.5f) / 255.0f;

            float_colors[1].r = 0.0f;
            float_colors[1].g = 0.0f;
            float_colors[1].b = 0.0f;
            float_colors[1].a = (key->color_space_high_value + 0.5f) / 255.0f;
            break;

        default:
            ERR("Unhandled color key to float conversion for format %s.\n", debug_d3dformat(format->id));
    }
}

enum wined3d_format_id pixelformat_for_depth(DWORD depth)
{
    switch (depth)
    {
        case 8:  return WINED3DFMT_P8_UINT;
        case 15: return WINED3DFMT_B5G5R5X1_UNORM;
        case 16: return WINED3DFMT_B5G6R5_UNORM;
        case 24: return WINED3DFMT_B8G8R8X8_UNORM; /* Robots needs 24bit to be WINED3DFMT_B8G8R8X8_UNORM */
        case 32: return WINED3DFMT_B8G8R8X8_UNORM; /* EVE online and the Fur demo need 32bit AdapterDisplayMode to return WINED3DFMT_B8G8R8X8_UNORM */
        default: return WINED3DFMT_UNKNOWN;
    }
}

void multiply_matrix(struct wined3d_matrix *dst, const struct wined3d_matrix *src1, const struct wined3d_matrix *src2)
{
    struct wined3d_matrix tmp;

    /* Now do the multiplication 'by hand'.
       I know that all this could be optimised, but this will be done later :-) */
    tmp._11 = (src1->_11 * src2->_11) + (src1->_21 * src2->_12) + (src1->_31 * src2->_13) + (src1->_41 * src2->_14);
    tmp._21 = (src1->_11 * src2->_21) + (src1->_21 * src2->_22) + (src1->_31 * src2->_23) + (src1->_41 * src2->_24);
    tmp._31 = (src1->_11 * src2->_31) + (src1->_21 * src2->_32) + (src1->_31 * src2->_33) + (src1->_41 * src2->_34);
    tmp._41 = (src1->_11 * src2->_41) + (src1->_21 * src2->_42) + (src1->_31 * src2->_43) + (src1->_41 * src2->_44);

    tmp._12 = (src1->_12 * src2->_11) + (src1->_22 * src2->_12) + (src1->_32 * src2->_13) + (src1->_42 * src2->_14);
    tmp._22 = (src1->_12 * src2->_21) + (src1->_22 * src2->_22) + (src1->_32 * src2->_23) + (src1->_42 * src2->_24);
    tmp._32 = (src1->_12 * src2->_31) + (src1->_22 * src2->_32) + (src1->_32 * src2->_33) + (src1->_42 * src2->_34);
    tmp._42 = (src1->_12 * src2->_41) + (src1->_22 * src2->_42) + (src1->_32 * src2->_43) + (src1->_42 * src2->_44);

    tmp._13 = (src1->_13 * src2->_11) + (src1->_23 * src2->_12) + (src1->_33 * src2->_13) + (src1->_43 * src2->_14);
    tmp._23 = (src1->_13 * src2->_21) + (src1->_23 * src2->_22) + (src1->_33 * src2->_23) + (src1->_43 * src2->_24);
    tmp._33 = (src1->_13 * src2->_31) + (src1->_23 * src2->_32) + (src1->_33 * src2->_33) + (src1->_43 * src2->_34);
    tmp._43 = (src1->_13 * src2->_41) + (src1->_23 * src2->_42) + (src1->_33 * src2->_43) + (src1->_43 * src2->_44);

    tmp._14 = (src1->_14 * src2->_11) + (src1->_24 * src2->_12) + (src1->_34 * src2->_13) + (src1->_44 * src2->_14);
    tmp._24 = (src1->_14 * src2->_21) + (src1->_24 * src2->_22) + (src1->_34 * src2->_23) + (src1->_44 * src2->_24);
    tmp._34 = (src1->_14 * src2->_31) + (src1->_24 * src2->_32) + (src1->_34 * src2->_33) + (src1->_44 * src2->_34);
    tmp._44 = (src1->_14 * src2->_41) + (src1->_24 * src2->_42) + (src1->_34 * src2->_43) + (src1->_44 * src2->_44);

    *dst = tmp;
}

void gen_ffp_frag_op(const struct wined3d_context *context, const struct wined3d_state *state,
        struct ffp_frag_settings *settings, BOOL ignore_textype)
{
#define ARG1 0x01
#define ARG2 0x02
#define ARG0 0x04
    static const unsigned char args[WINED3D_TOP_LERP + 1] =
    {
        /* undefined                        */  0,
        /* D3DTOP_DISABLE                   */  0,
        /* D3DTOP_SELECTARG1                */  ARG1,
        /* D3DTOP_SELECTARG2                */  ARG2,
        /* D3DTOP_MODULATE                  */  ARG1 | ARG2,
        /* D3DTOP_MODULATE2X                */  ARG1 | ARG2,
        /* D3DTOP_MODULATE4X                */  ARG1 | ARG2,
        /* D3DTOP_ADD                       */  ARG1 | ARG2,
        /* D3DTOP_ADDSIGNED                 */  ARG1 | ARG2,
        /* D3DTOP_ADDSIGNED2X               */  ARG1 | ARG2,
        /* D3DTOP_SUBTRACT                  */  ARG1 | ARG2,
        /* D3DTOP_ADDSMOOTH                 */  ARG1 | ARG2,
        /* D3DTOP_BLENDDIFFUSEALPHA         */  ARG1 | ARG2,
        /* D3DTOP_BLENDTEXTUREALPHA         */  ARG1 | ARG2,
        /* D3DTOP_BLENDFACTORALPHA          */  ARG1 | ARG2,
        /* D3DTOP_BLENDTEXTUREALPHAPM       */  ARG1 | ARG2,
        /* D3DTOP_BLENDCURRENTALPHA         */  ARG1 | ARG2,
        /* D3DTOP_PREMODULATE               */  ARG1 | ARG2,
        /* D3DTOP_MODULATEALPHA_ADDCOLOR    */  ARG1 | ARG2,
        /* D3DTOP_MODULATECOLOR_ADDALPHA    */  ARG1 | ARG2,
        /* D3DTOP_MODULATEINVALPHA_ADDCOLOR */  ARG1 | ARG2,
        /* D3DTOP_MODULATEINVCOLOR_ADDALPHA */  ARG1 | ARG2,
        /* D3DTOP_BUMPENVMAP                */  ARG1 | ARG2,
        /* D3DTOP_BUMPENVMAPLUMINANCE       */  ARG1 | ARG2,
        /* D3DTOP_DOTPRODUCT3               */  ARG1 | ARG2,
        /* D3DTOP_MULTIPLYADD               */  ARG1 | ARG2 | ARG0,
        /* D3DTOP_LERP                      */  ARG1 | ARG2 | ARG0
    };
    unsigned int i;
    DWORD ttff;
    DWORD cop, aop, carg0, carg1, carg2, aarg0, aarg1, aarg2;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;

    settings->padding = 0;

    for (i = 0; i < d3d_info->limits.ffp_blend_stages; ++i)
    {
        struct wined3d_texture *texture;

        settings->op[i].padding = 0;
        if (state->texture_states[i][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_DISABLE)
        {
            settings->op[i].cop = WINED3D_TOP_DISABLE;
            settings->op[i].aop = WINED3D_TOP_DISABLE;
            settings->op[i].carg0 = settings->op[i].carg1 = settings->op[i].carg2 = ARG_UNUSED;
            settings->op[i].aarg0 = settings->op[i].aarg1 = settings->op[i].aarg2 = ARG_UNUSED;
            settings->op[i].color_fixup = COLOR_FIXUP_IDENTITY;
            settings->op[i].tmp_dst = 0;
            settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_1D;
            settings->op[i].projected = WINED3D_PROJECTION_NONE;
            i++;
            break;
        }

        if ((texture = state->textures[i]))
        {
            if (can_use_texture_swizzle(d3d_info, texture->resource.format))
                settings->op[i].color_fixup = COLOR_FIXUP_IDENTITY;
            else
                settings->op[i].color_fixup = texture->resource.format->color_fixup;
            if (ignore_textype)
            {
                settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_1D;
            }
            else
            {
                switch (wined3d_texture_gl(texture)->target)
                {
                    case GL_TEXTURE_1D:
                        settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_1D;
                        break;
                    case GL_TEXTURE_2D:
                        settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_2D;
                        break;
                    case GL_TEXTURE_3D:
                        settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_3D;
                        break;
                    case GL_TEXTURE_CUBE_MAP_ARB:
                        settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
                        break;
                    case GL_TEXTURE_RECTANGLE_ARB:
                        settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_RECT;
                        break;
                }
            }
        } else {
            settings->op[i].color_fixup = COLOR_FIXUP_IDENTITY;
            settings->op[i].tex_type = WINED3D_GL_RES_TYPE_TEX_1D;
        }

        cop = state->texture_states[i][WINED3D_TSS_COLOR_OP];
        aop = state->texture_states[i][WINED3D_TSS_ALPHA_OP];

        carg1 = (args[cop] & ARG1) ? state->texture_states[i][WINED3D_TSS_COLOR_ARG1] : ARG_UNUSED;
        carg2 = (args[cop] & ARG2) ? state->texture_states[i][WINED3D_TSS_COLOR_ARG2] : ARG_UNUSED;
        carg0 = (args[cop] & ARG0) ? state->texture_states[i][WINED3D_TSS_COLOR_ARG0] : ARG_UNUSED;

        if (is_invalid_op(state, i, cop, carg1, carg2, carg0))
        {
            carg0 = ARG_UNUSED;
            carg2 = ARG_UNUSED;
            carg1 = WINED3DTA_CURRENT;
            cop = WINED3D_TOP_SELECT_ARG1;
        }

        if (cop == WINED3D_TOP_DOTPRODUCT3)
        {
            /* A dotproduct3 on the colorop overwrites the alphaop operation and replicates
             * the color result to the alpha component of the destination
             */
            aop = cop;
            aarg1 = carg1;
            aarg2 = carg2;
            aarg0 = carg0;
        }
        else
        {
            aarg1 = (args[aop] & ARG1) ? state->texture_states[i][WINED3D_TSS_ALPHA_ARG1] : ARG_UNUSED;
            aarg2 = (args[aop] & ARG2) ? state->texture_states[i][WINED3D_TSS_ALPHA_ARG2] : ARG_UNUSED;
            aarg0 = (args[aop] & ARG0) ? state->texture_states[i][WINED3D_TSS_ALPHA_ARG0] : ARG_UNUSED;
        }

        if (!i && state->textures[0] && state->render_states[WINED3D_RS_COLORKEYENABLE])
        {
            GLenum texture_dimensions;

            texture = state->textures[0];
            texture_dimensions = wined3d_texture_gl(texture)->target;

            if (texture_dimensions == GL_TEXTURE_2D || texture_dimensions == GL_TEXTURE_RECTANGLE_ARB)
            {
                if (texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT && !texture->resource.format->alpha_size)
                {
                    if (aop == WINED3D_TOP_DISABLE)
                    {
                       aarg1 = WINED3DTA_TEXTURE;
                       aop = WINED3D_TOP_SELECT_ARG1;
                    }
                    else if (aop == WINED3D_TOP_SELECT_ARG1 && aarg1 != WINED3DTA_TEXTURE)
                    {
                        if (state->render_states[WINED3D_RS_ALPHABLENDENABLE])
                        {
                            aarg2 = WINED3DTA_TEXTURE;
                            aop = WINED3D_TOP_MODULATE;
                        }
                        else aarg1 = WINED3DTA_TEXTURE;
                    }
                    else if (aop == WINED3D_TOP_SELECT_ARG2 && aarg2 != WINED3DTA_TEXTURE)
                    {
                        if (state->render_states[WINED3D_RS_ALPHABLENDENABLE])
                        {
                            aarg1 = WINED3DTA_TEXTURE;
                            aop = WINED3D_TOP_MODULATE;
                        }
                        else aarg2 = WINED3DTA_TEXTURE;
                    }
                }
            }
        }

        if (is_invalid_op(state, i, aop, aarg1, aarg2, aarg0))
        {
               aarg0 = ARG_UNUSED;
               aarg2 = ARG_UNUSED;
               aarg1 = WINED3DTA_CURRENT;
               aop = WINED3D_TOP_SELECT_ARG1;
        }

        if (carg1 == WINED3DTA_TEXTURE || carg2 == WINED3DTA_TEXTURE || carg0 == WINED3DTA_TEXTURE
                || aarg1 == WINED3DTA_TEXTURE || aarg2 == WINED3DTA_TEXTURE || aarg0 == WINED3DTA_TEXTURE)
        {
            ttff = state->texture_states[i][WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS];
            if (ttff == (WINED3D_TTFF_PROJECTED | WINED3D_TTFF_COUNT3))
                settings->op[i].projected = WINED3D_PROJECTION_COUNT3;
            else if (ttff & WINED3D_TTFF_PROJECTED)
                settings->op[i].projected = WINED3D_PROJECTION_COUNT4;
            else
                settings->op[i].projected = WINED3D_PROJECTION_NONE;
        }
        else
        {
            settings->op[i].projected = WINED3D_PROJECTION_NONE;
        }

        settings->op[i].cop = cop;
        settings->op[i].aop = aop;
        settings->op[i].carg0 = carg0;
        settings->op[i].carg1 = carg1;
        settings->op[i].carg2 = carg2;
        settings->op[i].aarg0 = aarg0;
        settings->op[i].aarg1 = aarg1;
        settings->op[i].aarg2 = aarg2;
        settings->op[i].tmp_dst = state->texture_states[i][WINED3D_TSS_RESULT_ARG] == WINED3DTA_TEMP;
    }

    /* Clear unsupported stages */
    for(; i < WINED3D_MAX_TEXTURES; i++) {
        memset(&settings->op[i], 0xff, sizeof(settings->op[i]));
    }

    if (!state->render_states[WINED3D_RS_FOGENABLE])
    {
        settings->fog = WINED3D_FFP_PS_FOG_OFF;
    }
    else if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
    {
        if (use_vs(state) || state->vertex_declaration->position_transformed)
        {
            settings->fog = WINED3D_FFP_PS_FOG_LINEAR;
        }
        else
        {
            switch (state->render_states[WINED3D_RS_FOGVERTEXMODE])
            {
                case WINED3D_FOG_NONE:
                case WINED3D_FOG_LINEAR:
                    settings->fog = WINED3D_FFP_PS_FOG_LINEAR;
                    break;
                case WINED3D_FOG_EXP:
                    settings->fog = WINED3D_FFP_PS_FOG_EXP;
                    break;
                case WINED3D_FOG_EXP2:
                    settings->fog = WINED3D_FFP_PS_FOG_EXP2;
                    break;
            }
        }
    }
    else
    {
        switch (state->render_states[WINED3D_RS_FOGTABLEMODE])
        {
            case WINED3D_FOG_LINEAR:
                settings->fog = WINED3D_FFP_PS_FOG_LINEAR;
                break;
            case WINED3D_FOG_EXP:
                settings->fog = WINED3D_FFP_PS_FOG_EXP;
                break;
            case WINED3D_FOG_EXP2:
                settings->fog = WINED3D_FFP_PS_FOG_EXP2;
                break;
        }
    }
    settings->sRGB_write = !d3d_info->srgb_write_control && needs_srgb_write(context, state, state->fb);
    if (d3d_info->vs_clipping || !use_vs(state) || !state->render_states[WINED3D_RS_CLIPPING]
            || !state->render_states[WINED3D_RS_CLIPPLANEENABLE])
    {
        /* No need to emulate clipplanes if GL supports native vertex shader clipping or if
         * the fixed function vertex pipeline is used(which always supports clipplanes), or
         * if no clipplane is enabled
         */
        settings->emul_clipplanes = 0;
    } else {
        settings->emul_clipplanes = 1;
    }

    if (state->render_states[WINED3D_RS_COLORKEYENABLE] && state->textures[0]
            && state->textures[0]->async.color_key_flags & WINED3D_CKEY_SRC_BLT
            && settings->op[0].cop != WINED3D_TOP_DISABLE)
        settings->color_key_enabled = 1;
    else
        settings->color_key_enabled = 0;

    /* texcoords_initialized is set to meaningful values only when GL doesn't
     * support enough varyings to always pass around all the possible texture
     * coordinates.
     * This is used to avoid reading a varying not written by the vertex shader.
     * Reading uninitialized varyings on core profile contexts results in an
     * error while with builtin varyings on legacy contexts you get undefined
     * behavior. */
    if (d3d_info->limits.varying_count && !d3d_info->full_ffp_varyings)
    {
        settings->texcoords_initialized = 0;
        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (use_vs(state))
            {
                if (state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.output_registers & (1u << i))
                    settings->texcoords_initialized |= 1u << i;
            }
            else
            {
                const struct wined3d_stream_info *si = &context->stream_info;
                unsigned int coord_idx = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
                if ((state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX] >> WINED3D_FFP_TCI_SHIFT)
                        & WINED3D_FFP_TCI_MASK
                        || (coord_idx < WINED3D_MAX_TEXTURES && (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx)))))
                    settings->texcoords_initialized |= 1u << i;
            }
        }
    }
    else
    {
        settings->texcoords_initialized = (1u << WINED3D_MAX_TEXTURES) - 1;
    }

    settings->pointsprite = state->render_states[WINED3D_RS_POINTSPRITEENABLE]
            && state->gl_primitive_type == GL_POINTS;

    if (d3d_info->ffp_alpha_test)
        settings->alpha_test_func = WINED3D_CMP_ALWAYS - 1;
    else
        settings->alpha_test_func = (state->render_states[WINED3D_RS_ALPHATESTENABLE]
                ? wined3d_sanitize_cmp_func(state->render_states[WINED3D_RS_ALPHAFUNC])
                : WINED3D_CMP_ALWAYS) - 1;

    if (d3d_info->emulated_flatshading)
        settings->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
    else
        settings->flatshading = FALSE;
}

const struct ffp_frag_desc *find_ffp_frag_shader(const struct wine_rb_tree *fragment_shaders,
        const struct ffp_frag_settings *settings)
{
    struct wine_rb_entry *entry = wine_rb_get(fragment_shaders, settings);
    return entry ? WINE_RB_ENTRY_VALUE(entry, struct ffp_frag_desc, entry) : NULL;
}

void add_ffp_frag_shader(struct wine_rb_tree *shaders, struct ffp_frag_desc *desc)
{
    /* Note that the key is the implementation independent part of the ffp_frag_desc structure,
     * whereas desc points to an extended structure with implementation specific parts. */
    if (wine_rb_put(shaders, &desc->settings, &desc->entry) == -1)
    {
        ERR("Failed to insert ffp frag shader.\n");
    }
}

/* Activates the texture dimension according to the bound D3D texture. Does
 * not care for the colorop or correct gl texture unit (when using nvrc).
 * Requires the caller to activate the correct unit. */
/* Context activation is done by the caller (state handler). */
void texture_activate_dimensions(struct wined3d_texture *texture, const struct wined3d_gl_info *gl_info)
{
    if (texture)
    {
        switch (wined3d_texture_gl(texture)->target)
        {
            case GL_TEXTURE_2D:
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
                checkGLcall("glEnable(GL_TEXTURE_2D)");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glEnable(GL_TEXTURE_RECTANGLE_ARB)");
                break;
            case GL_TEXTURE_3D:
                if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_3D);
                checkGLcall("glEnable(GL_TEXTURE_3D)");
                break;
            case GL_TEXTURE_CUBE_MAP_ARB:
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
                {
                    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glEnable(GL_TEXTURE_CUBE_MAP_ARB)");
              break;
        }
    }
    else
    {
        gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
        checkGLcall("glEnable(GL_TEXTURE_2D)");
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
        checkGLcall("glDisable(GL_TEXTURE_3D)");
        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
        }
        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
            checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
        }
        /* Binding textures is done by samplers. A dummy texture will be bound */
    }
}

/* Context activation is done by the caller (state handler). */
void sampler_texdim(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    unsigned int sampler = state_id - STATE_SAMPLER(0);
    unsigned int mapped_stage;

    /* No need to enable / disable anything here for unused samplers. The
     * tex_colorop handler takes care. Also no action is needed with pixel
     * shaders, or if tex_colorop will take care of this business. */
    mapped_stage = context_gl->tex_unit_map[sampler];
    if (mapped_stage == WINED3D_UNMAPPED_STAGE || mapped_stage >= context_gl->gl_info->limits.textures)
        return;
    if (sampler >= context->lowest_disabled_stage)
        return;
    if (isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3D_TSS_COLOR_OP)))
        return;

    texture_activate_dimensions(state->textures[sampler], context_gl->gl_info);
}

int wined3d_ffp_frag_program_key_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct ffp_frag_settings *ka = key;
    const struct ffp_frag_settings *kb = &WINE_RB_ENTRY_VALUE(entry, const struct ffp_frag_desc, entry)->settings;

    return memcmp(ka, kb, sizeof(*ka));
}

void wined3d_ffp_get_vs_settings(const struct wined3d_context *context,
        const struct wined3d_state *state, struct wined3d_ffp_vs_settings *settings)
{
    enum wined3d_material_color_source diffuse_source, emissive_source, ambient_source, specular_source;
    const struct wined3d_stream_info *si = &context->stream_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    unsigned int coord_idx, i;

    memset(settings, 0, sizeof(*settings));

    if (si->position_transformed)
    {
        settings->transformed = 1;
        settings->point_size = state->gl_primitive_type == GL_POINTS;
        settings->per_vertex_point_size = !!(si->use_map & 1u << WINED3D_FFP_PSIZE);
        if (!state->render_states[WINED3D_RS_FOGENABLE])
            settings->fog_mode = WINED3D_FFP_VS_FOG_OFF;
        else if (state->render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
            settings->fog_mode = WINED3D_FFP_VS_FOG_DEPTH;
        else
            settings->fog_mode = WINED3D_FFP_VS_FOG_FOGCOORD;

        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            coord_idx = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
            if (coord_idx < WINED3D_MAX_TEXTURES && (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx))))
                settings->texcoords |= 1u << i;
            settings->texgen[i] = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
        }
        if (d3d_info->full_ffp_varyings)
            settings->texcoords = (1u << WINED3D_MAX_TEXTURES) - 1;

        if (d3d_info->emulated_flatshading)
            settings->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
        else
            settings->flatshading = FALSE;

        settings->swizzle_map = si->swizzle_map;

        return;
    }

    switch (state->render_states[WINED3D_RS_VERTEXBLEND])
    {
        case WINED3D_VBF_DISABLE:
        case WINED3D_VBF_1WEIGHTS:
        case WINED3D_VBF_2WEIGHTS:
        case WINED3D_VBF_3WEIGHTS:
            settings->vertexblends = state->render_states[WINED3D_RS_VERTEXBLEND];
            break;
        default:
            FIXME("Unsupported vertex blending: %d\n", state->render_states[WINED3D_RS_VERTEXBLEND]);
            break;
    }

    settings->clipping = state->render_states[WINED3D_RS_CLIPPING]
            && state->render_states[WINED3D_RS_CLIPPLANEENABLE];
    settings->normal = !!(si->use_map & (1u << WINED3D_FFP_NORMAL));
    settings->normalize = settings->normal && state->render_states[WINED3D_RS_NORMALIZENORMALS];
    settings->lighting = !!state->render_states[WINED3D_RS_LIGHTING];
    settings->localviewer = !!state->render_states[WINED3D_RS_LOCALVIEWER];
    settings->point_size = state->gl_primitive_type == GL_POINTS;
    settings->per_vertex_point_size = !!(si->use_map & 1u << WINED3D_FFP_PSIZE);

    wined3d_get_material_colour_source(&diffuse_source, &emissive_source,
            &ambient_source, &specular_source, state, si);
    settings->diffuse_source = diffuse_source;
    settings->emissive_source = emissive_source;
    settings->ambient_source = ambient_source;
    settings->specular_source = specular_source;

    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        coord_idx = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
        if (coord_idx < WINED3D_MAX_TEXTURES && (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx))))
            settings->texcoords |= 1u << i;
        settings->texgen[i] = state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX];
    }
    if (d3d_info->full_ffp_varyings)
        settings->texcoords = (1u << WINED3D_MAX_TEXTURES) - 1;

    for (i = 0; i < WINED3D_MAX_ACTIVE_LIGHTS; ++i)
    {
        if (!state->light_state.lights[i])
            continue;

        switch (state->light_state.lights[i]->OriginalParms.type)
        {
            case WINED3D_LIGHT_POINT:
                ++settings->point_light_count;
                break;
            case WINED3D_LIGHT_SPOT:
                ++settings->spot_light_count;
                break;
            case WINED3D_LIGHT_DIRECTIONAL:
                ++settings->directional_light_count;
                break;
            case WINED3D_LIGHT_PARALLELPOINT:
                ++settings->parallel_point_light_count;
                break;
            default:
                FIXME("Unhandled light type %#x.\n", state->light_state.lights[i]->OriginalParms.type);
                break;
        }
    }

    if (!state->render_states[WINED3D_RS_FOGENABLE])
        settings->fog_mode = WINED3D_FFP_VS_FOG_OFF;
    else if (state->render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
    {
        settings->fog_mode = WINED3D_FFP_VS_FOG_DEPTH;

        if (state->transforms[WINED3D_TS_PROJECTION]._14 == 0.0f
                && state->transforms[WINED3D_TS_PROJECTION]._24 == 0.0f
                && state->transforms[WINED3D_TS_PROJECTION]._34 == 0.0f
                && state->transforms[WINED3D_TS_PROJECTION]._44 == 1.0f)
            settings->ortho_fog = 1;
    }
    else if (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE)
        settings->fog_mode = WINED3D_FFP_VS_FOG_FOGCOORD;
    else if (state->render_states[WINED3D_RS_RANGEFOGENABLE])
        settings->fog_mode = WINED3D_FFP_VS_FOG_RANGE;
    else
        settings->fog_mode = WINED3D_FFP_VS_FOG_DEPTH;

    if (d3d_info->emulated_flatshading)
        settings->flatshading = state->render_states[WINED3D_RS_SHADEMODE] == WINED3D_SHADE_FLAT;
    else
        settings->flatshading = FALSE;

    settings->swizzle_map = si->swizzle_map;
}

int wined3d_ffp_vertex_program_key_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_ffp_vs_settings *ka = key;
    const struct wined3d_ffp_vs_settings *kb = &WINE_RB_ENTRY_VALUE(entry,
            const struct wined3d_ffp_vs_desc, entry)->settings;

    return memcmp(ka, kb, sizeof(*ka));
}

const char *wined3d_debug_location(DWORD location)
{
    struct debug_buffer buffer;
    const char *prefix = "";
    const char *suffix = "";

    if (wined3d_popcount(location) > 16)
    {
        prefix = "~(";
        location = ~location;
        suffix = ")";
    }

    init_debug_buffer(&buffer, "0");
#define LOCATION_TO_STR(x) if (location & x) { debug_append(&buffer, #x, " | "); location &= ~x; }
    LOCATION_TO_STR(WINED3D_LOCATION_DISCARDED);
    LOCATION_TO_STR(WINED3D_LOCATION_SYSMEM);
    LOCATION_TO_STR(WINED3D_LOCATION_USER_MEMORY);
    LOCATION_TO_STR(WINED3D_LOCATION_BUFFER);
    LOCATION_TO_STR(WINED3D_LOCATION_TEXTURE_RGB);
    LOCATION_TO_STR(WINED3D_LOCATION_TEXTURE_SRGB);
    LOCATION_TO_STR(WINED3D_LOCATION_DRAWABLE);
    LOCATION_TO_STR(WINED3D_LOCATION_RB_MULTISAMPLE);
    LOCATION_TO_STR(WINED3D_LOCATION_RB_RESOLVED);
#undef LOCATION_TO_STR
    if (location)
        FIXME("Unrecognized location flag(s) %#x.\n", location);

    return wine_dbg_sprintf("%s%s%s", prefix, buffer.str, suffix);
}

const char *wined3d_debug_feature_level(enum wined3d_feature_level level)
{
    switch (level)
    {
#define LEVEL_TO_STR(level) case level: return #level
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_5);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_6);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_7);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_8);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_9_1);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_9_2);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_9_3);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_10);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_10_1);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_11);
        LEVEL_TO_STR(WINED3D_FEATURE_LEVEL_11_1);
#undef LEVEL_TO_STR
        default:
            return wine_dbg_sprintf("%#x", level);
    }
}

/* Print a floating point value with the %.8e format specifier, always using
 * '.' as decimal separator. */
void wined3d_ftoa(float value, char *s)
{
    int idx = 1;

    if (copysignf(1.0f, value) < 0.0f)
        ++idx;

    /* Be sure to allocate a buffer of at least 17 characters for the result
       as sprintf may return a 3 digit exponent when using the MSVC runtime
       instead of a 2 digit exponent. */
    sprintf(s, "%.8e", value);
    if (isfinite(value))
        s[idx] = '.';
}

void wined3d_release_dc(HWND window, HDC dc)
{
    /* You'd figure ReleaseDC() would fail if the DC doesn't match the window.
     * However, that's not what actually happens, and there are user32 tests
     * that confirm ReleaseDC() with the wrong window is supposed to succeed.
     * So explicitly check that the DC belongs to the window, since we want to
     * avoid releasing a DC that belongs to some other window if the original
     * window was already destroyed. */
    if (WindowFromDC(dc) != window)
        WARN("DC %p does not belong to window %p.\n", dc, window);
    else if (!ReleaseDC(window, dc))
        ERR("Failed to release device context %p, last error %#x.\n", dc, GetLastError());
}

BOOL wined3d_clip_blit(const RECT *clip_rect, RECT *clipped, RECT *other)
{
    RECT orig = *clipped;
    float scale_x = (float)(orig.right - orig.left) / (float)(other->right - other->left);
    float scale_y = (float)(orig.bottom - orig.top) / (float)(other->bottom - other->top);

    IntersectRect(clipped, clipped, clip_rect);

    if (IsRectEmpty(clipped))
    {
        SetRectEmpty(other);
        return FALSE;
    }

    other->left += (LONG)((clipped->left - orig.left) / scale_x);
    other->top += (LONG)((clipped->top - orig.top) / scale_y);
    other->right -= (LONG)((orig.right - clipped->right) / scale_x);
    other->bottom -= (LONG)((orig.bottom - clipped->bottom) / scale_y);

    return TRUE;
}

void wined3d_gl_limits_get_uniform_block_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count)
{
    unsigned int i;

    *base = 0;
    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        *count = gl_limits->uniform_blocks[i];
        if (i == shader_type)
            return;
        *base += *count;
    }

    ERR("Unrecognized shader type %#x.\n", shader_type);
    *count = 0;
}

void wined3d_gl_limits_get_texture_unit_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count)
{
    unsigned int i;

    if (shader_type == WINED3D_SHADER_TYPE_COMPUTE)
    {
        if (gl_limits->combined_samplers == gl_limits->graphics_samplers)
            *base = 0;
        else
            *base = gl_limits->graphics_samplers;
        *count = gl_limits->samplers[WINED3D_SHADER_TYPE_COMPUTE];
        return;
    }

    *base = 0;
    for (i = 0; i < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++i)
    {
        *count = gl_limits->samplers[i];
        if (i == shader_type)
            return;
        *base += *count;
    }

    ERR("Unrecognized shader type %#x.\n", shader_type);
    *count = 0;
}

BOOL wined3d_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size)
{
    SIZE_T max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(1, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    if (!*elements)
        new_elements = heap_alloc_zero(new_capacity * size);
    else
        new_elements = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *elements, new_capacity * size);
    if (!new_elements)
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

static void swap_rows(float **a, float **b)
{
    float *tmp = *a;

    *a = *b;
    *b = tmp;
}

BOOL invert_matrix(struct wined3d_matrix *out, const struct wined3d_matrix *m)
{
    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;

    r0 = wtmp[0];
    r1 = wtmp[1];
    r2 = wtmp[2];
    r3 = wtmp[3];

    r0[0] = m->_11;
    r0[1] = m->_12;
    r0[2] = m->_13;
    r0[3] = m->_14;
    r0[4] = 1.0f;
    r0[5] = r0[6] = r0[7] = 0.0f;

    r1[0] = m->_21;
    r1[1] = m->_22;
    r1[2] = m->_23;
    r1[3] = m->_24;
    r1[5] = 1.0f;
    r1[4] = r1[6] = r1[7] = 0.0f;

    r2[0] = m->_31;
    r2[1] = m->_32;
    r2[2] = m->_33;
    r2[3] = m->_34;
    r2[6] = 1.0f;
    r2[4] = r2[5] = r2[7] = 0.0f;

    r3[0] = m->_41;
    r3[1] = m->_42;
    r3[2] = m->_43;
    r3[3] = m->_44;
    r3[7] = 1.0f;
    r3[4] = r3[5] = r3[6] = 0.0f;

    /* Choose pivot - or die. */
    if (fabsf(r3[0]) > fabsf(r2[0]))
        swap_rows(&r3, &r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        swap_rows(&r2, &r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        swap_rows(&r1, &r0);
    if (r0[0] == 0.0f)
        return FALSE;

    /* Eliminate first variable. */
    m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
    s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
    s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
    s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0f)
    {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0f)
    {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0f)
    {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0f)
    {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* Choose pivot - or die. */
    if (fabsf(r3[1]) > fabsf(r2[1]))
        swap_rows(&r3, &r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        swap_rows(&r2, &r1);
    if (r1[1] == 0.0f)
        return FALSE;

    /* Eliminate second variable. */
    m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
    s = r1[4];
    if (s != 0.0f)
    {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (s != 0.0f)
    {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (s != 0.0f)
    {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (s != 0.0f)
    {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* Choose pivot - or die. */
    if (fabsf(r3[2]) > fabsf(r2[2]))
        swap_rows(&r3, &r2);
    if (r2[2] == 0.0f)
        return FALSE;

    /* Eliminate third variable. */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3];
    r3[4] -= m3 * r2[4];
    r3[5] -= m3 * r2[5];
    r3[6] -= m3 * r2[6];
    r3[7] -= m3 * r2[7];

    /* Last check. */
    if (r3[3] == 0.0f)
        return FALSE;

    /* Back substitute row 3. */
    s = 1.0f / r3[3];
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    /* Back substitute row 2. */
    m2 = r2[3];
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2);
    r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2);
    r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1;
    r1[5] -= r3[5] * m1;
    r1[6] -= r3[6] * m1;
    r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0;
    r0[5] -= r3[5] * m0;
    r0[6] -= r3[6] * m0;
    r0[7] -= r3[7] * m0;

    /* Back substitute row 1. */
    m1 = r1[2];
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1);
    r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1);
    r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0;
    r0[5] -= r2[5] * m0;
    r0[6] -= r2[6] * m0;
    r0[7] -= r2[7] * m0;

    /* Back substitute row 0. */
    m0 = r0[1];
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0);
    r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0);
    r0[7] = s * (r0[7] - r1[7] * m0);

    out->_11 = r0[4];
    out->_12 = r0[5];
    out->_13 = r0[6];
    out->_14 = r0[7];
    out->_21 = r1[4];
    out->_22 = r1[5];
    out->_23 = r1[6];
    out->_24 = r1[7];
    out->_31 = r2[4];
    out->_32 = r2[5];
    out->_33 = r2[6];
    out->_34 = r2[7];
    out->_41 = r3[4];
    out->_42 = r3[5];
    out->_43 = r3[6];
    out->_44 = r3[7];

    return TRUE;
}

/* Taken and adapted from Mesa. */
BOOL invert_matrix_3d(struct wined3d_matrix *out, const struct wined3d_matrix *in)
{
    float pos, neg, t, det;
    struct wined3d_matrix temp;

    /* Calculate the determinant of upper left 3x3 submatrix and
     * determine if the matrix is singular. */
    pos = neg = 0.0f;
    t =  in->_11 * in->_22 * in->_33;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t =  in->_21 * in->_32 * in->_13;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;
    t =  in->_31 * in->_12 * in->_23;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t = -in->_31 * in->_22 * in->_13;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;
    t = -in->_21 * in->_12 * in->_33;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    t = -in->_11 * in->_32 * in->_23;
    if (t >= 0.0f)
        pos += t;
    else
        neg += t;

    det = pos + neg;

    if (fabsf(det) < 1e-25f)
        return FALSE;

    det = 1.0f / det;
    temp._11 =  (in->_22 * in->_33 - in->_32 * in->_23) * det;
    temp._12 = -(in->_12 * in->_33 - in->_32 * in->_13) * det;
    temp._13 =  (in->_12 * in->_23 - in->_22 * in->_13) * det;
    temp._21 = -(in->_21 * in->_33 - in->_31 * in->_23) * det;
    temp._22 =  (in->_11 * in->_33 - in->_31 * in->_13) * det;
    temp._23 = -(in->_11 * in->_23 - in->_21 * in->_13) * det;
    temp._31 =  (in->_21 * in->_32 - in->_31 * in->_22) * det;
    temp._32 = -(in->_11 * in->_32 - in->_31 * in->_12) * det;
    temp._33 =  (in->_11 * in->_22 - in->_21 * in->_12) * det;

    *out = temp;
    return TRUE;
}

void compute_normal_matrix(float *normal_matrix, BOOL legacy_lighting,
        const struct wined3d_matrix *modelview)
{
    struct wined3d_matrix mv;
    unsigned int i, j;

    mv = *modelview;
    if (legacy_lighting)
        invert_matrix_3d(&mv, &mv);
    else
        invert_matrix(&mv, &mv);
    /* Tests show that singular modelview matrices are used unchanged as normal
     * matrices on D3D3 and older. There seems to be no clearly consistent
     * behavior on newer D3D versions so always follow older ddraw behavior. */
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            normal_matrix[i * 3 + j] = (&mv._11)[j * 4 + i];
}
