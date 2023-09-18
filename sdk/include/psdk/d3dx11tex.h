/*
 * Copyright 2016 Andrey Gusev
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

#include "d3dx11.h"

#ifndef __D3DX11TEX_H__
#define __D3DX11TEX_H__

typedef enum D3DX11_FILTER_FLAG
{
    D3DX11_FILTER_NONE             = 0x00000001,
    D3DX11_FILTER_POINT            = 0x00000002,
    D3DX11_FILTER_LINEAR           = 0x00000003,
    D3DX11_FILTER_TRIANGLE         = 0x00000004,
    D3DX11_FILTER_BOX              = 0x00000005,

    D3DX11_FILTER_MIRROR_U         = 0x00010000,
    D3DX11_FILTER_MIRROR_V         = 0x00020000,
    D3DX11_FILTER_MIRROR_W         = 0x00040000,
    D3DX11_FILTER_MIRROR           = 0x00070000,

    D3DX11_FILTER_DITHER           = 0x00080000,
    D3DX11_FILTER_DITHER_DIFFUSION = 0x00100000,

    D3DX11_FILTER_SRGB_IN          = 0x00200000,
    D3DX11_FILTER_SRGB_OUT         = 0x00400000,
    D3DX11_FILTER_SRGB             = 0x00600000,
} D3DX11_FILTER_FLAG;

typedef enum D3DX11_IMAGE_FILE_FORMAT
{
    D3DX11_IFF_BMP         = 0,
    D3DX11_IFF_JPG         = 1,
    D3DX11_IFF_PNG         = 3,
    D3DX11_IFF_DDS         = 4,
    D3DX11_IFF_TIFF        = 10,
    D3DX11_IFF_GIF         = 11,
    D3DX11_IFF_WMP         = 12,
    D3DX11_IFF_FORCE_DWORD = 0x7fffffff
} D3DX11_IMAGE_FILE_FORMAT;

typedef struct D3DX11_IMAGE_INFO
{
    UINT                     Width;
    UINT                     Height;
    UINT                     Depth;
    UINT                     ArraySize;
    UINT                     MipLevels;
    UINT                     MiscFlags;
    DXGI_FORMAT              Format;
    D3D11_RESOURCE_DIMENSION ResourceDimension;
    D3DX11_IMAGE_FILE_FORMAT ImageFileFormat;
} D3DX11_IMAGE_INFO;

typedef struct D3DX11_IMAGE_LOAD_INFO
{
    UINT              Width;
    UINT              Height;
    UINT              Depth;
    UINT              FirstMipLevel;
    UINT              MipLevels;
    D3D11_USAGE       Usage;
    UINT              BindFlags;
    UINT              CpuAccessFlags;
    UINT              MiscFlags;
    DXGI_FORMAT       Format;
    UINT              Filter;
    UINT              MipFilter;
    D3DX11_IMAGE_INFO *pSrcInfo;

#ifdef __cplusplus
    D3DX11_IMAGE_LOAD_INFO()
    {
        Width          = D3DX11_DEFAULT;
        Height         = D3DX11_DEFAULT;
        Depth          = D3DX11_DEFAULT;
        FirstMipLevel  = D3DX11_DEFAULT;
        MipLevels      = D3DX11_DEFAULT;
        Usage          = (D3D11_USAGE)D3DX11_DEFAULT;
        BindFlags      = D3DX11_DEFAULT;
        CpuAccessFlags = D3DX11_DEFAULT;
        MiscFlags      = D3DX11_DEFAULT;
        Format         = DXGI_FORMAT_FROM_FILE;
        Filter         = D3DX11_DEFAULT;
        MipFilter      = D3DX11_DEFAULT;
        pSrcInfo       = NULL;
    }
#endif
} D3DX11_IMAGE_LOAD_INFO;

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DX11CreateShaderResourceViewFromMemory(ID3D11Device *device, const void *data,
        SIZE_T data_size, D3DX11_IMAGE_LOAD_INFO *load_info, ID3DX11ThreadPump *pump,
        ID3D11ShaderResourceView **view, HRESULT *hresult);

HRESULT WINAPI D3DX11CreateTextureFromFileA(ID3D11Device *device, const char *filename,
        D3DX11_IMAGE_LOAD_INFO *load_info, ID3DX11ThreadPump *pump, ID3D11Resource **texture,
        HRESULT *hresult);

HRESULT WINAPI D3DX11CreateTextureFromFileW(ID3D11Device *device, const WCHAR *filename,
        D3DX11_IMAGE_LOAD_INFO *load_info, ID3DX11ThreadPump *pump, ID3D11Resource **texture,
        HRESULT *hresult);

HRESULT WINAPI D3DX11CreateTextureFromMemory(ID3D11Device *device, const void *src_data, SIZE_T src_data_size,
        D3DX11_IMAGE_LOAD_INFO *loadinfo, ID3DX11ThreadPump *pump, ID3D11Resource **texture, HRESULT *hresult);

HRESULT WINAPI D3DX11FilterTexture(ID3D11DeviceContext *context, ID3D11Resource *texture, UINT src_level, UINT filter);

HRESULT WINAPI D3DX11GetImageInfoFromMemory(const void *src_data, SIZE_T src_data_size, ID3DX11ThreadPump *pump,
        D3DX11_IMAGE_INFO *img_info, HRESULT *hresult);

HRESULT WINAPI D3DX11SaveTextureToMemory(ID3D11DeviceContext *context, ID3D11Resource *texture,
        D3DX11_IMAGE_FILE_FORMAT format, ID3D10Blob **buffer, UINT flags);

#ifdef __cplusplus
}
#endif

#endif
