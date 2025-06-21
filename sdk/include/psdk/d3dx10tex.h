/*
 * Copyright 2016 Alistair Leslie-Hughes
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

#include "d3dx10.h"

#ifndef __D3DX10TEX_H__
#define __D3DX10TEX_H__

typedef enum D3DX10_FILTER_FLAG
{
    D3DX10_FILTER_NONE             = 0x00000001,
    D3DX10_FILTER_POINT            = 0x00000002,
    D3DX10_FILTER_LINEAR           = 0x00000003,
    D3DX10_FILTER_TRIANGLE         = 0x00000004,
    D3DX10_FILTER_BOX              = 0x00000005,

    D3DX10_FILTER_MIRROR_U         = 0x00010000,
    D3DX10_FILTER_MIRROR_V         = 0x00020000,
    D3DX10_FILTER_MIRROR_W         = 0x00040000,
    D3DX10_FILTER_MIRROR           = 0x00070000,

    D3DX10_FILTER_DITHER           = 0x00080000,
    D3DX10_FILTER_DITHER_DIFFUSION = 0x00100000,

    D3DX10_FILTER_SRGB_IN          = 0x00200000,
    D3DX10_FILTER_SRGB_OUT         = 0x00400000,
    D3DX10_FILTER_SRGB             = 0x00600000,
} D3DX10_FILTER_FLAG;

typedef enum D3DX10_IMAGE_FILE_FORMAT
{
    D3DX10_IFF_BMP         = 0,
    D3DX10_IFF_JPG         = 1,
    D3DX10_IFF_PNG         = 3,
    D3DX10_IFF_DDS         = 4,
    D3DX10_IFF_TIFF        = 10,
    D3DX10_IFF_GIF         = 11,
    D3DX10_IFF_WMP         = 12,
    D3DX10_IFF_FORCE_DWORD = 0x7fffffff
} D3DX10_IMAGE_FILE_FORMAT;

typedef struct D3DX10_IMAGE_INFO
{
    UINT                     Width;
    UINT                     Height;
    UINT                     Depth;
    UINT                     ArraySize;
    UINT                     MipLevels;
    UINT                     MiscFlags;
    DXGI_FORMAT              Format;
    D3D10_RESOURCE_DIMENSION ResourceDimension;
    D3DX10_IMAGE_FILE_FORMAT ImageFileFormat;
} D3DX10_IMAGE_INFO;

typedef struct D3DX10_IMAGE_LOAD_INFO
{
    UINT              Width;
    UINT              Height;
    UINT              Depth;
    UINT              FirstMipLevel;
    UINT              MipLevels;
    D3D10_USAGE       Usage;
    UINT              BindFlags;
    UINT              CpuAccessFlags;
    UINT              MiscFlags;
    DXGI_FORMAT       Format;
    UINT              Filter;
    UINT              MipFilter;
    D3DX10_IMAGE_INFO *pSrcInfo;

#ifdef __cplusplus
    D3DX10_IMAGE_LOAD_INFO()
    {
        Width          = D3DX10_DEFAULT;
        Height         = D3DX10_DEFAULT;
        Depth          = D3DX10_DEFAULT;
        FirstMipLevel  = D3DX10_DEFAULT;
        MipLevels      = D3DX10_DEFAULT;
        Usage          = (D3D10_USAGE)D3DX10_DEFAULT;
        BindFlags      = D3DX10_DEFAULT;
        CpuAccessFlags = D3DX10_DEFAULT;
        MiscFlags      = D3DX10_DEFAULT;
        Format         = DXGI_FORMAT_FROM_FILE;
        Filter         = D3DX10_DEFAULT;
        MipFilter      = D3DX10_DEFAULT;
        pSrcInfo       = NULL;
    }
#endif
} D3DX10_IMAGE_LOAD_INFO;

typedef struct _D3DX10_TEXTURE_LOAD_INFO
{
    D3D10_BOX *pSrcBox;
    D3D10_BOX *pDstBox;
    UINT SrcFirstMip;
    UINT DstFirstMip;
    UINT NumMips;
    UINT SrcFirstElement;
    UINT DstFirstElement;
    UINT NumElements;
    UINT Filter;
    UINT MipFilter;

#ifdef __cplusplus
    _D3DX10_TEXTURE_LOAD_INFO()
    {
        pSrcBox = NULL;
        pDstBox = NULL;
        SrcFirstMip = 0;
        DstFirstMip = 0;
        NumMips = D3DX10_DEFAULT;
        SrcFirstElement = 0;
        DstFirstElement = 0;
        NumElements = D3DX10_DEFAULT;
        Filter = D3DX10_DEFAULT;
        MipFilter = D3DX10_DEFAULT;
    }
#endif
} D3DX10_TEXTURE_LOAD_INFO;

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DX10CreateTextureFromMemory(ID3D10Device *device, const void *src_data, SIZE_T src_data_size,
        D3DX10_IMAGE_LOAD_INFO *loadinfo, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult);
HRESULT WINAPI D3DX10FilterTexture(ID3D10Resource *texture, UINT src_level, UINT filter);

HRESULT WINAPI D3DX10GetImageInfoFromFileA(const char *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result);
HRESULT WINAPI D3DX10GetImageInfoFromFileW(const WCHAR *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result);
#define        D3DX10GetImageInfoFromFile WINELIB_NAME_AW(D3DX10GetImageInfoFromFile)

HRESULT WINAPI D3DX10GetImageInfoFromResourceA(HMODULE module, const char *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result);
HRESULT WINAPI D3DX10GetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result);
#define        D3DX10GetImageInfoFromResource WINELIB_NAME_AW(D3DX10GetImageInfoFromResource)

HRESULT WINAPI D3DX10GetImageInfoFromMemory(const void *src_data, SIZE_T src_data_size, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *img_info, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateTextureFromFileA(ID3D10Device *device, const char *src_file,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult);
HRESULT WINAPI D3DX10CreateTextureFromFileW(ID3D10Device *device, const WCHAR *src_file,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult);
#define        D3DX10CreateTextureFromFile WINELIB_NAME_AW(D3DX10CreateTextureFromFile)

HRESULT WINAPI D3DX10CreateTextureFromResourceA(ID3D10Device *device, HMODULE module, const char *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult);
HRESULT WINAPI D3DX10CreateTextureFromResourceW(ID3D10Device *device, HMODULE module, const WCHAR *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult);
#define        D3DX10CreateTextureFromResource WINELIB_NAME_AW(D3DX10CreateTextureFromResource)

HRESULT WINAPI D3DX10LoadTextureFromTexture(ID3D10Resource *src_texture, D3DX10_TEXTURE_LOAD_INFO *load_info,
        ID3D10Resource *dst_texture);

#ifdef __cplusplus
}
#endif

#endif
