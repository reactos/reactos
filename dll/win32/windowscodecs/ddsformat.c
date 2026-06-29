/*
 * Copyright 2020 Ziqing Hui
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
 *
 * Note:
 *
 * Uncompressed image:
 *     For uncompressed formats, a block is equivalent to a pixel.
 *
 * Cube map:
 *     A cube map is equivalent to a 2D texture array which has 6 textures.
 *     A cube map array is equivalent to a 2D texture array which has cubeCount*6 textures.
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#define DDS_MAGIC 0x20534444
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#define GET_RGB565_R(color)   ((BYTE)(((color) >> 11) & 0x1F))
#define GET_RGB565_G(color)   ((BYTE)(((color) >> 5)  & 0x3F))
#define GET_RGB565_B(color)   ((BYTE)(((color) >> 0)  & 0x1F))
#define MAKE_RGB565(r, g, b)  ((WORD)(((BYTE)(r) << 11) | ((BYTE)(g) << 5) | (BYTE)(b)))
#define MAKE_ARGB(a, r, g, b) (((DWORD)(a) << 24) | ((DWORD)(r) << 16) | ((DWORD)(g) << 8) | (DWORD)(b))

#define DDPF_ALPHAPIXELS     0x00000001
#define DDPF_ALPHA           0x00000002
#define DDPF_FOURCC          0x00000004
#define DDPF_PALETTEINDEXED8 0x00000020
#define DDPF_RGB             0x00000040
#define DDPF_LUMINANCE       0x00020000
#define DDPF_BUMPDUDV        0x00080000

#define DDSCAPS2_CUBEMAP 0x00000200
#define DDSCAPS2_VOLUME  0x00200000

#define DDS_DIMENSION_TEXTURE1D 2
#define DDS_DIMENSION_TEXTURE2D 3
#define DDS_DIMENSION_TEXTURE3D 4

#define DDS_RESOURCE_MISC_TEXTURECUBE 0x00000004

#define DDS_BLOCK_WIDTH  4
#define DDS_BLOCK_HEIGHT 4

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD fourCC;
    DWORD rgbBitCount;
    DWORD rBitMask;
    DWORD gBitMask;
    DWORD bBitMask;
    DWORD aBitMask;
} DDS_PIXELFORMAT;

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitchOrLinearSize;
    DWORD depth;
    DWORD mipMapCount;
    DWORD reserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
} DDS_HEADER;

typedef struct {
    DWORD dxgiFormat;
    DWORD resourceDimension;
    DWORD miscFlag;
    DWORD arraySize;
    DWORD miscFlags2;
} DDS_HEADER_DXT10;

typedef struct dds_info {
    UINT width;
    UINT height;
    UINT depth;
    UINT mip_levels;
    UINT array_size;
    UINT frame_count;
    UINT data_offset;
    UINT bytes_per_block; /* for uncompressed format, this means bytes per pixel*/
    DXGI_FORMAT format;
    WICDdsDimension dimension;
    WICDdsAlphaMode alpha_mode;
    const GUID *pixel_format;
    UINT pixel_format_bpp;
} dds_info;

typedef struct dds_frame_info {
    UINT width;
    UINT height;
    DXGI_FORMAT format;
    UINT bytes_per_block; /* for uncompressed format, this means bytes per pixel*/
    UINT block_width;
    UINT block_height;
    UINT width_in_blocks;
    UINT height_in_blocks;
    const GUID *pixel_format;
    UINT pixel_format_bpp;
} dds_frame_info;

typedef struct DdsDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICDdsDecoder IWICDdsDecoder_iface;
    IWICWineDecoder IWICWineDecoder_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    CRITICAL_SECTION lock;
    dds_info info;
} DdsDecoder;

typedef struct DdsFrameDecode {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICDdsFrameDecode IWICDdsFrameDecode_iface;
    LONG ref;
    BYTE *block_data;
    BYTE *pixel_data;
    CRITICAL_SECTION lock;
    dds_frame_info info;
} DdsFrameDecode;

typedef struct DdsEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    IWICDdsEncoder IWICDdsEncoder_iface;
    LONG ref;
    CRITICAL_SECTION lock;
    IStream *stream;
    UINT frame_count;
    UINT frame_index;
    BOOL uncommitted_frame;
    BOOL committed;
    dds_info info;
} DdsEncoder;

typedef struct DdsFrameEncode {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    DdsEncoder *parent;
    BOOL initialized;
    BOOL frame_created;
    UINT width;
    UINT height;
    double dpi_x;
    double dpi_y;
} DdsFrameEncode;

static struct dds_format {
    DDS_PIXELFORMAT pixel_format;
    const GUID *wic_format;
    UINT wic_format_bpp;
    DXGI_FORMAT dxgi_format;
} dds_format_table[] = {
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '1'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppPBGRA, 32,       DXGI_FORMAT_BC1_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '2'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppPBGRA, 32,       DXGI_FORMAT_BC2_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '3'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC2_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '4'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppPBGRA, 32,       DXGI_FORMAT_BC3_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '5'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC3_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('B', 'C', '4', 'U'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC4_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('B', 'C', '4', 'S'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC4_SNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('B', 'C', '5', 'U'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC5_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('B', 'C', '5', 'S'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC5_SNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('A', 'T', 'I', '1'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC4_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('A', 'T', 'I', '2'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppBGRA,  32,       DXGI_FORMAT_BC5_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('R', 'G', 'B', 'G'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bpp4Channels, 32,   DXGI_FORMAT_R8G8_B8G8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('G', 'R', 'G', 'B'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bpp4Channels, 32,   DXGI_FORMAT_G8R8_G8B8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', '1', '0'), 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormatUndefined,       0,   DXGI_FORMAT_UNKNOWN },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x24, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat64bppRGBA,       64,  DXGI_FORMAT_R16G16B16A16_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x6E, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat64bppRGBA,       64,  DXGI_FORMAT_R16G16B16A16_SNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x6F, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat16bppGrayHalf,   16,  DXGI_FORMAT_R16_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x70, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormatUndefined,       0,   DXGI_FORMAT_R16G16_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x71, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat64bppRGBAHalf,   64,  DXGI_FORMAT_R16G16B16A16_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x72, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat32bppGrayFloat,  32,  DXGI_FORMAT_R32_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x73, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormatUndefined,       32,  DXGI_FORMAT_R32G32_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_FOURCC, 0x74, 0, 0, 0, 0, 0 },
      &GUID_WICPixelFormat128bppRGBAFloat, 128, DXGI_FORMAT_R32G32B32A32_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0xFF,0xFF00,0xFF0000,0xFF000000 },
      &GUID_WICPixelFormat32bppRGBA,        32, DXGI_FORMAT_R8G8B8A8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0xFF,0xFF00,0xFF0000,0 },
      &GUID_WICPixelFormat32bppRGB,         32, DXGI_FORMAT_UNKNOWN },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0xFF0000,0xFF00,0xFF,0xFF000000 },
      &GUID_WICPixelFormat32bppBGRA,        32, DXGI_FORMAT_B8G8R8A8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0xFF0000,0xFF00,0xFF,0 },
      &GUID_WICPixelFormat32bppBGR,         32, DXGI_FORMAT_B8G8R8X8_UNORM },
    /* The red and blue masks are swapped for DXGI_FORMAT_R10G10B10A2_UNORM.
     * For "correct" one, the RGB masks should be 0x3FF,0xFFC00,0x3FF00000.
     * see: https://walbourn.github.io/dds-update-and-1010102-problems */
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0x3FF00000,0xFFC00,0x3FF,0xC0000000 },
      &GUID_WICPixelFormat32bppR10G10B10A2, 32, DXGI_FORMAT_R10G10B10A2_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 32, 0x3FF,0xFFC00,0x3FF00000,0xC0000000 },
      &GUID_WICPixelFormat32bppRGBA1010102, 32, DXGI_FORMAT_R10G10B10A2_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB ,      0, 32, 0xFFFF,0xFFFF0000,0,0 },
      &GUID_WICPixelFormatUndefined,        0,  DXGI_FORMAT_R16G16_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB ,      0, 32, 0xFFFFFFFF,0,0,0 },
      &GUID_WICPixelFormat32bppGrayFloat,   32, DXGI_FORMAT_R32_FLOAT },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB ,      0, 24, 0xFF0000,0x00FF00,0x0000FF,0 },
      &GUID_WICPixelFormat24bppBGR,         24, DXGI_FORMAT_UNKNOWN },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB ,      0, 24, 0x0000FF,0x00FF00,0xFF0000,0 },
      &GUID_WICPixelFormat24bppRGB,         24, DXGI_FORMAT_UNKNOWN },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 16, 0xF800,0x7E0,0x1F,0 },
      &GUID_WICPixelFormat16bppBGR565,      16, DXGI_FORMAT_B5G6R5_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 16, 0x7C00,0x3E0,0x1F,0 },
      &GUID_WICPixelFormat16bppBGR555,      16, DXGI_FORMAT_UNKNOWN },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 16, 0x7C00,0x3E0,0x1F,0x8000 },
      &GUID_WICPixelFormat16bppBGRA5551,    16, DXGI_FORMAT_B5G5R5A1_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_RGB,       0, 16, 0xF00,0xF0,0xF,0xF000 },
      &GUID_WICPixelFormatUndefined,        0,  DXGI_FORMAT_B4G4R4A4_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_ALPHA,     0, 8,  0,0,0,0xFF },
      &GUID_WICPixelFormat8bppAlpha,        8,  DXGI_FORMAT_A8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_LUMINANCE, 0, 16, 0xFFFF,0,0,0 },
      &GUID_WICPixelFormat16bppGray,        16, DXGI_FORMAT_R16_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_LUMINANCE, 0, 16, 0xFF,0,0,0xFF00 },
      &GUID_WICPixelFormatUndefined,        0,  DXGI_FORMAT_R8G8_UNORM },
    { { sizeof(DDS_PIXELFORMAT), DDPF_LUMINANCE, 0, 8,  0xFF,0,0,0 },
      &GUID_WICPixelFormat8bppGray,         8,  DXGI_FORMAT_R8_UNORM },
    { { 0 }, &GUID_WICPixelFormat8bppAlpha,          8,   DXGI_FORMAT_A8_UNORM },
    { { 0 }, &GUID_WICPixelFormat8bppGray,           8,   DXGI_FORMAT_R8_UNORM },
    { { 0 }, &GUID_WICPixelFormat16bppGray,          16,  DXGI_FORMAT_R16_UNORM },
    { { 0 }, &GUID_WICPixelFormat16bppGrayHalf,      16,  DXGI_FORMAT_R16_FLOAT },
    { { 0 }, &GUID_WICPixelFormat16bppBGR565,        16,  DXGI_FORMAT_B5G6R5_UNORM },
    { { 0 }, &GUID_WICPixelFormat16bppBGRA5551,      16,  DXGI_FORMAT_B5G5R5A1_UNORM },
    { { 0 }, &GUID_WICPixelFormat32bppGrayFloat,     32,  DXGI_FORMAT_R32_FLOAT },
    { { 0 }, &GUID_WICPixelFormat32bppRGBA,          32,  DXGI_FORMAT_R8G8B8A8_UNORM },
    { { 0 }, &GUID_WICPixelFormat32bppBGRA,          32,  DXGI_FORMAT_B8G8R8A8_UNORM },
    { { 0 }, &GUID_WICPixelFormat32bppBGR,           32,  DXGI_FORMAT_B8G8R8X8_UNORM },
    { { 0 }, &GUID_WICPixelFormat32bppR10G10B10A2,   32,  DXGI_FORMAT_R10G10B10A2_UNORM },
    { { 0 }, &GUID_WICPixelFormat32bppRGBE,          32,  DXGI_FORMAT_R9G9B9E5_SHAREDEXP },
    { { 0 }, &GUID_WICPixelFormat32bppRGBA1010102XR, 32,  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
    { { 0 }, &GUID_WICPixelFormat64bppRGBA,          64,  DXGI_FORMAT_R16G16B16A16_UNORM },
    { { 0 }, &GUID_WICPixelFormat64bppRGBAHalf,      64,  DXGI_FORMAT_R16G16B16A16_FLOAT },
    { { 0 }, &GUID_WICPixelFormat96bppRGBFloat,      96,  DXGI_FORMAT_R32G32B32_FLOAT },
    { { 0 }, &GUID_WICPixelFormat128bppRGBAFloat,    128, DXGI_FORMAT_R32G32B32A32_FLOAT },
    { { 0 }, &GUID_WICPixelFormatUndefined,          0,   DXGI_FORMAT_UNKNOWN }
};

static DXGI_FORMAT compressed_formats[] = {
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static HRESULT WINAPI DdsDecoder_Dds_GetFrame(IWICDdsDecoder *, UINT, UINT, UINT, IWICBitmapFrameDecode **);

static DWORD rgb565_to_argb(WORD color, BYTE alpha)
{
    return MAKE_ARGB(alpha, (GET_RGB565_R(color) * 0xFF + 0x0F) / 0x1F,
                            (GET_RGB565_G(color) * 0xFF + 0x1F) / 0x3F,
                            (GET_RGB565_B(color) * 0xFF + 0x0F) / 0x1F);
}

static inline BOOL has_extended_header(DDS_HEADER *header)
{
    return (header->ddspf.flags & DDPF_FOURCC) &&
           (header->ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));
}

static WICDdsDimension get_dimension(DDS_HEADER *header, DDS_HEADER_DXT10 *header_dxt10)
{
    if (header_dxt10) {
        if (header_dxt10->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) return WICDdsTextureCube;
        switch (header_dxt10->resourceDimension)
        {
        case DDS_DIMENSION_TEXTURE1D: return WICDdsTexture1D;
        case DDS_DIMENSION_TEXTURE2D: return WICDdsTexture2D;
        case DDS_DIMENSION_TEXTURE3D: return WICDdsTexture3D;
        default: return WICDdsTexture2D;
        }
    } else {
        if (header->caps2 & DDSCAPS2_CUBEMAP) {
            return WICDdsTextureCube;
        } else if (header->caps2 & DDSCAPS2_VOLUME) {
            return WICDdsTexture3D;
        } else {
            return WICDdsTexture2D;
        }
    }
}

static struct dds_format *get_dds_format(DDS_PIXELFORMAT *pixel_format)
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(dds_format_table); i++)
    {
        if ((pixel_format->flags & dds_format_table[i].pixel_format.flags) &&
            (pixel_format->fourCC == dds_format_table[i].pixel_format.fourCC) &&
            (pixel_format->rgbBitCount == dds_format_table[i].pixel_format.rgbBitCount) &&
            (pixel_format->rBitMask == dds_format_table[i].pixel_format.rBitMask) &&
            (pixel_format->gBitMask == dds_format_table[i].pixel_format.gBitMask) &&
            (pixel_format->bBitMask == dds_format_table[i].pixel_format.bBitMask) &&
            (pixel_format->aBitMask == dds_format_table[i].pixel_format.aBitMask))
            return dds_format_table + i;
    }

    return dds_format_table + ARRAY_SIZE(dds_format_table) - 1;
}

static WICDdsAlphaMode get_alpha_mode_from_fourcc(DWORD fourcc)
{
    switch (fourcc)
    {
        case MAKEFOURCC('D', 'X', 'T', '1'):
        case MAKEFOURCC('D', 'X', 'T', '2'):
        case MAKEFOURCC('D', 'X', 'T', '4'):
            return WICDdsAlphaModePremultiplied;
        default:
            return WICDdsAlphaModeUnknown;
    }
}

static UINT get_bytes_per_block_from_format(DXGI_FORMAT format)
{
    /* for uncompressed format, return bytes per pixel*/
    switch (format)
    {
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
            return 1;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 2;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 4;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 8;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 12;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 16;
        default:
            WARN("DXGI format 0x%x is not supported in DDS decoder\n", format);
            return 0;
    }
}

static UINT get_frame_count(UINT depth, UINT mip_levels, UINT array_size, WICDdsDimension dimension)
{
    UINT frame_count, i;

    if (depth == 1)
    {
        frame_count = mip_levels;
    }
    else
    {
        frame_count = 0;
        for (i = 0; i < mip_levels; i++)
        {
            frame_count += depth;
            if (depth > 1) depth /= 2;
        }
    }

    frame_count *= array_size;
    if (dimension == WICDdsTextureCube) frame_count *= 6;

    return frame_count;
}

static void get_frame_dds_index(UINT index, dds_info *info, UINT *array_index, UINT *mip_level, UINT *slice_index)
{
    UINT frame_per_texture, depth;

    if (info->dimension == WICDdsTextureCube)
        frame_per_texture = info->mip_levels;
    else
        frame_per_texture = info->frame_count / info->array_size;

    *array_index = index / frame_per_texture;
    *slice_index = index % frame_per_texture;
    depth = info->depth;
    *mip_level = 0;
    while (*slice_index >= depth)
    {
        *slice_index -= depth;
        (*mip_level)++;
        if (depth > 1) depth /= 2;
    }
}

static const GUID *dxgi_format_to_wic_format(DXGI_FORMAT dxgi_format)
{
    UINT i;
    for (i = 0; i < ARRAY_SIZE(dds_format_table); i++)
    {
        if (dds_format_table[i].pixel_format.size == 0 &&
            dds_format_table[i].dxgi_format == dxgi_format)
            return dds_format_table[i].wic_format;
    }
    return &GUID_WICPixelFormatUndefined;
}

static BOOL is_compressed(DXGI_FORMAT format)
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(compressed_formats); i++)
    {
        if (format == compressed_formats[i]) return TRUE;
    }
    return FALSE;
}

static void get_dds_info(dds_info* info, DDS_HEADER *header, DDS_HEADER_DXT10 *header_dxt10)
{
    struct dds_format *format_info;

    info->width = header->width;
    info->height = header->height;
    info->depth = 1;
    info->mip_levels = 1;
    info->array_size = 1;
    if (header->depth) info->depth = header->depth;
    if (header->mipMapCount) info->mip_levels = header->mipMapCount;

    if (has_extended_header(header)) {
        if (header_dxt10->arraySize) info->array_size = header_dxt10->arraySize;
        info->format = header_dxt10->dxgiFormat;
        info->dimension = get_dimension(NULL, header_dxt10);
        info->alpha_mode = header_dxt10->miscFlags2 & 0x00000008;
        info->data_offset = sizeof(DWORD) + sizeof(*header) + sizeof(*header_dxt10);
        if (is_compressed(info->format)) {
            info->pixel_format = (info->alpha_mode == WICDdsAlphaModePremultiplied) ?
                                 &GUID_WICPixelFormat32bppPBGRA : &GUID_WICPixelFormat32bppBGRA;
            info->pixel_format_bpp = 32;
        } else {
            info->pixel_format = dxgi_format_to_wic_format(info->format);
            info->pixel_format_bpp = get_bytes_per_block_from_format(info->format) * 8;
        }
    } else {
        format_info = get_dds_format(&header->ddspf);
        info->format = format_info->dxgi_format;
        info->dimension = get_dimension(header, NULL);
        info->alpha_mode = get_alpha_mode_from_fourcc(header->ddspf.fourCC);
        info->data_offset = sizeof(DWORD) + sizeof(*header);
        info->pixel_format = format_info->wic_format;
        info->pixel_format_bpp = format_info->wic_format_bpp;
    }

    if (header->ddspf.flags & (DDPF_RGB | DDPF_ALPHA | DDPF_LUMINANCE)) {
        info->bytes_per_block = header->ddspf.rgbBitCount / 8;
    } else {
        info->bytes_per_block = get_bytes_per_block_from_format(info->format);
    }

    info->frame_count = get_frame_count(info->depth, info->mip_levels, info->array_size, info->dimension);
}

static void decode_block(const BYTE *block_data, UINT block_count, DXGI_FORMAT format,
                         UINT width, UINT height, DWORD *buffer)
{
    const BYTE *block, *color_indices, *alpha_indices, *alpha_table;
    int i, j, x, y, block_x, block_y, color_index, alpha_index;
    int block_size, color_offset, color_indices_offset;
    WORD color[4], color_value = 0;
    BYTE alpha[8], alpha_value = 0;

    if (format == DXGI_FORMAT_BC1_UNORM) {
        block_size = 8;
        color_offset = 0;
        color_indices_offset = 4;
    } else {
        block_size = 16;
        color_offset = 8;
        color_indices_offset = 12;
    }
    block_x = 0;
    block_y = 0;

    for (i = 0; i < block_count; i++)
    {
        block = block_data + i * block_size;

        color[0] = *((WORD *)(block + color_offset));
        color[1] = *((WORD *)(block + color_offset + 2));
        color[2] = MAKE_RGB565(((GET_RGB565_R(color[0]) * 2 + GET_RGB565_R(color[1]) + 1) / 3),
                               ((GET_RGB565_G(color[0]) * 2 + GET_RGB565_G(color[1]) + 1) / 3),
                               ((GET_RGB565_B(color[0]) * 2 + GET_RGB565_B(color[1]) + 1) / 3));
        color[3] = MAKE_RGB565(((GET_RGB565_R(color[0]) + GET_RGB565_R(color[1]) * 2 + 1) / 3),
                               ((GET_RGB565_G(color[0]) + GET_RGB565_G(color[1]) * 2 + 1) / 3),
                               ((GET_RGB565_B(color[0]) + GET_RGB565_B(color[1]) * 2 + 1) / 3));

        switch (format)
        {
            case DXGI_FORMAT_BC1_UNORM:
                if (color[0] <= color[1]) {
                    color[2] = MAKE_RGB565(((GET_RGB565_R(color[0]) + GET_RGB565_R(color[1]) + 1) / 2),
                                           ((GET_RGB565_G(color[0]) + GET_RGB565_G(color[1]) + 1) / 2),
                                           ((GET_RGB565_B(color[0]) + GET_RGB565_B(color[1]) + 1) / 2));
                    color[3] = 0;
                }
                break;
            case DXGI_FORMAT_BC2_UNORM:
                alpha_table = block;
                break;
            case DXGI_FORMAT_BC3_UNORM:
                alpha[0] = *block;
                alpha[1] = *(block + 1);
                if (alpha[0] > alpha[1]) {
                    for (j = 2; j < 8; j++)
                    {
                        alpha[j] = (BYTE)((alpha[0] * (8 - j) + alpha[1] * (j - 1) + 3) / 7);
                    }
                } else {
                    for (j = 2; j < 6; j++)
                    {
                        alpha[j] = (BYTE)((alpha[0] * (6 - j) + alpha[1] * (j - 1) + 2) / 5);
                    }
                    alpha[6] = 0;
                    alpha[7] = 0xFF;
                }
                alpha_indices = block + 2;
                break;
            default:
                break;
        }

        color_indices = block + color_indices_offset;
        for (j = 0; j < 16; j++)
        {
            x = block_x + j % 4;
            y = block_y + j / 4;
            if (x >= width || y >= height) continue;

            color_index = (color_indices[j / 4] >> ((j % 4) * 2)) & 0x3;
            color_value = color[color_index];

            switch (format)
            {
                case DXGI_FORMAT_BC1_UNORM:
                    if ((color[0] <= color[1]) && !color_value) {
                        color_value = 0;
                        alpha_value = 0;
                    } else {
                        alpha_value = 0xFF;
                    }
                    break;
                case DXGI_FORMAT_BC2_UNORM:
                    alpha_value = (alpha_table[j / 2] >> (j % 2) * 4) & 0xF;
                    alpha_value = (BYTE)((alpha_value * 0xFF + 0x7)/ 0xF);
                    break;
                case DXGI_FORMAT_BC3_UNORM:
                    alpha_index = (*((DWORD *)(alpha_indices + (j / 8) * 3)) >> ((j % 8) * 3)) & 0x7;
                    alpha_value = alpha[alpha_index];
                    break;
                default:
                    break;
            }
            buffer[x + y * width] = rgb565_to_argb(color_value, alpha_value);
        }

        block_x += DDS_BLOCK_WIDTH;
        if (block_x >= width) {
            block_x = 0;
            block_y += DDS_BLOCK_HEIGHT;
        }
    }
}

static inline DdsDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICBitmapDecoder_iface);
}

static inline DdsDecoder *impl_from_IWICDdsDecoder(IWICDdsDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICDdsDecoder_iface);
}

static inline DdsDecoder *impl_from_IWICWineDecoder(IWICWineDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICWineDecoder_iface);
}

static inline DdsFrameDecode *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, DdsFrameDecode, IWICBitmapFrameDecode_iface);
}

static inline DdsFrameDecode *impl_from_IWICDdsFrameDecode(IWICDdsFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, DdsFrameDecode, IWICDdsFrameDecode_iface);
}

static inline DdsEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, DdsEncoder, IWICBitmapEncoder_iface);
}

static inline DdsEncoder *impl_from_IWICDdsEncoder(IWICDdsEncoder *iface)
{
    return CONTAINING_RECORD(iface, DdsEncoder, IWICDdsEncoder_iface);
}

static inline DdsFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, DdsFrameEncode, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI DdsFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
                                                    void **ppv)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid)) {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    } else if (IsEqualGUID(&IID_IWICDdsFrameDecode, iid)) {
        *ppv = &This->IWICDdsFrameDecode_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        if (This->pixel_data != This->block_data) free(This->pixel_data);
        free(This->block_data);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DdsFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
                                             UINT *puiWidth, UINT *puiHeight)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    if (!puiWidth || !puiHeight) return E_INVALIDARG;

    *puiWidth = This->info.width;
    *puiHeight = This->info.height;

    TRACE("(%p) -> (%d,%d)\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
                                                    WICPixelFormatGUID *pPixelFormat)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    if (!pPixelFormat) return E_INVALIDARG;

    *pPixelFormat = *This->info.pixel_format;

    TRACE("(%p) -> %s\n", iface, debugstr_guid(pPixelFormat));

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
                                                   double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub.\n", iface, pDpiX, pDpiY);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
                                                 IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub.\n", iface, pIPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
                                                const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    UINT bpp, frame_stride, frame_size;
    INT x, y, width, height;
    HRESULT hr;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    if (!pbBuffer) return E_INVALIDARG;

    bpp = This->info.pixel_format_bpp;
    if (!bpp) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    frame_stride = This->info.width * bpp / 8;
    frame_size = frame_stride * This->info.height;
    if (!prc) {
        if (cbStride < frame_stride) return E_INVALIDARG;
        if (cbBufferSize < frame_size) return WINCODEC_ERR_INSUFFICIENTBUFFER;
    } else {
        x = prc->X;
        y = prc->Y;
        width = prc->Width;
        height = prc->Height;
        if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
            x + width > This->info.width ||
            y + height > This->info.height) {
            return E_INVALIDARG;
        }
        if (cbStride < width * bpp / 8) return E_INVALIDARG;
        if (cbBufferSize < cbStride * height) return WINCODEC_ERR_INSUFFICIENTBUFFER;
    }

    EnterCriticalSection(&This->lock);

    if (!This->pixel_data) {
        if (is_compressed(This->info.format)) {
            This->pixel_data = malloc(frame_size);
            if (!This->pixel_data) {
                hr = E_OUTOFMEMORY;
                goto end;
            }
            decode_block(This->block_data, This->info.width_in_blocks * This->info.height_in_blocks, This->info.format,
                         This->info.width, This->info.height, (DWORD *)This->pixel_data);
        } else {
            This->pixel_data = This->block_data;
        }
    }

    hr = copy_pixels(bpp, This->pixel_data, This->info.width, This->info.height, frame_stride,
                     prc, cbStride, cbBufferSize, pbBuffer);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
                                                            IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub.\n", iface, ppIMetadataQueryReader);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
                                                      UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub.\n", iface, cCount, ppIColorContexts, pcActualCount);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
                                                  IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub.\n", iface, ppIThumbnail);

    return E_NOTIMPL;
}

static const IWICBitmapFrameDecodeVtbl DdsFrameDecode_Vtbl = {
    DdsFrameDecode_QueryInterface,
    DdsFrameDecode_AddRef,
    DdsFrameDecode_Release,
    DdsFrameDecode_GetSize,
    DdsFrameDecode_GetPixelFormat,
    DdsFrameDecode_GetResolution,
    DdsFrameDecode_CopyPalette,
    DdsFrameDecode_CopyPixels,
    DdsFrameDecode_GetMetadataQueryReader,
    DdsFrameDecode_GetColorContexts,
    DdsFrameDecode_GetThumbnail
};

static HRESULT WINAPI DdsFrameDecode_Dds_QueryInterface(IWICDdsFrameDecode *iface,
                                                        REFIID iid, void **ppv)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI DdsFrameDecode_Dds_AddRef(IWICDdsFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI DdsFrameDecode_Dds_Release(IWICDdsFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI DdsFrameDecode_Dds_GetSizeInBlocks(IWICDdsFrameDecode *iface,
                                                         UINT *widthInBlocks, UINT *heightInBlocks)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);

    if (!widthInBlocks || !heightInBlocks) return E_INVALIDARG;

    *widthInBlocks = This->info.width_in_blocks;
    *heightInBlocks = This->info.height_in_blocks;

    TRACE("(%p,%p,%p) -> (%d,%d)\n", iface, widthInBlocks, heightInBlocks, *widthInBlocks, *heightInBlocks);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_Dds_GetFormatInfo(IWICDdsFrameDecode *iface,
                                                       WICDdsFormatInfo *formatInfo)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);

    if (!formatInfo) return E_INVALIDARG;

    formatInfo->DxgiFormat = This->info.format;
    formatInfo->BytesPerBlock = This->info.bytes_per_block;
    formatInfo->BlockWidth = This->info.block_width;
    formatInfo->BlockHeight = This->info.block_height;

    TRACE("(%p,%p) -> (0x%x,%d,%d,%d)\n", iface, formatInfo,
          formatInfo->DxgiFormat, formatInfo->BytesPerBlock, formatInfo->BlockWidth, formatInfo->BlockHeight);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_Dds_CopyBlocks(IWICDdsFrameDecode *iface,
                                                    const WICRect *boundsInBlocks, UINT stride, UINT bufferSize,
                                                    BYTE *buffer)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    int x, y, width, height;
    UINT bytes_per_block, frame_stride, frame_size;

    TRACE("(%p,%p,%u,%u,%p)\n", iface, boundsInBlocks, stride, bufferSize, buffer);

    if (!buffer) return E_INVALIDARG;

    bytes_per_block = This->info.bytes_per_block;
    frame_stride = This->info.width_in_blocks * bytes_per_block;
    frame_size = frame_stride * This->info.height_in_blocks;

    if (!boundsInBlocks) {
        if (stride < frame_stride) return E_INVALIDARG;
        if (bufferSize < frame_size) return E_INVALIDARG;
    } else {
        x = boundsInBlocks->X;
        y = boundsInBlocks->Y;
        width = boundsInBlocks->Width;
        height = boundsInBlocks->Height;
        if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
            x + width > This->info.width_in_blocks ||
            y + height > This->info.height_in_blocks) {
            return E_INVALIDARG;
        }
        if (stride < width * bytes_per_block) return E_INVALIDARG;
        if (bufferSize < stride * height) return E_INVALIDARG;
    }

    return copy_pixels(This->info.bytes_per_block * 8, This->block_data, This->info.width_in_blocks,
                       This->info.height_in_blocks, frame_stride, boundsInBlocks, stride, bufferSize, buffer);
}

static const IWICDdsFrameDecodeVtbl DdsFrameDecode_Dds_Vtbl = {
    DdsFrameDecode_Dds_QueryInterface,
    DdsFrameDecode_Dds_AddRef,
    DdsFrameDecode_Dds_Release,
    DdsFrameDecode_Dds_GetSizeInBlocks,
    DdsFrameDecode_Dds_GetFormatInfo,
    DdsFrameDecode_Dds_CopyBlocks
};

static HRESULT DdsFrameDecode_CreateInstance(DdsFrameDecode **frame_decode)
{
    DdsFrameDecode *result;

    result = malloc(sizeof(*result));
    if (!result) return E_OUTOFMEMORY;

    result->IWICBitmapFrameDecode_iface.lpVtbl = &DdsFrameDecode_Vtbl;
    result->IWICDdsFrameDecode_iface.lpVtbl = &DdsFrameDecode_Dds_Vtbl;
    result->ref = 1;
#ifdef __REACTOS__
    InitializeCriticalSection(&result->lock);
#else
    InitializeCriticalSectionEx(&result->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    result->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DdsFrameDecode.lock");

    *frame_decode = result;
    return S_OK;
}

static HRESULT WINAPI DdsDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
                                                void **ppv)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid)) {
        *ppv = &This->IWICBitmapDecoder_iface;
    } else if (IsEqualIID(&IID_IWICDdsDecoder, iid)) {
        *ppv = &This->IWICDdsDecoder_iface;
    } else if (IsEqualIID(&IID_IWICWineDecoder, iid)) {
        *ppv = &This->IWICWineDecoder_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsDecoder_AddRef(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsDecoder_Release(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream) IStream_Release(This->stream);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DdsDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
                                                 DWORD *capability)
{
    FIXME("(%p,%p,%p): stub.\n", iface, stream, capability);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
                                            WICDecodeOptions cacheOptions)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    hr = IWICWineDecoder_Initialize(&This->IWICWineDecoder_iface, pIStream, cacheOptions);
    if (FAILED(hr)) goto end;

    if (This->info.dimension == WICDdsTextureCube ||
        (This->info.format != DXGI_FORMAT_BC1_UNORM &&
         This->info.format != DXGI_FORMAT_BC2_UNORM &&
         This->info.format != DXGI_FORMAT_BC3_UNORM)) {
        IStream_Release(pIStream);
        This->stream = NULL;
        This->initialized = FALSE;
        hr = WINCODEC_ERR_BADHEADER;
    }

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
                                                    GUID *pguidContainerFormat)
{
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);

    memcpy(pguidContainerFormat, &GUID_ContainerFormatDds, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
                                                IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICDdsDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI DdsDecoder_CopyPalette(IWICBitmapDecoder *iface,
                                             IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);

    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI DdsDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
                                                        IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    if (!ppIMetadataQueryReader) return E_INVALIDARG;

    FIXME("(%p,%p)\n", iface, ppIMetadataQueryReader);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_GetPreview(IWICBitmapDecoder *iface,
                                            IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetColorContexts(IWICBitmapDecoder *iface,
                                                  UINT cCount, IWICColorContext **ppDdslorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppDdslorContexts, pcActualCount);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetThumbnail(IWICBitmapDecoder *iface,
                                              IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI DdsDecoder_GetFrameCount(IWICBitmapDecoder *iface,
                                               UINT *pCount)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);

    if (!pCount) return E_INVALIDARG;
    if (!This->initialized) return WINCODEC_ERR_WRONGSTATE;

    EnterCriticalSection(&This->lock);

    *pCount = This->info.frame_count;

    LeaveCriticalSection(&This->lock);

    TRACE("(%p) -> %d\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetFrame(IWICBitmapDecoder *iface,
                                          UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    UINT array_index, mip_level, slice_index;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!ppIBitmapFrame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    get_frame_dds_index(index, &This->info, &array_index, &mip_level, &slice_index);

    LeaveCriticalSection(&This->lock);

    return DdsDecoder_Dds_GetFrame(&This->IWICDdsDecoder_iface, array_index, mip_level, slice_index, ppIBitmapFrame);
}

static const IWICBitmapDecoderVtbl DdsDecoder_Vtbl = {
        DdsDecoder_QueryInterface,
        DdsDecoder_AddRef,
        DdsDecoder_Release,
        DdsDecoder_QueryCapability,
        DdsDecoder_Initialize,
        DdsDecoder_GetContainerFormat,
        DdsDecoder_GetDecoderInfo,
        DdsDecoder_CopyPalette,
        DdsDecoder_GetMetadataQueryReader,
        DdsDecoder_GetPreview,
        DdsDecoder_GetColorContexts,
        DdsDecoder_GetThumbnail,
        DdsDecoder_GetFrameCount,
        DdsDecoder_GetFrame
};

static HRESULT WINAPI DdsDecoder_Dds_QueryInterface(IWICDdsDecoder *iface,
                                                    REFIID iid, void **ppv)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
}

static ULONG WINAPI DdsDecoder_Dds_AddRef(IWICDdsDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI DdsDecoder_Dds_Release(IWICDdsDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI DdsDecoder_Dds_GetParameters(IWICDdsDecoder *iface,
                                                   WICDdsParameters *parameters)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    HRESULT hr;

    if (!parameters) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    parameters->Width = This->info.width;
    parameters->Height = This->info.height;
    parameters->Depth = This->info.depth;
    parameters->MipLevels = This->info.mip_levels;
    parameters->ArraySize = This->info.array_size;
    parameters->DxgiFormat = This->info.format;
    parameters->Dimension = This->info.dimension;
    parameters->AlphaMode = This->info.alpha_mode;

    TRACE("(%p) -> (%dx%d depth=%d mipLevels=%d arraySize=%d dxgiFormat=0x%x dimension=0x%x alphaMode=0x%x)\n",
          iface, parameters->Width, parameters->Height, parameters->Depth, parameters->MipLevels,
          parameters->ArraySize, parameters->DxgiFormat, parameters->Dimension, parameters->AlphaMode);

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsDecoder_Dds_GetFrame(IWICDdsDecoder *iface,
                                              UINT arrayIndex, UINT mipLevel, UINT sliceIndex,
                                              IWICBitmapFrameDecode **bitmapFrame)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    HRESULT hr;
    LARGE_INTEGER seek;
    UINT width, height, depth, block_width, block_height, width_in_blocks, height_in_blocks, size;
    UINT frame_width = 0, frame_height = 0, frame_width_in_blocks = 0, frame_height_in_blocks = 0, frame_size = 0;
    UINT bytes_per_block, i;
    DWORD bytesread;
    DdsFrameDecode *frame_decode = NULL;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, arrayIndex, mipLevel, sliceIndex, bitmapFrame);

    if (!bitmapFrame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    if ((arrayIndex >= This->info.array_size && This->info.dimension != WICDdsTextureCube) ||
        (arrayIndex >= This->info.array_size * 6) ||
        (mipLevel   >= This->info.mip_levels) ||
        (sliceIndex >= This->info.depth)) {
        hr = E_INVALIDARG;
        goto end;
    }

    if (is_compressed(This->info.format)) {
        block_width = DDS_BLOCK_WIDTH;
        block_height = DDS_BLOCK_HEIGHT;
    } else {
        block_width = 1;
        block_height = 1;
    }
    bytes_per_block = This->info.bytes_per_block;
    seek.QuadPart = This->info.data_offset;

    width = This->info.width;
    height = This->info.height;
    depth = This->info.depth;
    for (i = 0; i < This->info.mip_levels; i++)
    {
        width_in_blocks = (width + block_width - 1) / block_width;
        height_in_blocks = (height + block_height - 1) / block_height;
        size = width_in_blocks * height_in_blocks * bytes_per_block;

        if (i < mipLevel)  {
            seek.QuadPart += size * depth;
        } else if (i == mipLevel){
            seek.QuadPart += size * sliceIndex;
            frame_width = width;
            frame_height = height;
            frame_width_in_blocks = width_in_blocks;
            frame_height_in_blocks = height_in_blocks;
            frame_size = frame_width_in_blocks * frame_height_in_blocks * bytes_per_block;
            if (arrayIndex == 0) break;
        }
        seek.QuadPart += arrayIndex * size * depth;

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
        if (depth > 1) depth /= 2;
    }

    hr = DdsFrameDecode_CreateInstance(&frame_decode);
    if (hr != S_OK) goto end;
    frame_decode->info.width = frame_width;
    frame_decode->info.height = frame_height;
    frame_decode->info.format = This->info.format;
    frame_decode->info.bytes_per_block = bytes_per_block;
    frame_decode->info.block_width = block_width;
    frame_decode->info.block_height = block_height;
    frame_decode->info.width_in_blocks = frame_width_in_blocks;
    frame_decode->info.height_in_blocks = frame_height_in_blocks;
    frame_decode->info.pixel_format = This->info.pixel_format;
    frame_decode->info.pixel_format_bpp = This->info.pixel_format_bpp;
    frame_decode->block_data = malloc(frame_size);
    frame_decode->pixel_data = NULL;
    hr = IStream_Seek(This->stream, seek, SEEK_SET, NULL);
    if (hr != S_OK) goto end;
    hr = IStream_Read(This->stream, frame_decode->block_data, frame_size, &bytesread);
    if (hr != S_OK || bytesread != frame_size) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    *bitmapFrame = &frame_decode->IWICBitmapFrameDecode_iface;

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);

    if (hr != S_OK && frame_decode) DdsFrameDecode_Release(&frame_decode->IWICBitmapFrameDecode_iface);

    return hr;
}

static const IWICDdsDecoderVtbl DdsDecoder_Dds_Vtbl = {
    DdsDecoder_Dds_QueryInterface,
    DdsDecoder_Dds_AddRef,
    DdsDecoder_Dds_Release,
    DdsDecoder_Dds_GetParameters,
    DdsDecoder_Dds_GetFrame
};

static HRESULT WINAPI DdsDecoder_Wine_QueryInterface(IWICWineDecoder *iface, REFIID iid, void **ppv)
{
    DdsDecoder *This = impl_from_IWICWineDecoder(iface);
    return DdsDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
}

static ULONG WINAPI DdsDecoder_Wine_AddRef(IWICWineDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICWineDecoder(iface);
    return DdsDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI DdsDecoder_Wine_Release(IWICWineDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICWineDecoder(iface);
    return DdsDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI DdsDecoder_Wine_Initialize(IWICWineDecoder *iface, IStream *stream, WICDecodeOptions options)
{
    DdsDecoder *This = impl_from_IWICWineDecoder(iface);
    DDS_HEADER_DXT10 header_dxt10;
    LARGE_INTEGER seek;
    DDS_HEADER header;
    ULONG bytesread;
    DWORD magic;
    HRESULT hr;

    TRACE("(This %p, stream %p, options %#x)\n", iface, stream, options);

    EnterCriticalSection(&This->lock);

    if (This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(stream, &magic, sizeof(magic), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(magic)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (magic != DDS_MAGIC) {
        hr = WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
        goto end;
    }

    hr = IStream_Read(stream, &header, sizeof(header), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(header)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (header.size != sizeof(header)) {
        hr = WINCODEC_ERR_BADHEADER;
        goto end;
    }

    if (has_extended_header(&header)) {
        hr = IStream_Read(stream, &header_dxt10, sizeof(header_dxt10), &bytesread);
        if (FAILED(hr)) goto end;
        if (bytesread != sizeof(header_dxt10)) {
            hr = WINCODEC_ERR_STREAMREAD;
            goto end;
        }
    }

    get_dds_info(&This->info, &header, &header_dxt10);

    This->initialized = TRUE;
    This->stream = stream;
    IStream_AddRef(stream);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static const IWICWineDecoderVtbl DdsDecoder_Wine_Vtbl = {
    DdsDecoder_Wine_QueryInterface,
    DdsDecoder_Wine_AddRef,
    DdsDecoder_Wine_Release,
    DdsDecoder_Wine_Initialize
};

static HRESULT WINAPI DdsFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid, void **ppv)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->IWICBitmapFrameEncode_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapEncoder_Release(&This->parent->IWICBitmapEncoder_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DdsFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
                                                IPropertyBag2 *encoderOptions)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, encoderOptions);
    if (encoderOptions) FIXME("encoder options are not supported for DDS.\n");

    EnterCriticalSection(&This->parent->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        This->initialized = TRUE;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI DdsFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
                                             UINT width, UINT height)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%u,%u)\n", iface, width, height);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        This->width = width;
        This->height = height;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI DdsFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
                                                   double dpiX, double dpiY)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        This->dpi_x = dpiX;
        This->dpi_y = dpiY;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI DdsFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
                                                    WICPixelFormatGUID *pixelFormat)
{
    DdsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pixelFormat));

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized)
    {
        hr = WINCODEC_ERR_NOTINITIALIZED;
    }
    else if (This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        *pixelFormat = GUID_WICPixelFormat32bppBGRA;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI DdsFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
                                                      UINT count, IWICColorContext **colorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, count, colorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
                                                IWICPalette *palette)
{
    FIXME("(%p,%p): stub\n", iface, palette);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
                                                  IWICBitmapSource *thumbnail)
{
    TRACE("(%p,%p)\n", iface, thumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
                                                 UINT lineCount, UINT stride, UINT bufferSize, BYTE *pixels)
{
    FIXME("(%p,%u,%u,%u,%p): stub\n", iface, lineCount, stride, bufferSize, pixels);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
                                                 IWICBitmapSource *bitmapSource, WICRect *rc)
{
    FIXME("(%p,%p,%s): stub\n", iface, bitmapSource, debug_wic_rect(rc));
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
                                                            IWICMetadataQueryWriter **metadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, metadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl DdsFrameEncode_Vtbl = {
    DdsFrameEncode_QueryInterface,
    DdsFrameEncode_AddRef,
    DdsFrameEncode_Release,
    DdsFrameEncode_Initialize,
    DdsFrameEncode_SetSize,
    DdsFrameEncode_SetResolution,
    DdsFrameEncode_SetPixelFormat,
    DdsFrameEncode_SetColorContexts,
    DdsFrameEncode_SetPalette,
    DdsFrameEncode_SetThumbnail,
    DdsFrameEncode_WritePixels,
    DdsFrameEncode_WriteSource,
    DdsFrameEncode_Commit,
    DdsFrameEncode_GetMetadataQueryWriter
};

HRESULT DdsDecoder_CreateInstance(REFIID iid, void** ppv)
{
    DdsDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(DdsDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &DdsDecoder_Vtbl;
    This->IWICDdsDecoder_iface.lpVtbl = &DdsDecoder_Dds_Vtbl;
    This->IWICWineDecoder_iface.lpVtbl = &DdsDecoder_Wine_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DdsDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}

static HRESULT WINAPI DdsEncoder_Dds_QueryInterface(IWICDdsEncoder *iface, REFIID iid,
                                                    void **ppv)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    return IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
}

static ULONG WINAPI DdsEncoder_Dds_AddRef(IWICDdsEncoder *iface)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    return IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);
}

static ULONG WINAPI DdsEncoder_Dds_Release(IWICDdsEncoder *iface)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    return IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);
}

static HRESULT WINAPI DdsEncoder_Dds_SetParameters(IWICDdsEncoder *iface,
                                                   WICDdsParameters *parameters)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, parameters);

    if (!parameters) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->stream)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    This->info.width      = parameters->Width;
    This->info.height     = parameters->Height;
    This->info.depth      = parameters->Depth;
    This->info.mip_levels = parameters->MipLevels;
    This->info.array_size = parameters->ArraySize;
    This->info.format     = parameters->DxgiFormat;
    This->info.dimension  = parameters->Dimension;
    This->info.alpha_mode = parameters->AlphaMode;

    This->info.bytes_per_block = get_bytes_per_block_from_format(This->info.format);
    This->info.frame_count = get_frame_count(This->info.depth, This->info.mip_levels,
                                             This->info.array_size, This->info.dimension);

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI DdsEncoder_Dds_GetParameters(IWICDdsEncoder *iface,
                                                   WICDdsParameters *parameters)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, parameters);

    if (!parameters) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->stream)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    parameters->Width      = This->info.width;
    parameters->Height     = This->info.height;
    parameters->Depth      = This->info.depth;
    parameters->MipLevels  = This->info.mip_levels;
    parameters->ArraySize  = This->info.array_size;
    parameters->DxgiFormat = This->info.format;
    parameters->Dimension  = This->info.dimension;
    parameters->AlphaMode  = This->info.alpha_mode;

    TRACE("(%p,%p) -> (%dx%d depth=%u mipLevels=%u arraySize=%u dxgiFormat=%#x dimension=%#x alphaMode=%#x)\n",
          iface, parameters, parameters->Width, parameters->Height, parameters->Depth, parameters->MipLevels,
          parameters->ArraySize, parameters->DxgiFormat, parameters->Dimension, parameters->AlphaMode);

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI DdsEncoder_Dds_CreateNewFrame(IWICDdsEncoder *iface,
                                                    IWICBitmapFrameEncode **frameEncode,
                                                    UINT *arrayIndex, UINT *mipLevel, UINT *sliceIndex)
{
    DdsEncoder *This = impl_from_IWICDdsEncoder(iface);
    UINT array_index, mip_level, slice_index;
    DdsFrameEncode *result;
    HRESULT hr;

    TRACE("(%p,%p,%p,%p,%p)\n", iface, frameEncode, arrayIndex, mipLevel, sliceIndex);

    EnterCriticalSection(&This->lock);

    if (!This->stream || This->committed || This->uncommitted_frame)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    result = malloc(sizeof(*result));
    if (!result)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    get_frame_dds_index(This->frame_index, &This->info, &array_index, &mip_level, &slice_index);
    if (arrayIndex) *arrayIndex = array_index;
    if (mipLevel)   *mipLevel   = mip_level;
    if (sliceIndex) *sliceIndex = slice_index;

    This->frame_index++;
    result->IWICBitmapFrameEncode_iface.lpVtbl = &DdsFrameEncode_Vtbl;
    result->ref = 1;
    result->parent = This;
    result->parent->uncommitted_frame = TRUE;
    result->initialized = FALSE;
    result->frame_created = FALSE;
    IWICDdsEncoder_AddRef(iface);

    *frameEncode = &result->IWICBitmapFrameEncode_iface;
    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static const IWICDdsEncoderVtbl DdsEncoder_Dds_Vtbl =
{
    DdsEncoder_Dds_QueryInterface,
    DdsEncoder_Dds_AddRef,
    DdsEncoder_Dds_Release,
    DdsEncoder_Dds_SetParameters,
    DdsEncoder_Dds_GetParameters,
    DdsEncoder_Dds_CreateNewFrame
};

static HRESULT WINAPI DdsEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
                                                   void **ppv)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    FIXME("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid)) {
        *ppv = &This->IWICBitmapEncoder_iface;
    } else if (IsEqualIID(&IID_IWICDdsEncoder, iid)) {
        *ppv = &This->IWICDdsEncoder_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsEncoder_AddRef(IWICBitmapEncoder *iface)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsEncoder_Release(IWICBitmapEncoder *iface)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0) {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream) IStream_Release(This->stream);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DdsEncoder_Initialize(IWICBitmapEncoder *iface,
                                            IStream *stream, WICBitmapEncoderCacheOption cacheOption)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p,%u)\n", iface, stream, cacheOption);

    if (cacheOption != WICBitmapEncoderNoCache)
        FIXME("Cache option %#x is not supported.\n", cacheOption);

    if (!stream) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->stream)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    This->stream = stream;
    IStream_AddRef(stream);

    This->info.width = 1;
    This->info.height = 1;
    This->info.depth = 1;
    This->info.mip_levels = 1;
    This->info.array_size = 1;
    This->info.frame_count = 1;
    This->info.data_offset = 0;
    This->info.bytes_per_block = get_bytes_per_block_from_format(DXGI_FORMAT_BC3_UNORM);
    This->info.format = DXGI_FORMAT_BC3_UNORM;
    This->info.dimension = WICDdsTexture2D;
    This->info.alpha_mode = WICDdsAlphaModeUnknown;
    This->info.pixel_format = &GUID_WICPixelFormatUndefined;
    This->info.pixel_format_bpp = 0;

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    TRACE("(%p,%p)\n", iface, format);

    if (!format)
        return E_INVALIDARG;

    memcpy(format, &GUID_ContainerFormatDds, sizeof(*format));
    return S_OK;
}

static HRESULT WINAPI DdsEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICDdsEncoder, &comp_info);
    if (hr == S_OK) {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI DdsEncoder_SetColorContexts(IWICBitmapEncoder *iface,
                                                     UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    EnterCriticalSection(&This->lock);

    hr = This->stream ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
                                                IWICBitmapFrameEncode **frameEncode, IPropertyBag2 **encoderOptions)
{
    DdsEncoder *This = impl_from_IWICBitmapEncoder(iface);

    TRACE("(%p,%p,%p)\n", iface, frameEncode, encoderOptions);

    return IWICDdsEncoder_CreateNewFrame(&This->IWICDdsEncoder_iface, frameEncode, NULL, NULL, NULL);
}

static HRESULT WINAPI DdsEncoder_Commit(IWICBitmapEncoder *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI DdsEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
                                                           IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl DdsEncoder_Vtbl = {
    DdsEncoder_QueryInterface,
    DdsEncoder_AddRef,
    DdsEncoder_Release,
    DdsEncoder_Initialize,
    DdsEncoder_GetContainerFormat,
    DdsEncoder_GetEncoderInfo,
    DdsEncoder_SetColorContexts,
    DdsEncoder_SetPalette,
    DdsEncoder_SetThumbnail,
    DdsEncoder_SetPreview,
    DdsEncoder_CreateNewFrame,
    DdsEncoder_Commit,
    DdsEncoder_GetMetadataQueryWriter
};

HRESULT DdsEncoder_CreateInstance( REFIID iid, void **ppv)
{
    DdsEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(DdsEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &DdsEncoder_Vtbl;
    This->IWICDdsEncoder_iface.lpVtbl = &DdsEncoder_Dds_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->frame_count = 0;
    This->frame_index = 0;
    This->uncommitted_frame = FALSE;
    This->committed = FALSE;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DdsEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}
