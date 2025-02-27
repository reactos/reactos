/*
 * Copyright 2009 Tony Wasserka
 * Copyright 2010 Christian Costa
 * Copyright 2010 Owen Rudge for CodeWeavers
 * Copyright 2010 Matteo Bruni for CodeWeavers
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

#include <assert.h>
#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/* Returns TRUE if num is a power of 2, FALSE if not, or if 0 */
static BOOL is_pow2(UINT num)
{
    return !(num & (num - 1));
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

HRESULT WINAPI D3DXFilterTexture(IDirect3DBaseTexture9 *texture,
                                 const PALETTEENTRY *palette,
                                 UINT srclevel,
                                 DWORD filter)
{
    UINT level;
    HRESULT hr;
    D3DRESOURCETYPE type;

    TRACE("texture %p, palette %p, srclevel %u, filter %#lx.\n", texture, palette, srclevel, filter);

    if (!texture)
        return D3DERR_INVALIDCALL;

    if (filter != D3DX_DEFAULT && FAILED(hr = d3dx9_validate_filter(filter)))
        return hr;

    if (srclevel == D3DX_DEFAULT)
        srclevel = 0;
    else if (srclevel >= IDirect3DBaseTexture9_GetLevelCount(texture))
        return D3DERR_INVALIDCALL;

    switch (type = IDirect3DBaseTexture9_GetType(texture))
    {
        case D3DRTYPE_TEXTURE:
        case D3DRTYPE_CUBETEXTURE:
        {
            IDirect3DSurface9 *topsurf, *mipsurf;
            D3DSURFACE_DESC desc;
            int i, numfaces;

            if (type == D3DRTYPE_TEXTURE)
            {
                numfaces = 1;
                IDirect3DTexture9_GetLevelDesc((IDirect3DTexture9*) texture, srclevel, &desc);
            }
            else
            {
                numfaces = 6;
                IDirect3DCubeTexture9_GetLevelDesc((IDirect3DCubeTexture9 *)texture, srclevel, &desc);
            }

            if (filter == D3DX_DEFAULT)
            {
                if (is_pow2(desc.Width) && is_pow2(desc.Height))
                    filter = D3DX_FILTER_BOX;
                else
                    filter = D3DX_FILTER_BOX | D3DX_FILTER_DITHER;
            }

            for (i = 0; i < numfaces; i++)
            {
                level = srclevel + 1;
                hr = get_surface(type, texture, i, srclevel, &topsurf);

                if (FAILED(hr))
                    return D3DERR_INVALIDCALL;

                while (get_surface(type, texture, i, level, &mipsurf) == D3D_OK)
                {
                    hr = D3DXLoadSurfaceFromSurface(mipsurf, palette, NULL, topsurf, palette, NULL, filter, 0);
                    IDirect3DSurface9_Release(topsurf);
                    topsurf = mipsurf;

                    if (FAILED(hr))
                        break;

                    level++;
                }

                IDirect3DSurface9_Release(topsurf);
                if (FAILED(hr))
                    return hr;
            }

            return D3D_OK;
        }

        case D3DRTYPE_VOLUMETEXTURE:
        {
            D3DVOLUME_DESC desc;
            int level, level_count;
            IDirect3DVolume9 *top_volume, *mip_volume;
            IDirect3DVolumeTexture9 *volume_texture = (IDirect3DVolumeTexture9*) texture;

            IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, srclevel, &desc);

            if (filter == D3DX_DEFAULT)
            {
                if (is_pow2(desc.Width) && is_pow2(desc.Height) && is_pow2(desc.Depth))
                    filter = D3DX_FILTER_BOX;
                else
                    filter = D3DX_FILTER_BOX | D3DX_FILTER_DITHER;
            }

            hr = IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, srclevel, &top_volume);
            if (FAILED(hr))
                return hr;

            level_count = IDirect3DVolumeTexture9_GetLevelCount(volume_texture);
            for (level = srclevel + 1; level < level_count; level++)
            {
                IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, level, &mip_volume);
                hr = D3DXLoadVolumeFromVolume(mip_volume, palette, NULL, top_volume, palette, NULL, filter, 0);
                IDirect3DVolume9_Release(top_volume);
                top_volume = mip_volume;

                if (FAILED(hr))
                    break;
            }

            IDirect3DVolume9_Release(top_volume);
            if (FAILED(hr))
                return hr;

            return D3D_OK;
        }

        default:
            return D3DERR_INVALIDCALL;
    }
}

static D3DFORMAT get_replacement_format(D3DFORMAT format)
{
    static const struct
    {
        D3DFORMAT format;
        D3DFORMAT replacement_format;
    }
    replacements[] =
    {
        {D3DFMT_L8, D3DFMT_X8R8G8B8},
        {D3DFMT_A8L8, D3DFMT_A8R8G8B8},
        {D3DFMT_A4L4, D3DFMT_A4R4G4B4},
        {D3DFMT_L16, D3DFMT_A16B16G16R16},
        {D3DFMT_DXT1, D3DFMT_A8R8G8B8},
        {D3DFMT_DXT2, D3DFMT_A8R8G8B8},
        {D3DFMT_DXT3, D3DFMT_A8R8G8B8},
        {D3DFMT_DXT4, D3DFMT_A8R8G8B8},
        {D3DFMT_DXT5, D3DFMT_A8R8G8B8},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(replacements); ++i)
        if (format == replacements[i].format)
            return replacements[i].replacement_format;
    return format;
}

static HRESULT check_texture_requirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool, D3DRESOURCETYPE resource_type)
{
    UINT w = (width && *width) ? *width : 1;
    UINT h = (height && *height) ? *height : 1;
    D3DCAPS9 caps;
    D3DDEVICE_CREATION_PARAMETERS params;
    IDirect3D9 *d3d = NULL;
    D3DDISPLAYMODE mode;
    HRESULT hr;
    D3DFORMAT usedformat = D3DFMT_UNKNOWN;
    const struct pixel_format_desc *fmt;

    if (!device)
        return D3DERR_INVALIDCALL;

    /* usage */
    if (usage == D3DX_DEFAULT)
        usage = 0;
    if (usage & (D3DUSAGE_WRITEONLY | D3DUSAGE_DONOTCLIP | D3DUSAGE_POINTS | D3DUSAGE_RTPATCHES | D3DUSAGE_NPATCHES))
        return D3DERR_INVALIDCALL;

    /* pool */
    if ((pool != D3DPOOL_DEFAULT) && (pool != D3DPOOL_MANAGED) && (pool != D3DPOOL_SYSTEMMEM) && (pool != D3DPOOL_SCRATCH))
        return D3DERR_INVALIDCALL;

    /* format */
    if (format)
    {
        TRACE("Requested format %x\n", *format);
        usedformat = *format;
    }

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d);

    if (FAILED(hr))
        goto cleanup;

    hr = IDirect3DDevice9_GetCreationParameters(device, &params);

    if (FAILED(hr))
        goto cleanup;

    hr = IDirect3DDevice9_GetDisplayMode(device, 0, &mode);

    if (FAILED(hr))
        goto cleanup;

    if ((usedformat == D3DFMT_UNKNOWN) || (usedformat == D3DX_DEFAULT))
        usedformat = D3DFMT_A8R8G8B8;

    fmt = get_format_info(usedformat);

    hr = IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType, mode.Format,
            usage, resource_type, usedformat);
    if (FAILED(hr))
    {
        BOOL allow_24bits;
        int bestscore = INT_MIN, i = 0, j;
        unsigned int channels;
        const struct pixel_format_desc *curfmt, *bestfmt = NULL;

        TRACE("Requested format is not supported, looking for a fallback.\n");

        if (!fmt)
        {
            FIXME("Pixel format %x not handled\n", usedformat);
            goto cleanup;
        }
        fmt = get_format_info(get_replacement_format(usedformat));

        allow_24bits = fmt->bytes_per_pixel == 3;
        channels = !!fmt->bits[0] + !!fmt->bits[1] + !!fmt->bits[2] + !!fmt->bits[3];
        usedformat = D3DFMT_UNKNOWN;

        while ((curfmt = get_format_info_idx(i)))
        {
            unsigned int curchannels = !!curfmt->bits[0] + !!curfmt->bits[1]
                    + !!curfmt->bits[2] + !!curfmt->bits[3];
            D3DFORMAT cur_d3dfmt;
            int score;

            i++;

            if ((cur_d3dfmt = d3dformat_from_d3dx_pixel_format_id(curfmt->format)) == D3DFMT_UNKNOWN)
                continue;
            if (curchannels < channels)
                continue;
            if (curfmt->bytes_per_pixel == 3 && !allow_24bits)
                continue;

            hr = IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                    mode.Format, usage, resource_type, cur_d3dfmt);
            if (FAILED(hr))
                continue;

            /* This format can be used, let's evaluate it.
               Weights chosen quite arbitrarily... */
            score = 512 * (format_types_match(curfmt, fmt));
            score -= 32 * (curchannels - channels);

            for (j = 0; j < 4; j++)
            {
                int diff = curfmt->bits[j] - fmt->bits[j];
                score -= (diff < 0 ? -diff * 8 : diff) * (j == 0 ? 1 : 2);
            }

            if (score > bestscore)
            {
                bestscore = score;
                usedformat = cur_d3dfmt;
                bestfmt = curfmt;
            }
        }
        if (!bestfmt)
        {
            hr = D3DERR_NOTAVAILABLE;
            goto cleanup;
        }
        fmt = bestfmt;
        hr = D3D_OK;
    }

    if (FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    if ((w == D3DX_DEFAULT) && (h == D3DX_DEFAULT))
        w = h = 256;
    else if (w == D3DX_DEFAULT)
        w = (height ? h : 256);
    else if (h == D3DX_DEFAULT)
        h = (width ? w : 256);

    assert(!(fmt->block_width & (fmt->block_width - 1)));
    assert(!(fmt->block_height & (fmt->block_height - 1)));
    if (w & (fmt->block_width - 1))
        w = (w + fmt->block_width) & ~(fmt->block_width - 1);
    if (h & (fmt->block_height - 1))
        h = (h + fmt->block_height) & ~(fmt->block_height - 1);

    if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && (!is_pow2(w)))
        w = make_pow2(w);

    if (w > caps.MaxTextureWidth)
        w = caps.MaxTextureWidth;

    if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && (!is_pow2(h)))
        h = make_pow2(h);

    if (h > caps.MaxTextureHeight)
        h = caps.MaxTextureHeight;

    if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
    {
        if (w > h)
            h = w;
        else
            w = h;
    }

    if (width)
        *width = w;

    if (height)
        *height = h;

    if (miplevels && (usage & D3DUSAGE_AUTOGENMIPMAP))
    {
        if (*miplevels > 1)
            *miplevels = 0;
    }
    else if (miplevels)
    {
        UINT max_mipmaps = 1;

        if (!width && !height)
            max_mipmaps = 9;    /* number of mipmaps in a 256x256 texture */
        else
        {
            UINT max_dimen = max(w, h);

            while (max_dimen > 1)
            {
                max_dimen >>= 1;
                max_mipmaps++;
            }
        }

        if (*miplevels == 0 || *miplevels > max_mipmaps)
            *miplevels = max_mipmaps;
    }

cleanup:

    if (d3d)
        IDirect3D9_Release(d3d);

    if (FAILED(hr))
        return hr;

    if (usedformat == D3DFMT_UNKNOWN)
    {
        WARN("Couldn't find a suitable pixel format\n");
        return D3DERR_NOTAVAILABLE;
    }

    TRACE("Format chosen: %x\n", usedformat);
    if (format)
        *format = usedformat;

    return D3D_OK;
}

HRESULT WINAPI D3DXCheckTextureRequirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
{
    TRACE("device %p, width %p, height %p, miplevels %p, usage %#lx, format %p, pool %#x.\n",
            device, width, height, miplevels, usage, format, pool);

    return check_texture_requirements(device, width, height, miplevels, usage, format, pool, D3DRTYPE_TEXTURE);
}

HRESULT WINAPI D3DXCheckCubeTextureRequirements(struct IDirect3DDevice9 *device, UINT *size,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
{
    D3DCAPS9 caps;
    UINT s = (size && *size) ? *size : 256;
    HRESULT hr;

    TRACE("device %p, size %p, miplevels %p, usage %#lx, format %p, pool %#x.\n",
            device, size, miplevels, usage, format, pool);

    if (s == D3DX_DEFAULT)
        s = 256;

    if (!device || FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
        return D3DERR_NOTAVAILABLE;

    if ((caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2) && (!is_pow2(s)))
        s = make_pow2(s);

    hr = check_texture_requirements(device, &s, &s, miplevels, usage, format, pool, D3DRTYPE_CUBETEXTURE);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP))
    {
        if(miplevels)
            *miplevels = 1;
    }

    if (size)
        *size = s;

    return hr;
}

HRESULT WINAPI D3DXCheckVolumeTextureRequirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *depth, UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
{
    D3DCAPS9 caps;
    UINT w = width ? *width : D3DX_DEFAULT;
    UINT h = height ? *height : D3DX_DEFAULT;
    UINT d = (depth && *depth) ? *depth : 1;
    HRESULT hr;

    TRACE("device %p, width %p, height %p, depth %p, miplevels %p, usage %#lx, format %p, pool %#x.\n",
            device, width, height, depth, miplevels, usage, format, pool);

    if (!device || FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
        return D3DERR_NOTAVAILABLE;

    hr = check_texture_requirements(device, &w, &h, NULL, usage, format, pool, D3DRTYPE_VOLUMETEXTURE);
    if (d == D3DX_DEFAULT)
        d = 1;

    if ((caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2) &&
        (!is_pow2(w) || !is_pow2(h) || !is_pow2(d)))
    {
        w = make_pow2(w);
        h = make_pow2(h);
        d = make_pow2(d);
    }

    if (w > caps.MaxVolumeExtent)
        w = caps.MaxVolumeExtent;
    if (h > caps.MaxVolumeExtent)
        h = caps.MaxVolumeExtent;
    if (d > caps.MaxVolumeExtent)
        d = caps.MaxVolumeExtent;

    if (miplevels)
    {
        if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
            *miplevels = 1;
        else if ((usage & D3DUSAGE_AUTOGENMIPMAP))
        {
            if (*miplevels > 1)
                *miplevels = 0;
        }
        else
        {
            UINT max_mipmaps = 1;
            UINT max_dimen = max(max(w, h), d);

            while (max_dimen > 1)
            {
                max_dimen >>= 1;
                max_mipmaps++;
            }

            if (*miplevels == 0 || *miplevels > max_mipmaps)
                *miplevels = max_mipmaps;
        }
    }

    if (width)
        *width = w;
    if (height)
        *height = h;
    if (depth)
        *depth = d;

    return hr;
}

HRESULT WINAPI D3DXCreateTexture(struct IDirect3DDevice9 *device, UINT width, UINT height,
        UINT miplevels, DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DTexture9 **texture)
{
    HRESULT hr;

    TRACE("device %p, width %u, height %u, miplevels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            device, width, height, miplevels, usage, format, pool, texture);

    if (!device || !texture)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = D3DXCheckTextureRequirements(device, &width, &height, &miplevels, usage, &format, pool)))
        return hr;

    return IDirect3DDevice9_CreateTexture(device, width, height, miplevels, usage, format, pool, texture, NULL);
}

static D3DFORMAT get_alpha_replacement_format(D3DFORMAT format)
{
    static const struct
    {
        D3DFORMAT orig_format;
        D3DFORMAT replacement_format;
    }
    replacement_formats[] =
    {
        {D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8},
        {D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5},
        {D3DFMT_X4R4G4B4, D3DFMT_A4R4G4B4},
        {D3DFMT_X8B8G8R8, D3DFMT_A8B8G8R8},
        {D3DFMT_L8, D3DFMT_A8L8},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(replacement_formats); ++i)
        if (replacement_formats[i].orig_format == format)
            return replacement_formats[i].replacement_format;
    return format;
}

static uint32_t d3dx9_get_mip_filter_value(uint32_t mip_filter, uint32_t *skip_levels)
{
    uint32_t filter = (mip_filter == D3DX_DEFAULT) ? D3DX_FILTER_BOX : mip_filter & ~D3DX9_FILTER_INVALID_BITS;

    *skip_levels = mip_filter != D3DX_DEFAULT ? mip_filter >> D3DX_SKIP_DDS_MIP_LEVELS_SHIFT : 0;
    *skip_levels &= D3DX_SKIP_DDS_MIP_LEVELS_MASK;
    if (!(filter & 0x7) || (filter & 0x7) > D3DX_FILTER_BOX)
        filter = (filter & 0x007f0000) | D3DX_FILTER_BOX;
    return filter;
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(struct IDirect3DDevice9 *device, const void *srcdata,
        UINT srcdatasize, UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    const struct pixel_format_desc *src_fmt_desc, *dst_fmt_desc;
    BOOL dynamic_texture, format_specified = FALSE;
    uint32_t loaded_miplevels, skip_levels, i;
    IDirect3DTexture9 *staging_tex, *tex;
    struct d3dx_image image;
    D3DXIMAGE_INFO imginfo;
    D3DCAPS9 caps;
    HRESULT hr;

    TRACE("device %p, srcdata %p, srcdatasize %u, width %u, height %u, miplevels %u, "
            "usage %#lx, format %#x, pool %#x, filter %#lx, mipfilter %#lx, colorkey %#lx, "
            "srcinfo %p, palette %p, texture %p.\n",
            device, srcdata, srcdatasize, width, height, miplevels, usage, format, pool,
            filter, mipfilter, colorkey, srcinfo, palette, texture);

    /* check for invalid parameters */
    if (!device || !texture || !srcdata || !srcdatasize)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    staging_tex = tex = *texture = NULL;
    mipfilter = d3dx9_get_mip_filter_value(mipfilter, &skip_levels);
    hr = d3dx_image_init(srcdata, srcdatasize, &image, skip_levels, 0);
    if (FAILED(hr))
    {
        FIXME("Unrecognized file format, returning failure.\n");
        return hr;
    }

    d3dximage_info_from_d3dx_image(&imginfo, &image);

    /* handle default values */
    if (!width || width == D3DX_DEFAULT_NONPOW2 || width == D3DX_FROM_FILE || width == D3DX_DEFAULT)
        width = (width == D3DX_DEFAULT) ? make_pow2(imginfo.Width) : imginfo.Width;
    if (!height || height == D3DX_DEFAULT_NONPOW2 || height == D3DX_FROM_FILE || height == D3DX_DEFAULT)
        height = (height == D3DX_DEFAULT) ? make_pow2(imginfo.Height) : imginfo.Height;

    format_specified = (format != D3DFMT_UNKNOWN && format != D3DX_DEFAULT);
    if (format == D3DFMT_FROM_FILE || format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = imginfo.Format;
    miplevels = (miplevels == D3DX_FROM_FILE) ? imginfo.MipLevels : miplevels;

    /* Fix up texture creation parameters. */
    hr = D3DXCheckTextureRequirements(device, &width, &height, &miplevels, usage, &format, pool);
    if (FAILED(hr))
    {
        FIXME("Couldn't find suitable texture parameters.\n");
        goto err;
    }

    if (colorkey && !format_specified)
        format = get_alpha_replacement_format(format);

    if (FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
    {
        hr = D3DERR_INVALIDCALL;
        goto err;
    }

    /* Create the to-be-filled texture */
    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        TRACE("Creating staging texture.\n");
        hr = D3DXCreateTexture(device, width, height, miplevels, 0, format, D3DPOOL_SYSTEMMEM, &staging_tex);
        tex = staging_tex;
    }
    else
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, &tex);
    }

    if (FAILED(hr))
    {
        FIXME("Texture creation failed.\n");
        goto err;
    }

    TRACE("Texture created correctly. Now loading the texture data into it.\n");
    dst_fmt_desc = get_format_info(format);
    src_fmt_desc = get_d3dx_pixel_format_info(image.format);
    loaded_miplevels = min(imginfo.MipLevels, IDirect3DTexture9_GetLevelCount(tex));
    for (i = 0; i < loaded_miplevels; i++)
    {
        struct d3dx_pixels src_pixels, dst_pixels;
        D3DSURFACE_DESC dst_surface_desc;
        D3DLOCKED_RECT dst_locked_rect;
        RECT dst_rect;

        hr = d3dx_image_get_pixels(&image, 0, i, &src_pixels);
        if (FAILED(hr))
            break;

        hr = IDirect3DTexture9_LockRect(tex, i, &dst_locked_rect, NULL, 0);
        if (FAILED(hr))
            break;

        IDirect3DTexture9_GetLevelDesc(tex, i, &dst_surface_desc);
        SetRect(&dst_rect, 0, 0, dst_surface_desc.Width, dst_surface_desc.Height);
        set_d3dx_pixels(&dst_pixels, dst_locked_rect.pBits, dst_locked_rect.Pitch, 0, palette,
                dst_surface_desc.Width, dst_surface_desc.Height, 1, &dst_rect);

        hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_fmt_desc, &src_pixels, src_fmt_desc, filter, colorkey);
        IDirect3DTexture9_UnlockRect(tex, i);
        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
    {
        FIXME("Texture loading failed.\n");
        goto err;
    }

    hr = D3DXFilterTexture((IDirect3DBaseTexture9 *)tex, palette, loaded_miplevels - 1, mipfilter);
    if (FAILED(hr))
    {
        FIXME("Texture filtering failed.\n");
        goto err;
    }

    /* Move the data to the actual texture if necessary */
    if (staging_tex)
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, texture);
        if (FAILED(hr))
            goto err;

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)staging_tex, (IDirect3DBaseTexture9 *)(*texture));
        IDirect3DTexture9_Release(staging_tex);
    }
    else
    {
        *texture = tex;
    }

    d3dx_image_cleanup(&image);
    if (srcinfo)
        *srcinfo = imginfo;

    return hr;

err:
    d3dx_image_cleanup(&image);
    if (tex)
        IDirect3DTexture9_Release(tex);

    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemory(struct IDirect3DDevice9 *device,
        const void *srcdata, UINT srcdatasize, struct IDirect3DTexture9 **texture)
{
    TRACE("device %p, srcdata %p, srcdatasize %u, texture %p.\n", device, srcdata, srcdatasize, texture);

    return D3DXCreateTextureFromFileInMemoryEx(device, srcdata, srcdatasize, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                               D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromFileExW(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("device %p, srcfile %s, width %u, height %u, miplevels %u, usage %#lx, format %#x, "
            "pool %#x, filter %#lx, mipfilter %#lx, colorkey 0x%08lx, srcinfo %p, palette %p, texture %p.\n",
            device, debugstr_w(srcfile), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    hr = map_view_of_file(srcfile, &buffer, &size);
    if (FAILED(hr))
    {
        WARN("Failed to open file.\n");
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXCreateTextureFromFileInMemoryEx(device, buffer, size, width, height, miplevels, usage, format, pool,
        filter, mipfilter, colorkey, srcinfo, palette, texture);

    UnmapViewOfFile(buffer);

    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileExA(struct IDirect3DDevice9 *device, const char *srcfile,
        UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    WCHAR *widename;
    HRESULT hr;
    DWORD len;

    TRACE("device %p, srcfile %s, width %u, height %u, miplevels %u, usage %#lx, format %#x, "
            "pool %#x, filter %#lx, mipfilter %#lx, colorkey 0x%08lx, srcinfo %p, palette %p, texture %p.\n",
            device, debugstr_a(srcfile), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!device || !srcfile || !texture)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    widename = malloc(len * sizeof(*widename));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, widename, len);

    hr = D3DXCreateTextureFromFileExW(device, widename, width, height, miplevels,
                                      usage, format, pool, filter, mipfilter,
                                      colorkey, srcinfo, palette, texture);

    free(widename);
    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileA(struct IDirect3DDevice9 *device,
        const char *srcfile, struct IDirect3DTexture9 **texture)
{
    TRACE("device %p, srcfile %s, texture %p.\n", device, debugstr_a(srcfile), texture);

    return D3DXCreateTextureFromFileExA(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromFileW(struct IDirect3DDevice9 *device,
        const WCHAR *srcfile, struct IDirect3DTexture9 **texture)
{
    TRACE("device %p, srcfile %s, texture %p.\n", device, debugstr_w(srcfile), texture);

    return D3DXCreateTextureFromFileExW(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}


HRESULT WINAPI D3DXCreateTextureFromResourceA(struct IDirect3DDevice9 *device,
        HMODULE srcmodule, const char *resource, struct IDirect3DTexture9 **texture)
{
    TRACE("device %p, srcmodule %p, resource %s, texture %p.\n",
            device, srcmodule, debugstr_a(resource), texture);

    return D3DXCreateTextureFromResourceExA(device, srcmodule, resource, D3DX_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL,
            NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromResourceW(struct IDirect3DDevice9 *device,
        HMODULE srcmodule, const WCHAR *resource, struct IDirect3DTexture9 **texture)
{
    TRACE("device %p, srcmodule %p, resource %s, texture %p.\n",
            device, srcmodule, debugstr_w(resource), texture);

    return D3DXCreateTextureFromResourceExW(device, srcmodule, resource, D3DX_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL,
            NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromResourceExA(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *resource, UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("device %p, srcmodule %p, resource %s, width %u, height %u, miplevels %u, usage %#lx, format %#x, "
            "pool %#x, filter %#lx, mipfilter %#lx, colorkey 0x%08lx, srcinfo %p, palette %p, texture %p.\n",
            device, srcmodule, debugstr_a(resource), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!device || !texture)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceA(srcmodule, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(srcmodule, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateTextureFromFileInMemoryEx(device, buffer, size, width, height, miplevels,
            usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);
}

HRESULT WINAPI D3DXCreateTextureFromResourceExW(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const WCHAR *resource, UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("device %p, srcmodule %p, resource %s, width %u, height %u, miplevels %u, usage %#lx, format %#x, "
            "pool %#x, filter %#lx, mipfilter %#lx, colorkey 0x%08lx, srcinfo %p, palette %p, texture %p.\n",
            device, srcmodule, debugstr_w(resource), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!device || !texture)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceW(srcmodule, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(srcmodule, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateTextureFromFileInMemoryEx(device, buffer, size, width, height, miplevels,
            usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);
}

HRESULT WINAPI D3DXCreateCubeTexture(struct IDirect3DDevice9 *device, UINT size, UINT miplevels,
        DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DCubeTexture9 **texture)
{
    HRESULT hr;

    TRACE("device %p, size %u, miplevels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            device, size, miplevels, usage, format, pool, texture);

    if (!device || !texture)
        return D3DERR_INVALIDCALL;

    hr = D3DXCheckCubeTextureRequirements(device, &size, &miplevels, usage, &format, pool);

    if (FAILED(hr))
    {
        TRACE("D3DXCheckCubeTextureRequirements failed\n");
        return hr;
    }

    return IDirect3DDevice9_CreateCubeTexture(device, size, miplevels, usage, format, pool, texture, NULL);
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemory(struct IDirect3DDevice9 *device,
        const void *data, UINT datasize, struct IDirect3DCubeTexture9 **texture)
{
    TRACE("device %p, data %p, datasize %u, texture %p.\n", device, data, datasize, texture);

    return D3DXCreateCubeTextureFromFileInMemoryEx(device, data, datasize, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateVolumeTexture(struct IDirect3DDevice9 *device, UINT width, UINT height, UINT depth,
        UINT miplevels, DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DVolumeTexture9 **texture)
{
    HRESULT hr;

    TRACE("device %p, width %u, height %u, depth %u, miplevels %u, usage %#lx, format %#x, pool %#x, texture %p.\n",
            device, width, height, depth, miplevels, usage, format, pool, texture);

    if (!device || !texture)
        return D3DERR_INVALIDCALL;

    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth,
                                            &miplevels, usage, &format, pool);

    if (FAILED(hr))
    {
        TRACE("D3DXCheckVolumeTextureRequirements failed\n");
        return hr;
    }

    return IDirect3DDevice9_CreateVolumeTexture(device, width, height, depth, miplevels,
                                                usage, format, pool, texture, NULL);
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileA(IDirect3DDevice9 *device,
                                                const char *filename,
                                                IDirect3DVolumeTexture9 **volume_texture)
{
    int len;
    HRESULT hr;
    void *data;
    DWORD data_size;
    WCHAR *filenameW;

    TRACE("device %p, filename %s, volume_texture %p.\n", device, debugstr_a(filename), volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = malloc(len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = map_view_of_file(filenameW, &data, &data_size);
    free(filenameW);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, volume_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileW(IDirect3DDevice9 *device,
                                                const WCHAR *filename,
                                                IDirect3DVolumeTexture9 **volume_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("device %p, filename %s, volume_texture %p.\n", device, debugstr_w(filename), volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, volume_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileExA(IDirect3DDevice9 *device,
                                                  const char *filename,
                                                  UINT width,
                                                  UINT height,
                                                  UINT depth,
                                                  UINT mip_levels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mip_filter,
                                                  D3DCOLOR color_key,
                                                  D3DXIMAGE_INFO *src_info,
                                                  PALETTEENTRY *palette,
                                                  IDirect3DVolumeTexture9 **volume_texture)
{
    int len;
    HRESULT hr;
    WCHAR *filenameW;
    void *data;
    DWORD data_size;

    TRACE("device %p, filename %s, width %u, height %u, depth %u, mip_levels %u, usage %#lx, "
            "format %#x, pool %#x, filter %#lx, mip_filter %#lx, color_key 0x%08lx, src_info %p, "
            "palette %p, volume_texture %p.\n",
            device, debugstr_a(filename), width, height, depth, mip_levels, usage,
            format, pool, filter, mip_filter, color_key, src_info, palette, volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = malloc(len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = map_view_of_file(filenameW, &data, &data_size);
    free(filenameW);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, width, height, depth,
            mip_levels, usage, format, pool, filter, mip_filter, color_key, src_info, palette,
            volume_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileExW(IDirect3DDevice9 *device,
                                                  const WCHAR *filename,
                                                  UINT width,
                                                  UINT height,
                                                  UINT depth,
                                                  UINT mip_levels,
                                                  DWORD usage,
                                                  D3DFORMAT format,
                                                  D3DPOOL pool,
                                                  DWORD filter,
                                                  DWORD mip_filter,
                                                  D3DCOLOR color_key,
                                                  D3DXIMAGE_INFO *src_info,
                                                  PALETTEENTRY *palette,
                                                  IDirect3DVolumeTexture9 **volume_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("device %p, filename %s, width %u, height %u, depth %u, mip_levels %u, usage %#lx, "
            "format %#x, pool %#x, filter %#lx, mip_filter %#lx, color_key 0x%08lx, src_info %p, "
            "palette %p, volume_texture %p.\n",
            device, debugstr_w(filename), width, height, depth, mip_levels, usage,
            format, pool, filter, mip_filter, color_key, src_info, palette, volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, width, height, depth,
            mip_levels, usage, format, pool, filter, mip_filter, color_key, src_info, palette,
            volume_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemory(IDirect3DDevice9 *device,
                                                       const void *data,
                                                       UINT data_size,
                                                       IDirect3DVolumeTexture9 **volume_texture)
{
    TRACE("device %p, data %p, data_size %u, volume_texture %p.\n", device, data, data_size, volume_texture);

    return D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT,
        0, NULL, NULL, volume_texture);
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemoryEx(IDirect3DDevice9 *device, const void *data,
        UINT data_size, UINT width, UINT height, UINT depth, UINT mip_levels, DWORD usage,
        D3DFORMAT format, D3DPOOL pool, DWORD filter, DWORD mip_filter, D3DCOLOR color_key,
        D3DXIMAGE_INFO *info, PALETTEENTRY *palette, IDirect3DVolumeTexture9 **volume_texture)
{
    HRESULT hr;
    D3DCAPS9 caps;
    struct d3dx_image image;
    D3DXIMAGE_INFO image_info;
    uint32_t loaded_miplevels, skip_levels, i;
    IDirect3DVolumeTexture9 *tex, *staging_tex;
    BOOL dynamic_texture, format_specified = FALSE;
    const struct pixel_format_desc *src_fmt_desc, *dst_fmt_desc;

    TRACE("device %p, data %p, data_size %u, width %u, height %u, depth %u, mip_levels %u, "
            "usage %#lx, format %#x, pool %#x, filter %#lx, mip_filter %#lx, color_key 0x%08lx, "
            "info %p, palette %p, volume_texture %p.\n",
            device, data, data_size, width, height, depth, mip_levels, usage, format, pool,
            filter, mip_filter, color_key, info, palette, volume_texture);

    if (!device || !data || !data_size || !volume_texture)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    staging_tex = tex = *volume_texture = NULL;
    mip_filter = d3dx9_get_mip_filter_value(mip_filter, &skip_levels);
    hr = d3dx_image_init(data, data_size, &image, skip_levels, 0);
    if (FAILED(hr))
    {
        FIXME("Unrecognized file format, returning failure.\n");
        return hr;
    }

    d3dximage_info_from_d3dx_image(&image_info, &image);

    /* Handle default values. */
    if (!width || width == D3DX_DEFAULT_NONPOW2 || width == D3DX_FROM_FILE || width == D3DX_DEFAULT)
        width = (width == D3DX_DEFAULT) ? make_pow2(image_info.Width) : image_info.Width;
    if (!height || height == D3DX_DEFAULT_NONPOW2 || height == D3DX_FROM_FILE || height == D3DX_DEFAULT)
        height = (height == D3DX_DEFAULT) ? make_pow2(image_info.Height) : image_info.Height;
    if (!depth || depth == D3DX_DEFAULT_NONPOW2 || depth == D3DX_FROM_FILE || depth == D3DX_DEFAULT)
        depth = (depth == D3DX_DEFAULT) ? make_pow2(image_info.Depth) : image_info.Depth;

    format_specified = (format != D3DFMT_UNKNOWN && format != D3DX_DEFAULT);
    if (format == D3DFMT_FROM_FILE || format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
         format = image_info.Format;
    mip_levels = (mip_levels == D3DX_FROM_FILE) ? image_info.MipLevels : mip_levels;

    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, &mip_levels, usage, &format, pool);
    if (FAILED(hr))
    {
        FIXME("Couldn't find suitable texture parameters.\n");
        goto err;
    }

    if (color_key && !format_specified)
        format = get_alpha_replacement_format(format);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr))
    {
        hr = D3DERR_INVALIDCALL;
        goto err;
    }

    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        TRACE("Creating staging texture.\n");
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, 0, format, D3DPOOL_SYSTEMMEM, &staging_tex);
        tex = staging_tex;
    }
    else
    {
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, usage, format, pool, &tex);
    }

    if (FAILED(hr))
    {
        FIXME("Texture creation failed.\n");
        goto err;
    }

    TRACE("Texture created correctly. Now loading the texture data into it.\n");
    dst_fmt_desc = get_format_info(format);
    src_fmt_desc = get_d3dx_pixel_format_info(image.format);
    loaded_miplevels = min(image_info.MipLevels, IDirect3DVolumeTexture9_GetLevelCount(tex));
    for (i = 0; i < loaded_miplevels; i++)
    {
        struct d3dx_pixels src_pixels, dst_pixels;
        D3DVOLUME_DESC dst_volume_desc;
        D3DLOCKED_BOX dst_locked_box;
        RECT dst_rect;

        hr = d3dx_image_get_pixels(&image, 0, i, &src_pixels);
        if (FAILED(hr))
            break;

        hr = IDirect3DVolumeTexture9_LockBox(tex, i, &dst_locked_box, NULL, 0);
        if (FAILED(hr))
            break;

        IDirect3DVolumeTexture9_GetLevelDesc(tex, i, &dst_volume_desc);
        SetRect(&dst_rect, 0, 0, dst_volume_desc.Width, dst_volume_desc.Height);
        set_d3dx_pixels(&dst_pixels, dst_locked_box.pBits, dst_locked_box.RowPitch, dst_locked_box.SlicePitch, palette,
                dst_volume_desc.Width, dst_volume_desc.Height, dst_volume_desc.Depth, &dst_rect);

        hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_fmt_desc, &src_pixels, src_fmt_desc, filter, color_key);
        IDirect3DVolumeTexture9_UnlockBox(tex, i);
        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
    {
        FIXME("Texture loading failed.\n");
        goto err;
    }

    hr = D3DXFilterTexture((IDirect3DBaseTexture9 *)tex, palette, loaded_miplevels - 1, mip_filter);
    if (FAILED(hr))
    {
        FIXME("Texture filtering failed.\n");
        goto err;
    }

    /* Move the data to the actual texture if necessary. */
    if (staging_tex)
    {
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, usage, format, pool, volume_texture);
        if (FAILED(hr))
            goto err;

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)staging_tex, (IDirect3DBaseTexture9 *)(*volume_texture));
        IDirect3DVolumeTexture9_Release(staging_tex);
    }
    else
    {
        *volume_texture = tex;
    }

    d3dx_image_cleanup(&image);
    if (info)
        *info = image_info;

    return hr;

err:
    d3dx_image_cleanup(&image);
    if (tex)
        IDirect3DVolumeTexture9_Release(tex);

    return hr;
}

HRESULT WINAPI D3DXFillTexture(struct IDirect3DTexture9 *texture, LPD3DXFILL2D function, void *funcdata)
{
    IDirect3DSurface9 *surface, *temp_surface;
    DWORD miplevels;
    DWORD m, x, y;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lock_rect;
    D3DXVECTOR4 value;
    D3DXVECTOR2 coord, size;
    const struct pixel_format_desc *format;
    BYTE *data;
    HRESULT hr;

    TRACE("texture %p, function %p, funcdata %p.\n", texture, function, funcdata);

    if (!texture || !function)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(hr = IDirect3DTexture9_GetLevelDesc(texture, m, &desc)))
            return hr;

        format = get_format_info(desc.Format);
        if (is_unknown_format(format) || is_index_format(format) || is_compressed_format(format))
        {
            FIXME("Unsupported texture format %#x.\n", desc.Format);
            return D3DERR_INVALIDCALL;
        }

        if (FAILED(hr = IDirect3DTexture9_GetSurfaceLevel(texture, m, &surface)))
            return hr;
        if (FAILED(hr = lock_surface(surface, NULL, &lock_rect, &temp_surface, TRUE)))
        {
            IDirect3DSurface9_Release(surface);
            return hr;
        }

        size.x = 1.0f / desc.Width;
        size.y = 1.0f / desc.Height;

        data = lock_rect.pBits;

        for (y = 0; y < desc.Height; y++)
        {
            /* The callback function expects the coordinates of the center
               of the texel */
            coord.y = (y + 0.5f) / desc.Height;

            for (x = 0; x < desc.Width; x++)
            {
                BYTE *dst = data + y * lock_rect.Pitch + x * format->bytes_per_pixel;
                struct d3dx_color color;

                coord.x = (x + 0.5f) / desc.Width;

                function(&value, &coord, &size, funcdata);

                set_d3dx_color(&color, (const struct vec4 *)&value, RANGE_FULL, RANGE_FULL);
                format_from_d3dx_color(format, &color, dst);
            }
        }
        if (FAILED(hr = unlock_surface(surface, NULL, temp_surface, TRUE)))
        {
            IDirect3DSurface9_Release(surface);
            return hr;
        }
        IDirect3DSurface9_Release(surface);
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemoryEx(IDirect3DDevice9 *device, const void *src_data,
        UINT src_data_size, UINT size, UINT mip_levels, DWORD usage, D3DFORMAT format, D3DPOOL pool,
        DWORD filter, DWORD mip_filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info,
        PALETTEENTRY *palette, IDirect3DCubeTexture9 **cube_texture)
{
    const struct pixel_format_desc *src_fmt_desc, *dst_fmt_desc;
    BOOL dynamic_texture, format_specified = FALSE;
    uint32_t loaded_miplevels, skip_levels, i;
    IDirect3DCubeTexture9 *tex, *staging_tex;
    struct d3dx_image image;
    D3DXIMAGE_INFO img_info;
    D3DCAPS9 caps;
    HRESULT hr;

    TRACE("device %p, src_data %p, src_data_size %u, size %u, mip_levels %u, usage %#lx, "
            "format %#x, pool %#x, filter %#lx, mip_filter %#lx, color_key 0x%08lx, src_info %p, "
            "palette %p, cube_texture %p.\n",
            device, src_data, src_data_size, size, mip_levels, usage, format, pool, filter,
            mip_filter, color_key, src_info, palette, cube_texture);

    if (!device || !cube_texture || !src_data || !src_data_size)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    staging_tex = tex = *cube_texture = NULL;
    mip_filter = d3dx9_get_mip_filter_value(mip_filter, &skip_levels);
    hr = d3dx_image_init(src_data, src_data_size, &image, skip_levels, 0);
    if (FAILED(hr))
    {
        FIXME("Unrecognized file format, returning failure.\n");
        return hr;
    }

    d3dximage_info_from_d3dx_image(&img_info, &image);
    if (img_info.ResourceType != D3DRTYPE_CUBETEXTURE)
    {
        hr = E_FAIL;
        goto err;
    }

    /* Handle default values. */
    if (!size || size == D3DX_DEFAULT_NONPOW2 || size == D3DX_FROM_FILE)
        size = max(img_info.Width, img_info.Height);
    else if (size == D3DX_DEFAULT)
        size = make_pow2(max(img_info.Width, img_info.Height));

    format_specified = (format != D3DFMT_UNKNOWN && format != D3DX_DEFAULT);
    if (format == D3DFMT_FROM_FILE || format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = img_info.Format;
    mip_levels = (mip_levels == D3DX_FROM_FILE) ? img_info.MipLevels : mip_levels;

    hr = D3DXCheckCubeTextureRequirements(device, &size, &mip_levels, usage, &format, pool);
    if (FAILED(hr))
    {
        FIXME("Couldn't find suitable texture parameters.\n");
        goto err;
    }

    if (color_key && !format_specified)
        format = get_alpha_replacement_format(format);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr))
    {
        hr = D3DERR_INVALIDCALL;
        goto err;
    }

    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        TRACE("Creating staging texture.\n");
        hr = D3DXCreateCubeTexture(device, size, mip_levels, 0, format, D3DPOOL_SYSTEMMEM, &staging_tex);
        tex = staging_tex;
    }
    else
    {
        hr = D3DXCreateCubeTexture(device, size, mip_levels, usage, format, pool, &tex);
    }

    if (FAILED(hr))
    {
        FIXME("Texture creation failed.\n");
        goto err;
    }

    TRACE("Texture created correctly. Now loading the texture data into it.\n");
    dst_fmt_desc = get_format_info(format);
    src_fmt_desc = get_d3dx_pixel_format_info(image.format);
    loaded_miplevels = min(img_info.MipLevels, IDirect3DCubeTexture9_GetLevelCount(tex));
    for (i = 0; i < loaded_miplevels; ++i)
    {
        struct d3dx_pixels src_pixels, dst_pixels;
        D3DSURFACE_DESC dst_surface_desc;
        D3DLOCKED_RECT dst_locked_rect;
        RECT dst_rect;
        uint32_t face;

        IDirect3DCubeTexture9_GetLevelDesc(tex, i, &dst_surface_desc);
        SetRect(&dst_rect, 0, 0, dst_surface_desc.Width, dst_surface_desc.Height);
        for (face = D3DCUBEMAP_FACE_POSITIVE_X; face <= D3DCUBEMAP_FACE_NEGATIVE_Z; ++face)
        {
            hr = d3dx_image_get_pixels(&image, face, i, &src_pixels);
            if (FAILED(hr))
                break;

            hr = IDirect3DCubeTexture9_LockRect(tex, face, i, &dst_locked_rect, NULL, 0);
            if (FAILED(hr))
                break;

            set_d3dx_pixels(&dst_pixels, dst_locked_rect.pBits, dst_locked_rect.Pitch, 0, palette,
                    dst_surface_desc.Width, dst_surface_desc.Height, 1, &dst_rect);

            hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_fmt_desc, &src_pixels, src_fmt_desc, filter, color_key);
            IDirect3DCubeTexture9_UnlockRect(tex, face, i);
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
    {
        FIXME("Texture loading failed.\n");
        goto err;
    }

    hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, palette, loaded_miplevels - 1, mip_filter);
    if (FAILED(hr))
    {
        FIXME("Texture filtering failed.\n");
        goto err;
    }

    if (staging_tex)
    {
        hr = D3DXCreateCubeTexture(device, size, mip_levels, usage, format, pool, cube_texture);
        if (FAILED(hr))
            goto err;

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)staging_tex, (IDirect3DBaseTexture9 *)(*cube_texture));
        IDirect3DCubeTexture9_Release(staging_tex);
    }
    else
    {
        *cube_texture = tex;
    }

    d3dx_image_cleanup(&image);
    if (src_info)
        *src_info = img_info;

    return hr;

err:
    d3dx_image_cleanup(&image);
    if (tex)
        IDirect3DCubeTexture9_Release(tex);

    return hr;
}


HRESULT WINAPI D3DXCreateCubeTextureFromFileA(IDirect3DDevice9 *device,
                                              const char *src_filename,
                                              IDirect3DCubeTexture9 **cube_texture)
{
    int len;
    HRESULT hr;
    WCHAR *filename;
    void *data;
    DWORD data_size;

    TRACE("device %p, src_filename %s, cube_texture %p.\n", device, wine_dbgstr_a(src_filename), cube_texture);

    if (!src_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, src_filename, -1, NULL, 0);
    filename = malloc(len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, src_filename, -1, filename, len);

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr))
    {
        free(filename);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, cube_texture);

    UnmapViewOfFile(data);
    free(filename);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileW(IDirect3DDevice9 *device,
                                              const WCHAR *src_filename,
                                              IDirect3DCubeTexture9 **cube_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("device %p, src_filename %s, cube_texture %p.\n", device, wine_dbgstr_w(src_filename), cube_texture);

    hr = map_view_of_file(src_filename, &data, &data_size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, cube_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileExA(IDirect3DDevice9 *device, const char *src_filename,
        UINT size, UINT mip_levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, DWORD filter,
        DWORD mip_filter, D3DCOLOR color_key, D3DXIMAGE_INFO *image_info, PALETTEENTRY *palette,
        IDirect3DCubeTexture9 **cube_texture)
{
    int len;
    HRESULT hr;
    WCHAR *filename;
    void *data;
    DWORD data_size;

    TRACE("device %p, src_filename %s, size %u, mip_levels %u, usage %#lx, format %#x, pool %#x, "
            "filter %#lx, mip_filter %#lx, color_key 0x%08lx, image_info %p, palette %p, cube_texture %p.\n",
            device, wine_dbgstr_a(src_filename), size, mip_levels, usage, format, pool,
            filter, mip_filter, color_key, image_info, palette, cube_texture);

    if (!src_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, src_filename, -1, NULL, 0);
    filename = malloc(len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, src_filename, -1, filename, len);

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr))
    {
        free(filename);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, size, mip_levels,
        usage, format, pool, filter, mip_filter, color_key, image_info, palette, cube_texture);

    UnmapViewOfFile(data);
    free(filename);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileExW(IDirect3DDevice9 *device, const WCHAR *src_filename,
        UINT size, UINT mip_levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, DWORD filter,
        DWORD mip_filter, D3DCOLOR color_key, D3DXIMAGE_INFO *image_info, PALETTEENTRY *palette,
        IDirect3DCubeTexture9 **cube_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("device %p, src_filename %s, size %u, mip_levels %u, usage %#lx, format %#x, pool %#x, "
            "filter %#lx, mip_filter %#lx, color_key 0x%08lx, image_info %p, palette %p, cube_texture %p.\n",
            device, wine_dbgstr_w(src_filename), size, mip_levels, usage, format,
            pool, filter, mip_filter, color_key, image_info, palette, cube_texture);

    hr = map_view_of_file(src_filename, &data, &data_size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, size, mip_levels,
        usage, format, pool, filter, mip_filter, color_key, image_info, palette, cube_texture);

    UnmapViewOfFile(data);
    return hr;
}

enum cube_coord
{
    XCOORD = 0,
    XCOORDINV = 1,
    YCOORD = 2,
    YCOORDINV = 3,
    ZERO = 4,
    ONE = 5
};

static float get_cube_coord(enum cube_coord coord, unsigned int x, unsigned int y, unsigned int size)
{
    switch (coord)
    {
        case XCOORD:
            return x + 0.5f;
        case XCOORDINV:
            return size - x - 0.5f;
        case YCOORD:
            return y + 0.5f;
        case YCOORDINV:
            return size - y - 0.5f;
        case ZERO:
            return 0.0f;
        case ONE:
            return size;
       default:
           ERR("Unexpected coordinate value\n");
           return 0.0f;
    }
}

HRESULT WINAPI D3DXFillCubeTexture(struct IDirect3DCubeTexture9 *texture, LPD3DXFILL3D function, void *funcdata)
{
    DWORD miplevels;
    DWORD m, x, y, f;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lock_rect;
    D3DXVECTOR4 value;
    D3DXVECTOR3 coord, size;
    const struct pixel_format_desc *format;
    BYTE *data;
    static const enum cube_coord coordmap[6][3] =
        {
            {ONE, YCOORDINV, XCOORDINV},
            {ZERO, YCOORDINV, XCOORD},
            {XCOORD, ONE, YCOORD},
            {XCOORD, ZERO, YCOORDINV},
            {XCOORD, YCOORDINV, ONE},
            {XCOORDINV, YCOORDINV, ZERO}
        };

    TRACE("texture %p, function %p, funcdata %p.\n", texture, function, funcdata);

    if (texture == NULL || function == NULL)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DCubeTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(IDirect3DCubeTexture9_GetLevelDesc(texture, m, &desc)))
            return D3DERR_INVALIDCALL;

        format = get_format_info(desc.Format);
        if (is_unknown_format(format) || is_index_format(format) || is_compressed_format(format))
        {
            FIXME("Unsupported texture format %#x\n", desc.Format);
            return D3DERR_INVALIDCALL;
        }

        for (f = 0; f < 6; f++)
        {
            if (FAILED(IDirect3DCubeTexture9_LockRect(texture, f, m, &lock_rect, NULL, D3DLOCK_DISCARD)))
                return D3DERR_INVALIDCALL;

            size.x = (f == 0) || (f == 1) ? 0.0f : 2.0f / desc.Width;
            size.y = (f == 2) || (f == 3) ? 0.0f : 2.0f / desc.Width;
            size.z = (f == 4) || (f == 5) ? 0.0f : 2.0f / desc.Width;

            data = lock_rect.pBits;

            for (y = 0; y < desc.Height; y++)
            {
                for (x = 0; x < desc.Width; x++)
                {
                    BYTE *dst = data + y * lock_rect.Pitch + x * format->bytes_per_pixel;
                    struct d3dx_color color;

                    coord.x = get_cube_coord(coordmap[f][0], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;
                    coord.y = get_cube_coord(coordmap[f][1], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;
                    coord.z = get_cube_coord(coordmap[f][2], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;

                    function(&value, &coord, &size, funcdata);

                    set_d3dx_color(&color, (const struct vec4 *)&value, RANGE_FULL, RANGE_FULL);
                    format_from_d3dx_color(format, &color, dst);
                }
            }
            IDirect3DCubeTexture9_UnlockRect(texture, f, m);
        }
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXFillVolumeTexture(struct IDirect3DVolumeTexture9 *texture, LPD3DXFILL3D function, void *funcdata)
{
    DWORD miplevels;
    DWORD m, x, y, z;
    D3DVOLUME_DESC desc;
    D3DLOCKED_BOX lock_box;
    D3DXVECTOR4 value;
    D3DXVECTOR3 coord, size;
    const struct pixel_format_desc *format;
    BYTE *data;

    TRACE("texture %p, function %p, funcdata %p.\n", texture, function, funcdata);

    if (texture == NULL || function == NULL)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DVolumeTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(IDirect3DVolumeTexture9_GetLevelDesc(texture, m, &desc)))
            return D3DERR_INVALIDCALL;

        format = get_format_info(desc.Format);
        if (is_unknown_format(format) || is_index_format(format) || is_compressed_format(format))
        {
            FIXME("Unsupported texture format %#x\n", desc.Format);
            return D3DERR_INVALIDCALL;
        }

        if (FAILED(IDirect3DVolumeTexture9_LockBox(texture, m, &lock_box, NULL, D3DLOCK_DISCARD)))
            return D3DERR_INVALIDCALL;

        size.x = 1.0f / desc.Width;
        size.y = 1.0f / desc.Height;
        size.z = 1.0f / desc.Depth;

        data = lock_box.pBits;

        for (z = 0; z < desc.Depth; z++)
        {
            /* The callback function expects the coordinates of the center
               of the texel */
            coord.z = (z + 0.5f) / desc.Depth;

            for (y = 0; y < desc.Height; y++)
            {
                coord.y = (y + 0.5f) / desc.Height;

                for (x = 0; x < desc.Width; x++)
                {
                    BYTE *dst = data + z * lock_box.SlicePitch + y * lock_box.RowPitch + x * format->bytes_per_pixel;
                    struct d3dx_color color;

                    coord.x = (x + 0.5f) / desc.Width;

                    function(&value, &coord, &size, funcdata);

                    set_d3dx_color(&color, (const struct vec4 *)&value, RANGE_FULL, RANGE_FULL);
                    format_from_d3dx_color(format, &color, dst);
                }
            }
        }
        IDirect3DVolumeTexture9_UnlockBox(texture, m);
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXFillVolumeTextureTX(struct IDirect3DVolumeTexture9 *texture, ID3DXTextureShader *texture_shader)
{
    FIXME("texture %p, texture_shader %p stub.\n", texture, texture_shader);
    return E_NOTIMPL;
}

HRESULT WINAPI D3DXSaveTextureToFileA(const char *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    int len;
    WCHAR *filename;
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("dst_filename %s, file_format %u, src_texture %p, src_palette %p.\n",
            wine_dbgstr_a(dst_filename), file_format, src_texture, src_palette);

    if (!dst_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, NULL, 0);
    filename = malloc(len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, filename, len);

    hr = D3DXSaveTextureToFileInMemory(&buffer, file_format, src_texture, src_palette);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    free(filename);
    return hr;
}

HRESULT WINAPI D3DXSaveTextureToFileW(const WCHAR *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("dst_filename %s, file_format %u, src_texture %p, src_palette %p.\n",
            wine_dbgstr_w(dst_filename), file_format, src_texture, src_palette);

    if (!dst_filename) return D3DERR_INVALIDCALL;

    hr = D3DXSaveTextureToFileInMemory(&buffer, file_format, src_texture, src_palette);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(dst_filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    return hr;
}

HRESULT WINAPI D3DXSaveTextureToFileInMemory(ID3DXBuffer **dst_buffer, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    HRESULT hr;
    D3DRESOURCETYPE type;
    IDirect3DSurface9 *surface;

    TRACE("dst_buffer %p, file_format %u, src_texture %p, src_palette %p.\n",
            dst_buffer, file_format, src_texture, src_palette);

    if (!dst_buffer || !src_texture) return D3DERR_INVALIDCALL;

    if (file_format == D3DXIFF_DDS)
    {
        FIXME("DDS file format isn't supported yet\n");
        return E_NOTIMPL;
    }

    type = IDirect3DBaseTexture9_GetType(src_texture);
    switch (type)
    {
        case D3DRTYPE_TEXTURE:
        case D3DRTYPE_CUBETEXTURE:
            hr = get_surface(type, src_texture, D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
            break;
        case D3DRTYPE_VOLUMETEXTURE:
            FIXME("Volume textures aren't supported yet\n");
            return E_NOTIMPL;
        default:
            return D3DERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr))
    {
        hr = D3DXSaveSurfaceToFileInMemory(dst_buffer, file_format, surface, src_palette, NULL);
        IDirect3DSurface9_Release(surface);
    }

    return hr;
}

HRESULT WINAPI D3DXComputeNormalMap(IDirect3DTexture9 *texture, IDirect3DTexture9 *src_texture,
        const PALETTEENTRY *src_palette, DWORD flags, DWORD channel, float amplitude)
{
    FIXME("texture %p, src_texture %p, src_palette %p, flags %#lx, channel %lu, amplitude %.8e stub.\n",
            texture, src_texture, src_palette, flags, channel, amplitude);

    return D3D_OK;
}
