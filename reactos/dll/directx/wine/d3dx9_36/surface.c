/*
 * Copyright (C) 2009-2010 Tony Wasserka
 * Copyright (C) 2012 Józef Kucia
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

#include "d3dx9_36_private.h"

#include <ole2.h>
#include <wine/wined3d.h>

#include <initguid.h>
#include <wincodec.h>

/* Wine-specific WIC GUIDs */
DEFINE_GUID(GUID_WineContainerFormatTga, 0x0c44fda1,0xa5c5,0x4298,0x96,0x85,0x47,0x3f,0xc1,0x7c,0xd3,0x22);

static const struct
{
    const GUID *wic_guid;
    D3DFORMAT d3dformat;
} wic_pixel_formats[] = {
    { &GUID_WICPixelFormat8bppIndexed, D3DFMT_P8 },
    { &GUID_WICPixelFormat1bppIndexed, D3DFMT_P8 },
    { &GUID_WICPixelFormat4bppIndexed, D3DFMT_P8 },
    { &GUID_WICPixelFormat8bppGray, D3DFMT_L8 },
    { &GUID_WICPixelFormat16bppBGR555, D3DFMT_X1R5G5B5 },
    { &GUID_WICPixelFormat16bppBGR565, D3DFMT_R5G6B5 },
    { &GUID_WICPixelFormat24bppBGR, D3DFMT_R8G8B8 },
    { &GUID_WICPixelFormat32bppBGR, D3DFMT_X8R8G8B8 },
    { &GUID_WICPixelFormat32bppBGRA, D3DFMT_A8R8G8B8 }
};

static D3DFORMAT wic_guid_to_d3dformat(const GUID *guid)
{
    unsigned int i;

    for (i = 0; i < sizeof(wic_pixel_formats) / sizeof(wic_pixel_formats[0]); i++)
    {
        if (IsEqualGUID(wic_pixel_formats[i].wic_guid, guid))
            return wic_pixel_formats[i].d3dformat;
    }

    return D3DFMT_UNKNOWN;
}

static const GUID *d3dformat_to_wic_guid(D3DFORMAT format)
{
    unsigned int i;

    for (i = 0; i < sizeof(wic_pixel_formats) / sizeof(wic_pixel_formats[0]); i++)
    {
        if (wic_pixel_formats[i].d3dformat == format)
            return wic_pixel_formats[i].wic_guid;
    }

    return NULL;
}

/* dds_header.flags */
#define DDS_CAPS 0x1
#define DDS_HEIGHT 0x2
#define DDS_WIDTH 0x4
#define DDS_PITCH 0x8
#define DDS_PIXELFORMAT 0x1000
#define DDS_MIPMAPCOUNT 0x20000
#define DDS_LINEARSIZE 0x80000
#define DDS_DEPTH 0x800000

/* dds_header.caps */
#define DDS_CAPS_COMPLEX 0x8
#define DDS_CAPS_TEXTURE 0x1000
#define DDS_CAPS_MIPMAP 0x400000

/* dds_header.caps2 */
#define DDS_CAPS2_CUBEMAP 0x200
#define DDS_CAPS2_CUBEMAP_POSITIVEX 0x400
#define DDS_CAPS2_CUBEMAP_NEGATIVEX 0x800
#define DDS_CAPS2_CUBEMAP_POSITIVEY 0x1000
#define DDS_CAPS2_CUBEMAP_NEGATIVEY 0x2000
#define DDS_CAPS2_CUBEMAP_POSITIVEZ 0x4000
#define DDS_CAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define DDS_CAPS2_CUBEMAP_ALL_FACES ( DDS_CAPS2_CUBEMAP_POSITIVEX | DDS_CAPS2_CUBEMAP_NEGATIVEX \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEY | DDS_CAPS2_CUBEMAP_NEGATIVEY \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEZ | DDS_CAPS2_CUBEMAP_NEGATIVEZ )
#define DDS_CAPS2_VOLUME 0x200000

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x1
#define DDS_PF_ALPHA_ONLY 0x2
#define DDS_PF_FOURCC 0x4
#define DDS_PF_RGB 0x40
#define DDS_PF_YUV 0x200
#define DDS_PF_LUMINANCE 0x20000
#define DDS_PF_BUMPDUDV 0x80000

struct dds_pixel_format
{
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

struct dds_header
{
    DWORD signature;
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitch_or_linear_size;
    DWORD depth;
    DWORD miplevels;
    DWORD reserved[11];
    struct dds_pixel_format pixel_format;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
};

static D3DFORMAT dds_fourcc_to_d3dformat(DWORD fourcc)
{
    unsigned int i;
    static const DWORD known_fourcc[] = {
        MAKEFOURCC('U','Y','V','Y'),
        MAKEFOURCC('Y','U','Y','2'),
        MAKEFOURCC('R','G','B','G'),
        MAKEFOURCC('G','R','G','B'),
        MAKEFOURCC('D','X','T','1'),
        MAKEFOURCC('D','X','T','2'),
        MAKEFOURCC('D','X','T','3'),
        MAKEFOURCC('D','X','T','4'),
        MAKEFOURCC('D','X','T','5'),
        D3DFMT_R16F,
        D3DFMT_G16R16F,
        D3DFMT_A16B16G16R16F,
        D3DFMT_R32F,
        D3DFMT_G32R32F,
        D3DFMT_A32B32G32R32F,
    };

    for (i = 0; i < sizeof(known_fourcc) / sizeof(known_fourcc[0]); i++)
    {
        if (known_fourcc[i] == fourcc)
            return fourcc;
    }

    WARN("Unknown FourCC %#x\n", fourcc);
    return D3DFMT_UNKNOWN;
}

static const struct {
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
    D3DFORMAT format;
} rgb_pixel_formats[] = {
    { 8, 0xe0, 0x1c, 0x03, 0, D3DFMT_R3G3B2 },
    { 16, 0xf800, 0x07e0, 0x001f, 0x0000, D3DFMT_R5G6B5 },
    { 16, 0x7c00, 0x03e0, 0x001f, 0x8000, D3DFMT_A1R5G5B5 },
    { 16, 0x7c00, 0x03e0, 0x001f, 0x0000, D3DFMT_X1R5G5B5 },
    { 16, 0x0f00, 0x00f0, 0x000f, 0xf000, D3DFMT_A4R4G4B4 },
    { 16, 0x0f00, 0x00f0, 0x000f, 0x0000, D3DFMT_X4R4G4B4 },
    { 16, 0x00e0, 0x001c, 0x0003, 0xff00, D3DFMT_A8R3G3B2 },
    { 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000, D3DFMT_R8G8B8 },
    { 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, D3DFMT_A8R8G8B8 },
    { 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, D3DFMT_X8R8G8B8 },
    { 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, D3DFMT_A2B10G10R10 },
    { 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, D3DFMT_A2R10G10B10 },
    { 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000, D3DFMT_G16R16 },
    { 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, D3DFMT_A8B8G8R8 },
    { 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000, D3DFMT_X8B8G8R8 },
};

static D3DFORMAT dds_rgb_to_d3dformat(const struct dds_pixel_format *pixel_format)
{
    unsigned int i;

    for (i = 0; i < sizeof(rgb_pixel_formats) / sizeof(rgb_pixel_formats[0]); i++)
    {
        if (rgb_pixel_formats[i].bpp == pixel_format->bpp
            && rgb_pixel_formats[i].rmask == pixel_format->rmask
            && rgb_pixel_formats[i].gmask == pixel_format->gmask
            && rgb_pixel_formats[i].bmask == pixel_format->bmask)
        {
            if ((pixel_format->flags & DDS_PF_ALPHA) && rgb_pixel_formats[i].amask == pixel_format->amask)
                return rgb_pixel_formats[i].format;
            if (rgb_pixel_formats[i].amask == 0)
                return rgb_pixel_formats[i].format;
        }
    }

    WARN("Unknown RGB pixel format (%#x, %#x, %#x, %#x)\n",
        pixel_format->rmask, pixel_format->gmask, pixel_format->bmask, pixel_format->amask);
    return D3DFMT_UNKNOWN;
}

static D3DFORMAT dds_luminance_to_d3dformat(const struct dds_pixel_format *pixel_format)
{
    if (pixel_format->bpp == 8)
    {
        if (pixel_format->rmask == 0xff)
            return D3DFMT_L8;
        if ((pixel_format->flags & DDS_PF_ALPHA) && pixel_format->rmask == 0x0f && pixel_format->amask == 0xf0)
            return D3DFMT_A4L4;
    }
    if (pixel_format->bpp == 16)
    {
        if (pixel_format->rmask == 0xffff)
            return D3DFMT_L16;
        if ((pixel_format->flags & DDS_PF_ALPHA) && pixel_format->rmask == 0x00ff && pixel_format->amask == 0xff00)
            return D3DFMT_A8L8;
    }

    WARN("Unknown luminance pixel format (bpp %u, l %#x, a %#x)\n",
        pixel_format->bpp, pixel_format->rmask, pixel_format->amask);
    return D3DFMT_UNKNOWN;
}

static D3DFORMAT dds_alpha_to_d3dformat(const struct dds_pixel_format *pixel_format)
{
    if (pixel_format->bpp == 8 && pixel_format->amask == 0xff)
        return D3DFMT_A8;

    WARN("Unknown Alpha pixel format (%u, %#x)\n", pixel_format->bpp, pixel_format->rmask);
    return D3DFMT_UNKNOWN;
}

static D3DFORMAT dds_bump_to_d3dformat(const struct dds_pixel_format *pixel_format)
{
    if (pixel_format->bpp == 16 && pixel_format->rmask == 0x00ff && pixel_format->gmask == 0xff00)
        return D3DFMT_V8U8;
    if (pixel_format->bpp == 32 && pixel_format->rmask == 0x0000ffff && pixel_format->gmask == 0xffff0000)
        return D3DFMT_V16U16;

    WARN("Unknown bump pixel format (%u, %#x, %#x, %#x, %#x)\n", pixel_format->bpp,
        pixel_format->rmask, pixel_format->gmask, pixel_format->bmask, pixel_format->amask);
    return D3DFMT_UNKNOWN;
}

static D3DFORMAT dds_pixel_format_to_d3dformat(const struct dds_pixel_format *pixel_format)
{
    TRACE("pixel_format: size %u, flags %#x, fourcc %#x, bpp %u.\n", pixel_format->size,
            pixel_format->flags, pixel_format->fourcc, pixel_format->bpp);
    TRACE("rmask %#x, gmask %#x, bmask %#x, amask %#x.\n", pixel_format->rmask, pixel_format->gmask,
            pixel_format->bmask, pixel_format->amask);

    if (pixel_format->flags & DDS_PF_FOURCC)
        return dds_fourcc_to_d3dformat(pixel_format->fourcc);
    if (pixel_format->flags & DDS_PF_RGB)
        return dds_rgb_to_d3dformat(pixel_format);
    if (pixel_format->flags & DDS_PF_LUMINANCE)
        return dds_luminance_to_d3dformat(pixel_format);
    if (pixel_format->flags & DDS_PF_ALPHA_ONLY)
        return dds_alpha_to_d3dformat(pixel_format);
    if (pixel_format->flags & DDS_PF_BUMPDUDV)
        return dds_bump_to_d3dformat(pixel_format);

    WARN("Unknown pixel format (flags %#x, fourcc %#x, bpp %u, r %#x, g %#x, b %#x, a %#x)\n",
        pixel_format->flags, pixel_format->fourcc, pixel_format->bpp,
        pixel_format->rmask, pixel_format->gmask, pixel_format->bmask, pixel_format->amask);
    return D3DFMT_UNKNOWN;
}

static HRESULT d3dformat_to_dds_pixel_format(struct dds_pixel_format *pixel_format, D3DFORMAT d3dformat)
{
    unsigned int i;

    memset(pixel_format, 0, sizeof(*pixel_format));

    pixel_format->size = sizeof(*pixel_format);

    for (i = 0; i < sizeof(rgb_pixel_formats) / sizeof(rgb_pixel_formats[0]); i++)
    {
        if (rgb_pixel_formats[i].format == d3dformat)
        {
            pixel_format->flags |= DDS_PF_RGB;
            pixel_format->bpp = rgb_pixel_formats[i].bpp;
            pixel_format->rmask = rgb_pixel_formats[i].rmask;
            pixel_format->gmask = rgb_pixel_formats[i].gmask;
            pixel_format->bmask = rgb_pixel_formats[i].bmask;
            pixel_format->amask = rgb_pixel_formats[i].amask;
            if (pixel_format->amask) pixel_format->flags |= DDS_PF_ALPHA;
            return D3D_OK;
        }
    }

    /* Reuse dds_fourcc_to_d3dformat as D3DFORMAT and FOURCC are DWORD with same values */
    if (dds_fourcc_to_d3dformat(d3dformat) != D3DFMT_UNKNOWN)
    {
        pixel_format->flags |= DDS_PF_FOURCC;
        pixel_format->fourcc = d3dformat;
        return D3D_OK;
    }

    WARN("Unknown pixel format %#x\n", d3dformat);
    return E_NOTIMPL;
}

static HRESULT calculate_dds_surface_size(D3DFORMAT format, UINT width, UINT height,
    UINT *pitch, UINT *size)
{
    const struct pixel_format_desc *format_desc = get_format_info(format);
    if (format_desc->type == FORMAT_UNKNOWN)
        return E_NOTIMPL;

    if (format_desc->block_width != 1 || format_desc->block_height != 1)
    {
        *pitch = format_desc->block_byte_count
            * max(1, (width + format_desc->block_width - 1) / format_desc->block_width);
        *size = *pitch
            * max(1, (height + format_desc->block_height - 1) / format_desc->block_height);
    }
    else
    {
        *pitch = width * format_desc->bytes_per_pixel;
        *size = *pitch * height;
    }

    return D3D_OK;
}

static UINT calculate_dds_file_size(D3DFORMAT format, UINT width, UINT height, UINT depth,
    UINT miplevels, UINT faces)
{
    UINT i, file_size = 0;

    for (i = 0; i < miplevels; i++)
    {
        UINT pitch, size = 0;
        calculate_dds_surface_size(format, width, height, &pitch, &size);
        size *= depth;
        file_size += size;
        width = max(1, width / 2);
        height = max(1, height / 2);
        depth = max(1, depth / 2);
    }

    file_size *= faces;
    file_size += sizeof(struct dds_header);
    return file_size;
}

/************************************************************
* get_image_info_from_dds
*
* Fills a D3DXIMAGE_INFO structure with information
* about a DDS file stored in the memory.
*
* PARAMS
*   buffer  [I] pointer to DDS data
*   length  [I] size of DDS data
*   info    [O] pointer to D3DXIMAGE_INFO structure
*
* RETURNS
*   Success: D3D_OK
*   Failure: D3DXERR_INVALIDDATA
*
*/
static HRESULT get_image_info_from_dds(const void *buffer, UINT length, D3DXIMAGE_INFO *info)
{
    UINT faces = 1;
    UINT expected_length;
    const struct dds_header *header = buffer;

    if (length < sizeof(*header) || !info)
        return D3DXERR_INVALIDDATA;

    if (header->pixel_format.size != sizeof(header->pixel_format))
        return D3DXERR_INVALIDDATA;

    info->Width = header->width;
    info->Height = header->height;
    info->Depth = 1;
    info->MipLevels = (header->flags & DDS_MIPMAPCOUNT) ?  header->miplevels : 1;

    info->Format = dds_pixel_format_to_d3dformat(&header->pixel_format);
    if (info->Format == D3DFMT_UNKNOWN)
        return D3DXERR_INVALIDDATA;

    TRACE("Pixel format is %#x\n", info->Format);

    if (header->caps2 & DDS_CAPS2_VOLUME)
    {
        info->Depth = header->depth;
        info->ResourceType = D3DRTYPE_VOLUMETEXTURE;
    }
    else if (header->caps2 & DDS_CAPS2_CUBEMAP)
    {
        DWORD face;
        faces = 0;
        for (face = DDS_CAPS2_CUBEMAP_POSITIVEX; face <= DDS_CAPS2_CUBEMAP_NEGATIVEZ; face <<= 1)
        {
            if (header->caps2 & face)
                faces++;
        }
        info->ResourceType = D3DRTYPE_CUBETEXTURE;
    }
    else
    {
        info->ResourceType = D3DRTYPE_TEXTURE;
    }

    expected_length = calculate_dds_file_size(info->Format, info->Width, info->Height, info->Depth,
        info->MipLevels, faces);
    if (length < expected_length)
    {
        WARN("File is too short %u, expected at least %u bytes\n", length, expected_length);
        return D3DXERR_INVALIDDATA;
    }

    info->ImageFileFormat = D3DXIFF_DDS;
    return D3D_OK;
}

static HRESULT load_surface_from_dds(IDirect3DSurface9 *dst_surface, const PALETTEENTRY *dst_palette,
    const RECT *dst_rect, const void *src_data, const RECT *src_rect, DWORD filter, D3DCOLOR color_key,
    const D3DXIMAGE_INFO *src_info)
{
    UINT size;
    UINT src_pitch;
    const struct dds_header *header = src_data;
    const BYTE *pixels = (BYTE *)(header + 1);

    if (src_info->ResourceType != D3DRTYPE_TEXTURE)
        return D3DXERR_INVALIDDATA;

    if (FAILED(calculate_dds_surface_size(src_info->Format, src_info->Width, src_info->Height, &src_pitch, &size)))
        return E_NOTIMPL;

    return D3DXLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, pixels, src_info->Format,
        src_pitch, NULL, src_rect, filter, color_key);
}

static HRESULT save_dds_surface_to_memory(ID3DXBuffer **dst_buffer, IDirect3DSurface9 *src_surface, const RECT *src_rect)
{
    HRESULT hr;
    UINT dst_pitch, surface_size, file_size;
    D3DSURFACE_DESC src_desc;
    D3DLOCKED_RECT locked_rect;
    ID3DXBuffer *buffer;
    struct dds_header *header;
    BYTE *pixels;
    struct volume volume;
    const struct pixel_format_desc *pixel_format;

    if (src_rect)
    {
        FIXME("Saving a part of a surface to a DDS file is not implemented yet\n");
        return E_NOTIMPL;
    }

    hr = IDirect3DSurface9_GetDesc(src_surface, &src_desc);
    if (FAILED(hr)) return hr;

    pixel_format = get_format_info(src_desc.Format);
    if (pixel_format->type == FORMAT_UNKNOWN) return E_NOTIMPL;

    file_size = calculate_dds_file_size(src_desc.Format, src_desc.Width, src_desc.Height, 1, 1, 1);

    hr = calculate_dds_surface_size(src_desc.Format, src_desc.Width, src_desc.Height, &dst_pitch, &surface_size);
    if (FAILED(hr)) return hr;

    hr = D3DXCreateBuffer(file_size, &buffer);
    if (FAILED(hr)) return hr;

    header = ID3DXBuffer_GetBufferPointer(buffer);
    pixels = (BYTE *)(header + 1);

    memset(header, 0, sizeof(*header));
    header->signature = MAKEFOURCC('D','D','S',' ');
    /* The signature is not really part of the DDS header */
    header->size = sizeof(*header) - sizeof(header->signature);
    header->flags = DDS_CAPS | DDS_HEIGHT | DDS_WIDTH | DDS_PIXELFORMAT;
    /* Note that native does not set DDS_LINEARSIZE flag nor pitch_or_linear_size field for DXTn */
    header->flags |= (pixel_format->block_width != 1) || (pixel_format->block_height != 1) ? DDS_LINEARSIZE : DDS_PITCH;
    header->height = src_desc.Height;
    header->width = src_desc.Width;
    header->pitch_or_linear_size = dst_pitch;
    header->caps = DDS_CAPS_TEXTURE;
    hr = d3dformat_to_dds_pixel_format(&header->pixel_format, src_desc.Format);
    if (FAILED(hr))
    {
        ID3DXBuffer_Release(buffer);
        return hr;
    }

    hr = IDirect3DSurface9_LockRect(src_surface, &locked_rect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        ID3DXBuffer_Release(buffer);
        return hr;
    }

    volume.width = src_desc.Width;
    volume.height = src_desc.Height;
    volume.depth = 1;
    copy_pixels(locked_rect.pBits, locked_rect.Pitch, 0, pixels, dst_pitch, 0,
        &volume, pixel_format);

    IDirect3DSurface9_UnlockRect(src_surface);

    *dst_buffer = buffer;
    return D3D_OK;
}

static HRESULT get_surface(D3DRESOURCETYPE type, struct IDirect3DBaseTexture9 *tex,
        int face, UINT level, struct IDirect3DSurface9 **surf)
{
    switch (type)
    {
        case D3DRTYPE_TEXTURE:
            return IDirect3DTexture9_GetSurfaceLevel((IDirect3DTexture9*) tex, level, surf);
        case D3DRTYPE_CUBETEXTURE:
            return IDirect3DCubeTexture9_GetCubeMapSurface((IDirect3DCubeTexture9*) tex, face, level, surf);
        default:
            ERR("Unexpected texture type\n");
            return E_NOTIMPL;
    }
}

HRESULT save_dds_texture_to_memory(ID3DXBuffer **dst_buffer, IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    HRESULT hr;
    D3DRESOURCETYPE type;
    UINT mip_levels;
    IDirect3DSurface9 *surface;

    type = IDirect3DBaseTexture9_GetType(src_texture);

    if ((type !=  D3DRTYPE_TEXTURE) && (type != D3DRTYPE_CUBETEXTURE) && (type != D3DRTYPE_VOLUMETEXTURE))
        return D3DERR_INVALIDCALL;

    if (type == D3DRTYPE_CUBETEXTURE)
    {
        FIXME("Cube texture not supported yet\n");
        return E_NOTIMPL;
    }
    else if (type == D3DRTYPE_VOLUMETEXTURE)
    {
        FIXME("Volume texture not supported yet\n");
        return E_NOTIMPL;
    }

    mip_levels = IDirect3DTexture9_GetLevelCount(src_texture);

    if (mip_levels > 1)
    {
        FIXME("Mipmap not supported yet\n");
        return E_NOTIMPL;
    }

    if (src_palette)
    {
        FIXME("Saving surfaces with palettized pixel formats not implemented yet\n");
        return E_NOTIMPL;
    }

    hr = get_surface(type, src_texture, D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);

    if (SUCCEEDED(hr))
    {
        hr = save_dds_surface_to_memory(dst_buffer, surface, NULL);
        IDirect3DSurface9_Release(surface);
    }

    return hr;
}
HRESULT load_volume_from_dds(IDirect3DVolume9 *dst_volume, const PALETTEENTRY *dst_palette,
    const D3DBOX *dst_box, const void *src_data, const D3DBOX *src_box, DWORD filter, D3DCOLOR color_key,
    const D3DXIMAGE_INFO *src_info)
{
    UINT row_pitch, slice_pitch;
    const struct dds_header *header = src_data;
    const BYTE *pixels = (BYTE *)(header + 1);

    if (src_info->ResourceType != D3DRTYPE_VOLUMETEXTURE)
        return D3DXERR_INVALIDDATA;

    if (FAILED(calculate_dds_surface_size(src_info->Format, src_info->Width, src_info->Height, &row_pitch, &slice_pitch)))
        return E_NOTIMPL;

    return D3DXLoadVolumeFromMemory(dst_volume, dst_palette, dst_box, pixels, src_info->Format,
        row_pitch, slice_pitch, NULL, src_box, filter, color_key);
}

HRESULT load_texture_from_dds(IDirect3DTexture9 *texture, const void *src_data, const PALETTEENTRY *palette,
        DWORD filter, D3DCOLOR color_key, const D3DXIMAGE_INFO *src_info, unsigned int skip_levels,
        unsigned int *loaded_miplevels)
{
    HRESULT hr;
    RECT src_rect;
    UINT src_pitch;
    UINT mip_level;
    UINT mip_levels;
    UINT mip_level_size;
    UINT width, height;
    IDirect3DSurface9 *surface;
    const struct dds_header *header = src_data;
    const BYTE *pixels = (BYTE *)(header + 1);

    /* Loading a cube texture as a simple texture is also supported
     * (only first face texture is taken). Same with volume textures. */
    if ((src_info->ResourceType != D3DRTYPE_TEXTURE)
            && (src_info->ResourceType != D3DRTYPE_CUBETEXTURE)
            && (src_info->ResourceType != D3DRTYPE_VOLUMETEXTURE))
    {
        WARN("Trying to load a %u resource as a 2D texture, returning failure.\n", src_info->ResourceType);
        return D3DXERR_INVALIDDATA;
    }

    width = src_info->Width;
    height = src_info->Height;
    mip_levels = min(src_info->MipLevels, IDirect3DTexture9_GetLevelCount(texture));
    if (src_info->ResourceType == D3DRTYPE_VOLUMETEXTURE)
        mip_levels = 1;
    for (mip_level = 0; mip_level < mip_levels + skip_levels; ++mip_level)
    {
        hr = calculate_dds_surface_size(src_info->Format, width, height, &src_pitch, &mip_level_size);
        if (FAILED(hr)) return hr;

        if (mip_level >= skip_levels)
        {
            SetRect(&src_rect, 0, 0, width, height);

            IDirect3DTexture9_GetSurfaceLevel(texture, mip_level - skip_levels, &surface);
            hr = D3DXLoadSurfaceFromMemory(surface, palette, NULL, pixels, src_info->Format, src_pitch,
                    NULL, &src_rect, filter, color_key);
            IDirect3DSurface9_Release(surface);
            if (FAILED(hr))
                return hr;
        }

        pixels += mip_level_size;
        width = max(1, width / 2);
        height = max(1, height / 2);
    }

    *loaded_miplevels = mip_levels - skip_levels;

    return D3D_OK;
}

HRESULT load_cube_texture_from_dds(IDirect3DCubeTexture9 *cube_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, DWORD color_key, const D3DXIMAGE_INFO *src_info)
{
    HRESULT hr;
    int face;
    UINT mip_level;
    UINT size;
    RECT src_rect;
    UINT src_pitch;
    UINT mip_levels;
    UINT mip_level_size;
    IDirect3DSurface9 *surface;
    const struct dds_header *header = src_data;
    const BYTE *pixels = (BYTE *)(header + 1);

    if (src_info->ResourceType != D3DRTYPE_CUBETEXTURE)
        return D3DXERR_INVALIDDATA;

    if ((header->caps2 & DDS_CAPS2_CUBEMAP_ALL_FACES) != DDS_CAPS2_CUBEMAP_ALL_FACES)
    {
        WARN("Only full cubemaps are supported\n");
        return D3DXERR_INVALIDDATA;
    }

    mip_levels = min(src_info->MipLevels, IDirect3DCubeTexture9_GetLevelCount(cube_texture));
    for (face = D3DCUBEMAP_FACE_POSITIVE_X; face <= D3DCUBEMAP_FACE_NEGATIVE_Z; face++)
    {
        size = src_info->Width;
        for (mip_level = 0; mip_level < src_info->MipLevels; mip_level++)
        {
            hr = calculate_dds_surface_size(src_info->Format, size, size, &src_pitch, &mip_level_size);
            if (FAILED(hr)) return hr;

            /* if texture has fewer mip levels than DDS file, skip excessive mip levels */
            if (mip_level < mip_levels)
            {
                SetRect(&src_rect, 0, 0, size, size);

                IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture, face, mip_level, &surface);
                hr = D3DXLoadSurfaceFromMemory(surface, palette, NULL, pixels, src_info->Format, src_pitch,
                    NULL, &src_rect, filter, color_key);
                IDirect3DSurface9_Release(surface);
                if (FAILED(hr)) return hr;
            }

            pixels += mip_level_size;
            size = max(1, size / 2);
        }
    }

    return D3D_OK;
}

HRESULT load_volume_texture_from_dds(IDirect3DVolumeTexture9 *volume_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, DWORD color_key, const D3DXIMAGE_INFO *src_info)
{
    HRESULT hr;
    UINT mip_level;
    UINT mip_levels;
    UINT src_slice_pitch;
    UINT src_row_pitch;
    D3DBOX src_box;
    UINT width, height, depth;
    IDirect3DVolume9 *volume;
    const struct dds_header *header = src_data;
    const BYTE *pixels = (BYTE *)(header + 1);

    if (src_info->ResourceType != D3DRTYPE_VOLUMETEXTURE)
        return D3DXERR_INVALIDDATA;

    width = src_info->Width;
    height = src_info->Height;
    depth = src_info->Depth;
    mip_levels = min(src_info->MipLevels, IDirect3DVolumeTexture9_GetLevelCount(volume_texture));

    for (mip_level = 0; mip_level < mip_levels; mip_level++)
    {
        hr = calculate_dds_surface_size(src_info->Format, width, height, &src_row_pitch, &src_slice_pitch);
        if (FAILED(hr)) return hr;

        hr = IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, mip_level, &volume);
        if (FAILED(hr)) return hr;

        src_box.Left = 0;
        src_box.Top = 0;
        src_box.Right = width;
        src_box.Bottom = height;
        src_box.Front = 0;
        src_box.Back = depth;

        hr = D3DXLoadVolumeFromMemory(volume, palette, NULL, pixels, src_info->Format,
            src_row_pitch, src_slice_pitch, NULL, &src_box, filter, color_key);

        IDirect3DVolume9_Release(volume);
        if (FAILED(hr)) return hr;

        pixels += depth * src_slice_pitch;
        width = max(1, width / 2);
        height = max(1, height / 2);
        depth = max(1, depth / 2);
    }

    return D3D_OK;
}

static BOOL convert_dib_to_bmp(void **data, UINT *size)
{
    ULONG header_size;
    ULONG count = 0;
    ULONG offset;
    BITMAPFILEHEADER *header;
    BYTE *new_data;
    UINT new_size;

    if ((*size < 4) || (*size < (header_size = *(ULONG*)*data)))
        return FALSE;

    if ((header_size == sizeof(BITMAPINFOHEADER)) ||
        (header_size == sizeof(BITMAPV4HEADER)) ||
        (header_size == sizeof(BITMAPV5HEADER)) ||
        (header_size == 64 /* sizeof(BITMAPCOREHEADER2) */))
    {
        /* All structures begin with the same memory layout as BITMAPINFOHEADER */
        BITMAPINFOHEADER *info_header = (BITMAPINFOHEADER*)*data;
        count = info_header->biClrUsed;

        if (!count && info_header->biBitCount <= 8)
            count = 1 << info_header->biBitCount;

        offset = sizeof(BITMAPFILEHEADER) + header_size + sizeof(RGBQUAD) * count;

        /* For BITMAPINFOHEADER with BI_BITFIELDS compression, there are 3 additional color masks after header */
        if ((info_header->biSize == sizeof(BITMAPINFOHEADER)) && (info_header->biCompression == BI_BITFIELDS))
            offset += 3 * sizeof(DWORD);
    }
    else if (header_size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core_header = (BITMAPCOREHEADER*)*data;

        if (core_header->bcBitCount <= 8)
            count = 1 << core_header->bcBitCount;

        offset = sizeof(BITMAPFILEHEADER) + header_size + sizeof(RGBTRIPLE) * count;
    }
    else
    {
        return FALSE;
    }

    TRACE("Converting DIB file to BMP\n");

    new_size = *size + sizeof(BITMAPFILEHEADER);
    new_data = HeapAlloc(GetProcessHeap(), 0, new_size);
    CopyMemory(new_data + sizeof(BITMAPFILEHEADER), *data, *size);

    /* Add BMP header */
    header = (BITMAPFILEHEADER*)new_data;
    header->bfType = 0x4d42; /* BM */
    header->bfSize = new_size;
    header->bfReserved1 = 0;
    header->bfReserved2 = 0;
    header->bfOffBits = offset;

    /* Update input data */
    *data = new_data;
    *size = new_size;

    return TRUE;
}

/************************************************************
 * D3DXGetImageInfoFromFileInMemory
 *
 * Fills a D3DXIMAGE_INFO structure with info about an image
 *
 * PARAMS
 *   data     [I] pointer to the image file data
 *   datasize [I] size of the passed data
 *   info     [O] pointer to the destination structure
 *
 * RETURNS
 *   Success: D3D_OK, if info is not NULL and data and datasize make up a valid image file or
 *                    if info is NULL and data and datasize are not NULL
 *   Failure: D3DXERR_INVALIDDATA, if data is no valid image file and datasize and info are not NULL
 *            D3DERR_INVALIDCALL, if data is NULL or
 *                                if datasize is 0
 *
 * NOTES
 *   datasize may be bigger than the actual file size
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(const void *data, UINT datasize, D3DXIMAGE_INFO *info)
{
    IWICImagingFactory *factory;
    IWICBitmapDecoder *decoder = NULL;
    IWICStream *stream;
    HRESULT hr;
    HRESULT initresult;
    BOOL dib;

    TRACE("(%p, %d, %p)\n", data, datasize, info);

    if (!data || !datasize)
        return D3DERR_INVALIDCALL;

    if (!info)
        return D3D_OK;

    if ((datasize >= 4) && !strncmp(data, "DDS ", 4)) {
        TRACE("File type is DDS\n");
        return get_image_info_from_dds(data, datasize, info);
    }

    /* In case of DIB file, convert it to BMP */
    dib = convert_dib_to_bmp((void**)&data, &datasize);

    initresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&factory);

    if (SUCCEEDED(hr)) {
        IWICImagingFactory_CreateStream(factory, &stream);
        IWICStream_InitializeFromMemory(stream, (BYTE*)data, datasize);
        hr = IWICImagingFactory_CreateDecoderFromStream(factory, (IStream*)stream, NULL, 0, &decoder);
        IWICStream_Release(stream);
        IWICImagingFactory_Release(factory);
    }

    if (FAILED(hr)) {
        if ((datasize >= 2) && (!strncmp(data, "P3", 2) || !strncmp(data, "P6", 2)))
            FIXME("File type PPM is not supported yet\n");
        else if ((datasize >= 10) && !strncmp(data, "#?RADIANCE", 10))
            FIXME("File type HDR is not supported yet\n");
        else if ((datasize >= 2) && (!strncmp(data, "PF", 2) || !strncmp(data, "Pf", 2)))
            FIXME("File type PFM is not supported yet\n");
    }

    if (SUCCEEDED(hr)) {
        GUID container_format;
        UINT frame_count;

        hr = IWICBitmapDecoder_GetContainerFormat(decoder, &container_format);
        if (SUCCEEDED(hr)) {
            if (IsEqualGUID(&container_format, &GUID_ContainerFormatBmp)) {
                if (dib) {
                    TRACE("File type is DIB\n");
                    info->ImageFileFormat = D3DXIFF_DIB;
                } else {
                    TRACE("File type is BMP\n");
                    info->ImageFileFormat = D3DXIFF_BMP;
                }
            } else if (IsEqualGUID(&container_format, &GUID_ContainerFormatPng)) {
                TRACE("File type is PNG\n");
                info->ImageFileFormat = D3DXIFF_PNG;
            } else if(IsEqualGUID(&container_format, &GUID_ContainerFormatJpeg)) {
                TRACE("File type is JPG\n");
                info->ImageFileFormat = D3DXIFF_JPG;
            } else if(IsEqualGUID(&container_format, &GUID_WineContainerFormatTga)) {
                TRACE("File type is TGA\n");
                info->ImageFileFormat = D3DXIFF_TGA;
            } else {
                WARN("Unsupported image file format %s\n", debugstr_guid(&container_format));
                hr = D3DXERR_INVALIDDATA;
            }
        }

        if (SUCCEEDED(hr))
            hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        if (SUCCEEDED(hr) && !frame_count)
            hr = D3DXERR_INVALIDDATA;

        if (SUCCEEDED(hr)) {
            IWICBitmapFrameDecode *frame = NULL;

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);

            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameDecode_GetSize(frame, &info->Width, &info->Height);

            if (SUCCEEDED(hr)) {
                WICPixelFormatGUID pixel_format;

                hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &pixel_format);
                if (SUCCEEDED(hr)) {
                    info->Format = wic_guid_to_d3dformat(&pixel_format);
                    if (info->Format == D3DFMT_UNKNOWN) {
                        WARN("Unsupported pixel format %s\n", debugstr_guid(&pixel_format));
                        hr = D3DXERR_INVALIDDATA;
                    }
                }
            }

            if (frame)
                 IWICBitmapFrameDecode_Release(frame);

            info->Depth = 1;
            info->MipLevels = 1;
            info->ResourceType = D3DRTYPE_TEXTURE;
        }
    }

    if (decoder)
        IWICBitmapDecoder_Release(decoder);

    if (SUCCEEDED(initresult))
        CoUninitialize();

    if (dib)
        HeapFree(GetProcessHeap(), 0, (void*)data);

    if (FAILED(hr)) {
        TRACE("Invalid or unsupported image file\n");
        return D3DXERR_INVALIDDATA;
    }

    return D3D_OK;
}

/************************************************************
 * D3DXGetImageInfoFromFile
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load a valid image file or
 *                    if we successfully load a file which is no valid image and info is NULL
 *   Failure: D3DXERR_INVALIDDATA, if we fail to load file or
 *                                 if file is not a valid image file and info is not NULL
 *            D3DERR_INVALIDCALL, if file is NULL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileA(const char *file, D3DXIMAGE_INFO *info)
{
    WCHAR *widename;
    HRESULT hr;
    int strlength;

    TRACE("file %s, info %p.\n", debugstr_a(file), info);

    if( !file ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
    widename = HeapAlloc(GetProcessHeap(), 0, strlength * sizeof(*widename));
    MultiByteToWideChar(CP_ACP, 0, file, -1, widename, strlength);

    hr = D3DXGetImageInfoFromFileW(widename, info);
    HeapFree(GetProcessHeap(), 0, widename);

    return hr;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(const WCHAR *file, D3DXIMAGE_INFO *info)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("file %s, info %p.\n", debugstr_w(file), info);

    if (!file)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(file, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    UnmapViewOfFile(buffer);

    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromResource
 *
 * RETURNS
 *   Success: D3D_OK, if resource is a valid image file
 *   Failure: D3DXERR_INVALIDDATA, if resource is no valid image file or NULL or
 *                                 if we fail to load resource
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, const char *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_a(resource), info);

    if (!(resinfo = FindResourceA(module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_w(resource), info);

    if (!(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

/************************************************************
 * D3DXLoadSurfaceFromFileInMemory
 *
 * Loads data from a given buffer into a surface and fills a given
 * D3DXIMAGE_INFO structure with info about the source data.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcData     [I] pointer to the source data
 *   SrcDataSize  [I] size of the source data in bytes
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *   pSrcInfo     [O] pointer to a D3DXIMAGE_INFO structure
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, pSrcData or SrcDataSize is NULL
 *            D3DXERR_INVALIDDATA, if pSrcData is no valid image file
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(IDirect3DSurface9 *pDestSurface,
        const PALETTEENTRY *pDestPalette, const RECT *pDestRect, const void *pSrcData, UINT SrcDataSize,
        const RECT *pSrcRect, DWORD dwFilter, D3DCOLOR Colorkey, D3DXIMAGE_INFO *pSrcInfo)
{
    D3DXIMAGE_INFO imginfo;
    HRESULT hr, com_init;

    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *bitmapframe;
    IWICStream *stream;

    const struct pixel_format_desc *formatdesc;
    WICRect wicrect;
    RECT rect;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_data %p, src_data_size %u, "
            "src_rect %s, filter %#x, color_key 0x%08x, src_info %p.\n",
            pDestSurface, pDestPalette, wine_dbgstr_rect(pDestRect), pSrcData, SrcDataSize,
            wine_dbgstr_rect(pSrcRect), dwFilter, Colorkey, pSrcInfo);

    if (!pDestSurface || !pSrcData || !SrcDataSize)
        return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(pSrcData, SrcDataSize, &imginfo);

    if (FAILED(hr))
        return hr;

    if (pSrcRect)
    {
        wicrect.X = pSrcRect->left;
        wicrect.Y = pSrcRect->top;
        wicrect.Width = pSrcRect->right - pSrcRect->left;
        wicrect.Height = pSrcRect->bottom - pSrcRect->top;
    }
    else
    {
        wicrect.X = 0;
        wicrect.Y = 0;
        wicrect.Width = imginfo.Width;
        wicrect.Height = imginfo.Height;
    }

    SetRect(&rect, 0, 0, wicrect.Width, wicrect.Height);

    if (imginfo.ImageFileFormat == D3DXIFF_DDS)
    {
        hr = load_surface_from_dds(pDestSurface, pDestPalette, pDestRect, pSrcData, &rect,
            dwFilter, Colorkey, &imginfo);
        if (SUCCEEDED(hr) && pSrcInfo)
            *pSrcInfo = imginfo;
        return hr;
    }

    if (imginfo.ImageFileFormat == D3DXIFF_DIB)
        convert_dib_to_bmp((void**)&pSrcData, &SrcDataSize);

    com_init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&factory)))
        goto cleanup_err;

    if (FAILED(IWICImagingFactory_CreateStream(factory, &stream)))
    {
        IWICImagingFactory_Release(factory);
        factory = NULL;
        goto cleanup_err;
    }

    IWICStream_InitializeFromMemory(stream, (BYTE*)pSrcData, SrcDataSize);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, (IStream*)stream, NULL, 0, &decoder);

    IWICStream_Release(stream);

    if (FAILED(hr))
        goto cleanup_err;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &bitmapframe);

    if (FAILED(hr))
        goto cleanup_bmp;

    formatdesc = get_format_info(imginfo.Format);

    if (formatdesc->type == FORMAT_UNKNOWN)
    {
        FIXME("Unsupported pixel format\n");
        hr = D3DXERR_INVALIDDATA;
    }
    else
    {
        BYTE *buffer;
        DWORD pitch;
        PALETTEENTRY *palette = NULL;
        WICColor *colors = NULL;

        pitch = formatdesc->bytes_per_pixel * wicrect.Width;
        buffer = HeapAlloc(GetProcessHeap(), 0, pitch * wicrect.Height);

        hr = IWICBitmapFrameDecode_CopyPixels(bitmapframe, &wicrect, pitch,
                                              pitch * wicrect.Height, buffer);

        if (SUCCEEDED(hr) && (formatdesc->type == FORMAT_INDEX))
        {
            IWICPalette *wic_palette = NULL;
            UINT nb_colors;

            hr = IWICImagingFactory_CreatePalette(factory, &wic_palette);
            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameDecode_CopyPalette(bitmapframe, wic_palette);
            if (SUCCEEDED(hr))
                hr = IWICPalette_GetColorCount(wic_palette, &nb_colors);
            if (SUCCEEDED(hr))
            {
                colors = HeapAlloc(GetProcessHeap(), 0, nb_colors * sizeof(colors[0]));
                palette = HeapAlloc(GetProcessHeap(), 0, nb_colors * sizeof(palette[0]));
                if (!colors || !palette)
                    hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
                hr = IWICPalette_GetColors(wic_palette, nb_colors, colors, &nb_colors);
            if (SUCCEEDED(hr))
            {
                UINT i;

                /* Convert colors from WICColor (ARGB) to PALETTEENTRY (ABGR) */
                for (i = 0; i < nb_colors; i++)
                {
                    palette[i].peRed   = (colors[i] >> 16) & 0xff;
                    palette[i].peGreen = (colors[i] >> 8) & 0xff;
                    palette[i].peBlue  = colors[i] & 0xff;
                    palette[i].peFlags = (colors[i] >> 24) & 0xff; /* peFlags is the alpha component in DX8 and higher */
                }
            }
            if (wic_palette)
                IWICPalette_Release(wic_palette);
        }

        if (SUCCEEDED(hr))
        {
            hr = D3DXLoadSurfaceFromMemory(pDestSurface, pDestPalette, pDestRect,
                                           buffer, imginfo.Format, pitch,
                                           palette, &rect, dwFilter, Colorkey);
        }

        HeapFree(GetProcessHeap(), 0, colors);
        HeapFree(GetProcessHeap(), 0, palette);
        HeapFree(GetProcessHeap(), 0, buffer);
    }

    IWICBitmapFrameDecode_Release(bitmapframe);

cleanup_bmp:
    IWICBitmapDecoder_Release(decoder);

cleanup_err:
    if (factory)
        IWICImagingFactory_Release(factory);

    if (SUCCEEDED(com_init))
        CoUninitialize();

    if (imginfo.ImageFileFormat == D3DXIFF_DIB)
        HeapFree(GetProcessHeap(), 0, (void*)pSrcData);

    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    if (pSrcInfo)
        *pSrcInfo = imginfo;

    return D3D_OK;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const char *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    WCHAR *src_file_w;
    HRESULT hr;
    int strlength;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#x, color_key 0x%08x, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_a(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0);
    src_file_w = HeapAlloc(GetProcessHeap(), 0, strlength * sizeof(*src_file_w));
    MultiByteToWideChar(CP_ACP, 0, src_file, -1, src_file_w, strlength);

    hr = D3DXLoadSurfaceFromFileW(dst_surface, dst_palette, dst_rect,
            src_file_w, src_rect, filter, color_key, src_info);
    HeapFree(GetProcessHeap(), 0, src_file_w);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const WCHAR *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    UINT data_size;
    void *data;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#x, color_key 0x%08x, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_w(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(src_file, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
    UnmapViewOfFile(data);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const char *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    UINT data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#x, color_key 0x%08x, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_a(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceA(src_module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(src_module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const WCHAR *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    UINT data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#x, color_key 0x%08x, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_w(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}


/************************************************************
 * helper functions for D3DXLoadSurfaceFromMemory
 */
struct argb_conversion_info
{
    const struct pixel_format_desc *srcformat;
    const struct pixel_format_desc *destformat;
    DWORD srcshift[4], destshift[4];
    DWORD srcmask[4], destmask[4];
    BOOL process_channel[4];
    DWORD channelmask;
};

static void init_argb_conversion_info(const struct pixel_format_desc *srcformat, const struct pixel_format_desc *destformat, struct argb_conversion_info *info)
{
    UINT i;
    ZeroMemory(info->process_channel, 4 * sizeof(BOOL));
    info->channelmask = 0;

    info->srcformat  =  srcformat;
    info->destformat = destformat;

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        info->srcshift[i]  =  srcformat->shift[i] + max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        info->destshift[i] = destformat->shift[i] + max(destformat->bits[i] -  srcformat->bits[i], 0);

        info->srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        info->destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) info->process_channel[i] = TRUE;
            else info->channelmask |= info->destmask[i];
        }
    }
}

/************************************************************
 * get_relevant_argb_components
 *
 * Extracts the relevant components from the source color and
 * drops the less significant bits if they aren't used by the destination format.
 */
static void get_relevant_argb_components(const struct argb_conversion_info *info, const BYTE *col, DWORD *out)
{
    unsigned int i, j;
    unsigned int component, mask;

    for (i = 0; i < 4; ++i)
    {
        if (!info->process_channel[i])
            continue;

        component = 0;
        mask = info->srcmask[i];
        for (j = 0; j < 4 && mask; ++j)
        {
            if (info->srcshift[i] < j * 8)
                component |= (col[j] & mask) << (j * 8 - info->srcshift[i]);
            else
                component |= (col[j] & mask) >> (info->srcshift[i] - j * 8);
            mask >>= 8;
        }
        out[i] = component;
    }
}

/************************************************************
 * make_argb_color
 *
 * Recombines the output of get_relevant_argb_components and converts
 * it to the destination format.
 */
static DWORD make_argb_color(const struct argb_conversion_info *info, const DWORD *in)
{
    UINT i;
    DWORD val = 0;

    for(i = 0;i < 4;i++) {
        if(info->process_channel[i]) {
            /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
            signed int shift;
            for(shift = info->destshift[i]; shift > info->destformat->shift[i]; shift -= info->srcformat->bits[i]) val |= in[i] << shift;
            val |= (in[i] >> (info->destformat->shift[i] - shift)) << info->destformat->shift[i];
        }
    }
    val |= info->channelmask;   /* new channels are set to their maximal value */
    return val;
}

/* It doesn't work for components bigger than 32 bits (or somewhat smaller but unaligned). */
static void format_to_vec4(const struct pixel_format_desc *format, const BYTE *src, struct vec4 *dst)
{
    DWORD mask, tmp;
    unsigned int c;

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        float *dst_component = (float *)dst + component_offsets[c];

        if (format->bits[c])
        {
            mask = ~0u >> (32 - format->bits[c]);

            memcpy(&tmp, src + format->shift[c] / 8,
                    min(sizeof(DWORD), (format->shift[c] % 8 + format->bits[c] + 7) / 8));

            if (format->type == FORMAT_ARGBF16)
                *dst_component = float_16_to_32(tmp);
            else if (format->type == FORMAT_ARGBF)
                *dst_component = *(float *)&tmp;
            else
                *dst_component = (float)((tmp >> format->shift[c] % 8) & mask) / mask;
        }
        else
            *dst_component = 1.0f;
    }
}

/* It doesn't work for components bigger than 32 bits. */
static void format_from_vec4(const struct pixel_format_desc *format, const struct vec4 *src, BYTE *dst)
{
    DWORD v, mask32;
    unsigned int c, i;

    memset(dst, 0, format->bytes_per_pixel);

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        const float src_component = *((const float *)src + component_offsets[c]);

        if (!format->bits[c])
            continue;

        mask32 = ~0u >> (32 - format->bits[c]);

        if (format->type == FORMAT_ARGBF16)
            v = float_32_to_16(src_component);
        else if (format->type == FORMAT_ARGBF)
            v = *(DWORD *)&src_component;
        else
            v = (DWORD)(src_component * ((1 << format->bits[c]) - 1) + 0.5f);

        for (i = format->shift[c] / 8 * 8; i < format->shift[c] + format->bits[c]; i += 8)
        {
            BYTE mask, byte;

            if (format->shift[c] > i)
            {
                mask = mask32 << (format->shift[c] - i);
                byte = (v << (format->shift[c] - i)) & mask;
            }
            else
            {
                mask = mask32 >> (i - format->shift[c]);
                byte = (v >> (i - format->shift[c])) & mask;
            }
            dst[i / 8] |= byte;
        }
    }
}

/************************************************************
 * copy_pixels
 *
 * Copies the source buffer to the destination buffer.
 * Works for any pixel format.
 * The source and the destination must be block-aligned.
 */
void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
        BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
        const struct pixel_format_desc *format)
{
    UINT row, slice;
    BYTE *dst_addr;
    const BYTE *src_addr;
    UINT row_block_count = (size->width + format->block_width - 1) / format->block_width;
    UINT row_count = (size->height + format->block_height - 1) / format->block_height;

    for (slice = 0; slice < size->depth; slice++)
    {
        src_addr = src + slice * src_slice_pitch;
        dst_addr = dst + slice * dst_slice_pitch;

        for (row = 0; row < row_count; row++)
        {
            memcpy(dst_addr, src_addr, row_block_count * format->block_byte_count);
            src_addr += src_row_pitch;
            dst_addr += dst_row_pitch;
        }
    }
}

/************************************************************
 * convert_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Pixels outsize the source rect are blacked out.
 * Works only for ARGB formats with 1 - 4 bytes per pixel.
 */
void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
        const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
        const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
        const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT min_width, min_height, min_depth;
    UINT x, y, z;

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    min_width = min(src_size->width, dst_size->width);
    min_height = min(src_size->height, dst_size->height);
    min_depth = min(src_size->depth, dst_size->depth);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < min_depth; z++) {
        const BYTE *src_slice_ptr = src + z * src_slice_pitch;
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;

        for (y = 0; y < min_height; y++) {
            const BYTE *src_ptr = src_slice_ptr + y * src_row_pitch;
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;

            for (x = 0; x < min_width; x++) {
                if (!src_format->to_rgba && !dst_format->from_rgba
                        && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                src_ptr += src_format->bytes_per_pixel;
                dst_ptr += dst_format->bytes_per_pixel;
            }

            if (src_size->width < dst_size->width) /* black out remaining pixels */
                memset(dst_ptr, 0, dst_format->bytes_per_pixel * (dst_size->width - src_size->width));
        }

        if (src_size->height < dst_size->height) /* black out remaining pixels */
            memset(dst + src_size->height * dst_row_pitch, 0, dst_row_pitch * (dst_size->height - src_size->height));
    }
    if (src_size->depth < dst_size->depth) /* black out remaining pixels */
        memset(dst + src_size->depth * dst_slice_pitch, 0, dst_slice_pitch * (dst_size->depth - src_size->depth));
}

/************************************************************
 * point_filter_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion, color keying and stretching
 * using a point filter.
 * Works only for ARGB formats with 1 - 4 bytes per pixel.
 */
void point_filter_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
        const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
        const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
        const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT x, y, z;

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < dst_size->depth; z++)
    {
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;
        const BYTE *src_slice_ptr = src + src_slice_pitch * (z * src_size->depth / dst_size->depth);

        for (y = 0; y < dst_size->height; y++)
        {
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;
            const BYTE *src_row_ptr = src_slice_ptr + src_row_pitch * (y * src_size->height / dst_size->height);

            for (x = 0; x < dst_size->width; x++)
            {
                const BYTE *src_ptr = src_row_ptr + (x * src_size->width / dst_size->width) * src_format->bytes_per_pixel;

                if (!src_format->to_rgba && !dst_format->from_rgba
                        && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                dst_ptr += dst_format->bytes_per_pixel;
            }
        }
    }
}

typedef BOOL (*dxtn_conversion_func)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                                     enum wined3d_format_id format, unsigned int w, unsigned int h);

static dxtn_conversion_func get_dxtn_conversion_func(D3DFORMAT format, BOOL encode)
{
    switch (format)
    {
        case D3DFMT_DXT1:
            if (!wined3d_dxtn_supported()) return NULL;
            return encode ? wined3d_dxt1_encode : wined3d_dxt1_decode;
        case D3DFMT_DXT3:
            if (!wined3d_dxtn_supported()) return NULL;
            return encode ? wined3d_dxt3_encode : wined3d_dxt3_decode;
        case D3DFMT_DXT5:
            if (!wined3d_dxtn_supported()) return NULL;
            return encode ? wined3d_dxt5_encode : wined3d_dxt5_decode;
        default:
            return NULL;
    }
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN) or
 *                                if DestRect is invalid
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *srcformatdesc, *destformatdesc;
    D3DSURFACE_DESC surfdesc;
    D3DLOCKED_RECT lockrect;
    struct volume src_size, dst_size;
    HRESULT ret = D3D_OK;

    TRACE("(%p, %p, %s, %p, %#x, %u, %p, %s, %#x, 0x%08x)\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_memory, src_format,
            src_pitch, src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_memory || !src_rect)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }
    if (src_format == D3DFMT_UNKNOWN
            || src_rect->left >= src_rect->right
            || src_rect->top >= src_rect->bottom)
    {
        WARN("Invalid src_format or src_rect.\n");
        return E_FAIL;
    }

    if (filter == D3DX_DEFAULT)
        filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    IDirect3DSurface9_GetDesc(dst_surface, &surfdesc);

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;
    if (!dst_rect)
    {
        dst_size.width = surfdesc.Width;
        dst_size.height = surfdesc.Height;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > surfdesc.Width
                || dst_rect->top > dst_rect->bottom || dst_rect->bottom > surfdesc.Height
                || dst_rect->left < 0 || dst_rect->top < 0)
        {
            WARN("Invalid dst_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
        dst_size.width = dst_rect->right - dst_rect->left;
        dst_size.height = dst_rect->bottom - dst_rect->top;
        if (!dst_size.width || !dst_size.height)
            return D3D_OK;
    }
    dst_size.depth = 1;

    srcformatdesc = get_format_info(src_format);
    destformatdesc = get_format_info(surfdesc.Format);
    if (srcformatdesc->type == FORMAT_UNKNOWN || destformatdesc->type == FORMAT_UNKNOWN)
    {
        FIXME("Unsupported pixel format conversion %#x -> %#x\n", src_format, surfdesc.Format);
        return E_NOTIMPL;
    }

    if (src_format == surfdesc.Format
            && dst_size.width == src_size.width
            && dst_size.height == src_size.height
            && color_key == 0) /* Simple copy. */
    {
        if (src_rect->left & (srcformatdesc->block_width - 1)
                || src_rect->top & (srcformatdesc->block_height - 1)
                || (src_rect->right & (srcformatdesc->block_width - 1)
                    && src_size.width != surfdesc.Width)
                || (src_rect->bottom & (srcformatdesc->block_height - 1)
                    && src_size.height != surfdesc.Height))
        {
            WARN("Source rect %s is misaligned.\n", wine_dbgstr_rect(src_rect));
            return D3DXERR_INVALIDDATA;
        }

        if (FAILED(IDirect3DSurface9_LockRect(dst_surface, &lockrect, dst_rect, 0)))
            return D3DXERR_INVALIDDATA;

        copy_pixels(src_memory, src_pitch, 0, lockrect.pBits, lockrect.Pitch, 0,
                &src_size, srcformatdesc);

        IDirect3DSurface9_UnlockRect(dst_surface);
    }
    else /* Stretching or format conversion. */
    {
        dxtn_conversion_func pre_convert, post_convert;
        void *tmp_src_memory = NULL, *tmp_dst_memory = NULL;
        UINT tmp_src_pitch, tmp_dst_pitch;

        pre_convert  = get_dxtn_conversion_func(srcformatdesc->format, FALSE);
        post_convert = get_dxtn_conversion_func(destformatdesc->format, TRUE);

        if ((!pre_convert && (srcformatdesc->type != FORMAT_ARGB) && (srcformatdesc->type != FORMAT_INDEX)) ||
            (!post_convert && (destformatdesc->type != FORMAT_ARGB)))
        {
            FIXME("Format conversion missing %#x -> %#x\n", src_format, surfdesc.Format);
            return E_NOTIMPL;
        }

        if (FAILED(IDirect3DSurface9_LockRect(dst_surface, &lockrect, dst_rect, 0)))
            return D3DXERR_INVALIDDATA;

        /* handle pre-conversion */
        if (pre_convert)
        {
            tmp_src_memory = HeapAlloc(GetProcessHeap(), 0, src_size.width * src_size.height * sizeof(DWORD));
            if (!tmp_src_memory)
            {
                ret = E_OUTOFMEMORY;
                goto error;
            }
            tmp_src_pitch = src_size.width * sizeof(DWORD);
            if (!pre_convert(src_memory, tmp_src_memory, src_pitch, tmp_src_pitch,
                    WINED3DFMT_B8G8R8A8_UNORM, src_size.width, src_size.height))
            {
                ret = E_FAIL;
                goto error;
            }
            srcformatdesc = get_format_info(D3DFMT_A8R8G8B8);
        }
        else
        {
            tmp_src_memory = (void *)src_memory;
            tmp_src_pitch  = src_pitch;
        }

        /* handle post-conversion */
        if (post_convert)
        {
            tmp_dst_memory = HeapAlloc(GetProcessHeap(), 0, dst_size.width * dst_size.height * sizeof(DWORD));
            if (!tmp_dst_memory)
            {
                ret = E_OUTOFMEMORY;
                goto error;
            }
            tmp_dst_pitch = dst_size.width * sizeof(DWORD);
            destformatdesc = get_format_info(D3DFMT_A8R8G8B8);
        }
        else
        {
            tmp_dst_memory = lockrect.pBits;
            tmp_dst_pitch  = lockrect.Pitch;
        }

        if ((filter & 0xf) == D3DX_FILTER_NONE)
        {
            convert_argb_pixels(tmp_src_memory, tmp_src_pitch, 0, &src_size, srcformatdesc,
                    tmp_dst_memory, tmp_dst_pitch, 0, &dst_size, destformatdesc, color_key, src_palette);
        }
        else /* if ((filter & 0xf) == D3DX_FILTER_POINT) */
        {
            if ((filter & 0xf) != D3DX_FILTER_POINT)
                FIXME("Unhandled filter %#x.\n", filter);

            /* Always apply a point filter until D3DX_FILTER_LINEAR,
             * D3DX_FILTER_TRIANGLE and D3DX_FILTER_BOX are implemented. */
            point_filter_argb_pixels(tmp_src_memory, tmp_src_pitch, 0, &src_size, srcformatdesc,
                    tmp_dst_memory, tmp_dst_pitch, 0, &dst_size, destformatdesc, color_key, src_palette);
        }

        /* handle post-conversion */
        if (post_convert)
        {
            if (!post_convert(tmp_dst_memory, lockrect.pBits, tmp_dst_pitch, lockrect.Pitch,
                    WINED3DFMT_B8G8R8A8_UNORM, dst_size.width, dst_size.height))
            {
                ret = E_FAIL;
                goto error;
            }
        }

error:
        if (pre_convert)
            HeapFree(GetProcessHeap(), 0, tmp_src_memory);
        if (post_convert)
            HeapFree(GetProcessHeap(), 0, tmp_dst_memory);
        IDirect3DSurface9_UnlockRect(dst_surface);
    }

    return ret;
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface is NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
        const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
    RECT rect;
    D3DLOCKED_RECT lock;
    D3DSURFACE_DESC SrcDesc;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
            "src_palette %p, src_rect %s, filter %#x, color_key 0x%08x.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
            src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(src_surface, &SrcDesc);

    if (!src_rect)
        SetRect(&rect, 0, 0, SrcDesc.Width, SrcDesc.Height);
    else
        rect = *src_rect;

    if (FAILED(IDirect3DSurface9_LockRect(src_surface, &lock, NULL, D3DLOCK_READONLY)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect,
            lock.pBits, SrcDesc.Format, lock.Pitch, src_palette, &rect, filter, color_key);

    IDirect3DSurface9_UnlockRect(src_surface);

    return hr;
}


HRESULT WINAPI D3DXSaveSurfaceToFileA(const char *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    int len;
    WCHAR *filename;
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
            wine_dbgstr_a(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, NULL, 0);
    filename = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, filename, len);

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    HeapFree(GetProcessHeap(), 0, filename);
    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileW(const WCHAR *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
        wine_dbgstr_w(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(dst_filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileInMemory(ID3DXBuffer **dst_buffer, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    IWICBitmapEncoder *encoder = NULL;
    IWICBitmapFrameEncode *frame = NULL;
    IPropertyBag2 *encoder_options = NULL;
    IStream *stream = NULL;
    HRESULT hr;
    HRESULT initresult;
    const CLSID *encoder_clsid;
    const GUID *pixel_format_guid;
    WICPixelFormatGUID wic_pixel_format;
    D3DFORMAT d3d_pixel_format;
    D3DSURFACE_DESC src_surface_desc;
    D3DLOCKED_RECT locked_rect;
    int width, height;
    STATSTG stream_stats;
    HGLOBAL stream_hglobal;
    ID3DXBuffer *buffer;
    DWORD size;

    TRACE("(%p, %#x, %p, %p, %s)\n",
        dst_buffer, file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_buffer || !src_surface) return D3DERR_INVALIDCALL;

    if (src_palette)
    {
        FIXME("Saving surfaces with palettized pixel formats is not implemented yet\n");
        return D3DERR_INVALIDCALL;
    }

    switch (file_format)
    {
        case D3DXIFF_BMP:
        case D3DXIFF_DIB:
            encoder_clsid = &CLSID_WICBmpEncoder;
            break;
        case D3DXIFF_PNG:
            encoder_clsid = &CLSID_WICPngEncoder;
            break;
        case D3DXIFF_JPG:
            encoder_clsid = &CLSID_WICJpegEncoder;
            break;
        case D3DXIFF_DDS:
            return save_dds_surface_to_memory(dst_buffer, src_surface, src_rect);
        case D3DXIFF_HDR:
        case D3DXIFF_PFM:
        case D3DXIFF_TGA:
        case D3DXIFF_PPM:
            FIXME("File format %#x is not supported yet\n", file_format);
            return E_NOTIMPL;
        default:
            return D3DERR_INVALIDCALL;
    }

    IDirect3DSurface9_GetDesc(src_surface, &src_surface_desc);
    if (src_rect)
    {
        if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
        {
            WARN("Invalid rectangle with 0 area\n");
            return D3DXCreateBuffer(64, dst_buffer);
        }
        if (src_rect->left < 0 || src_rect->top < 0)
            return D3DERR_INVALIDCALL;
        if (src_rect->left > src_rect->right || src_rect->top > src_rect->bottom)
            return D3DERR_INVALIDCALL;
        if (src_rect->right > src_surface_desc.Width || src_rect->bottom > src_surface_desc.Height)
            return D3DERR_INVALIDCALL;

        width = src_rect->right - src_rect->left;
        height = src_rect->bottom - src_rect->top;
    }
    else
    {
        width = src_surface_desc.Width;
        height = src_surface_desc.Height;
    }

    initresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(encoder_clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void **)&encoder);
    if (FAILED(hr)) goto cleanup_err;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr)) goto cleanup_err;

    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) goto cleanup_err;

    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame, &encoder_options);
    if (FAILED(hr)) goto cleanup_err;

    hr = IWICBitmapFrameEncode_Initialize(frame, encoder_options);
    if (FAILED(hr)) goto cleanup_err;

    hr = IWICBitmapFrameEncode_SetSize(frame, width, height);
    if (FAILED(hr)) goto cleanup_err;

    pixel_format_guid = d3dformat_to_wic_guid(src_surface_desc.Format);
    if (!pixel_format_guid)
    {
        FIXME("Pixel format %#x is not supported yet\n", src_surface_desc.Format);
        hr = E_NOTIMPL;
        goto cleanup;
    }

    memcpy(&wic_pixel_format, pixel_format_guid, sizeof(GUID));
    hr = IWICBitmapFrameEncode_SetPixelFormat(frame, &wic_pixel_format);
    d3d_pixel_format = wic_guid_to_d3dformat(&wic_pixel_format);
    if (SUCCEEDED(hr) && d3d_pixel_format != D3DFMT_UNKNOWN)
    {
        TRACE("Using pixel format %s %#x\n", debugstr_guid(&wic_pixel_format), d3d_pixel_format);

        if (src_surface_desc.Format == d3d_pixel_format) /* Simple copy */
        {
            hr = IDirect3DSurface9_LockRect(src_surface, &locked_rect, src_rect, D3DLOCK_READONLY);
            if (SUCCEEDED(hr))
            {
                IWICBitmapFrameEncode_WritePixels(frame, height,
                    locked_rect.Pitch, height * locked_rect.Pitch, locked_rect.pBits);
                IDirect3DSurface9_UnlockRect(src_surface);
            }
        }
        else /* Pixel format conversion */
        {
            const struct pixel_format_desc *src_format_desc, *dst_format_desc;
            struct volume size;
            DWORD dst_pitch;
            void *dst_data;

            src_format_desc = get_format_info(src_surface_desc.Format);
            dst_format_desc = get_format_info(d3d_pixel_format);
            if (src_format_desc->type != FORMAT_ARGB || dst_format_desc->type != FORMAT_ARGB)
            {
                FIXME("Unsupported pixel format conversion %#x -> %#x\n",
                    src_surface_desc.Format, d3d_pixel_format);
                hr = E_NOTIMPL;
                goto cleanup;
            }

            size.width = width;
            size.height = height;
            size.depth = 1;
            dst_pitch = width * dst_format_desc->bytes_per_pixel;
            dst_data = HeapAlloc(GetProcessHeap(), 0, dst_pitch * height);
            if (!dst_data)
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

            hr = IDirect3DSurface9_LockRect(src_surface, &locked_rect, src_rect, D3DLOCK_READONLY);
            if (SUCCEEDED(hr))
            {
                convert_argb_pixels(locked_rect.pBits, locked_rect.Pitch, 0, &size, src_format_desc,
                    dst_data, dst_pitch, 0, &size, dst_format_desc, 0, NULL);
                IDirect3DSurface9_UnlockRect(src_surface);
            }

            IWICBitmapFrameEncode_WritePixels(frame, height, dst_pitch, dst_pitch * height, dst_data);
            HeapFree(GetProcessHeap(), 0, dst_data);
        }

        hr = IWICBitmapFrameEncode_Commit(frame);
        if (SUCCEEDED(hr)) hr = IWICBitmapEncoder_Commit(encoder);
    }
    else WARN("Unsupported pixel format %#x\n", src_surface_desc.Format);

    /* copy data from stream to ID3DXBuffer */
    hr = IStream_Stat(stream, &stream_stats, STATFLAG_NONAME);
    if (FAILED(hr)) goto cleanup_err;

    if (stream_stats.cbSize.u.HighPart != 0)
    {
        hr = D3DXERR_INVALIDDATA;
        goto cleanup;
    }
    size = stream_stats.cbSize.u.LowPart;

    /* Remove BMP header for DIB */
    if (file_format == D3DXIFF_DIB)
        size -= sizeof(BITMAPFILEHEADER);

    hr = D3DXCreateBuffer(size, &buffer);
    if (FAILED(hr)) goto cleanup;

    hr = GetHGlobalFromStream(stream, &stream_hglobal);
    if (SUCCEEDED(hr))
    {
        void *buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        void *stream_data = GlobalLock(stream_hglobal);
        /* Remove BMP header for DIB */
        if (file_format == D3DXIFF_DIB)
            stream_data = (void*)((BYTE*)stream_data + sizeof(BITMAPFILEHEADER));
        memcpy(buffer_pointer, stream_data, size);
        GlobalUnlock(stream_hglobal);
        *dst_buffer = buffer;
    }
    else ID3DXBuffer_Release(buffer);

cleanup_err:
    if (FAILED(hr) && hr != E_OUTOFMEMORY)
        hr = D3DERR_INVALIDCALL;

cleanup:
    if (stream) IStream_Release(stream);

    if (frame) IWICBitmapFrameEncode_Release(frame);
    if (encoder_options) IPropertyBag2_Release(encoder_options);

    if (encoder) IWICBitmapEncoder_Release(encoder);

    if (SUCCEEDED(initresult)) CoUninitialize();

    return hr;
}
