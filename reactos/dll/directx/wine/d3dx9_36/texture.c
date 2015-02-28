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

#include "d3dx9_36_private.h"

/* Returns TRUE if num is a power of 2, FALSE if not, or if 0 */
static BOOL is_pow2(UINT num)
{
    return !(num & (num - 1));
}

/* Returns the smallest power of 2 which is greater than or equal to num */
static UINT make_pow2(UINT num)
{
    UINT result = 1;

    /* In the unlikely event somebody passes a large value, make sure we don't enter an infinite loop */
    if (num >= 0x80000000)
        return 0x80000000;

    while (result < num)
        result <<= 1;

    return result;
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

    TRACE("(%p, %p, %u, %#x)\n", texture, palette, srclevel, filter);

    if (!texture)
        return D3DERR_INVALIDCALL;

    if ((filter & 0xFFFF) > D3DX_FILTER_BOX && filter != D3DX_DEFAULT)
        return D3DERR_INVALIDCALL;

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
                IDirect3DCubeTexture9_GetLevelDesc((IDirect3DTexture9*) texture, srclevel, &desc);
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

static D3DFORMAT get_luminance_replacement_format(D3DFORMAT format)
{
    static const struct
    {
        D3DFORMAT luminance_format;
        D3DFORMAT replacement_format;
    } luminance_replacements[] =
    {
        {D3DFMT_L8, D3DFMT_X8R8G8B8},
        {D3DFMT_A8L8, D3DFMT_A8R8G8B8},
        {D3DFMT_A4L4, D3DFMT_A4R4G4B4},
        {D3DFMT_L16, D3DFMT_A16B16G16R16}
    };
    unsigned int i;

    for (i = 0; i < sizeof(luminance_replacements) / sizeof(luminance_replacements[0]); ++i)
        if (format == luminance_replacements[i].luminance_format)
            return luminance_replacements[i].replacement_format;
    return format;
}

HRESULT WINAPI D3DXCheckTextureRequirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
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

    TRACE("(%p, %p, %p, %p, %u, %p, %u)\n", device, width, height, miplevels, usage, format, pool);

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
        usage, D3DRTYPE_TEXTURE, usedformat);
    if (FAILED(hr))
    {
        BOOL allow_24bits;
        int bestscore = INT_MIN, i = 0, j;
        unsigned int channels;
        const struct pixel_format_desc *curfmt, *bestfmt = NULL;

        TRACE("Requested format not supported, looking for a fallback.\n");

        if (!fmt)
        {
            FIXME("Pixel format %x not handled\n", usedformat);
            goto cleanup;
        }
        fmt = get_format_info(get_luminance_replacement_format(usedformat));

        allow_24bits = fmt->bytes_per_pixel == 3;
        channels = !!fmt->bits[0] + !!fmt->bits[1] + !!fmt->bits[2] + !!fmt->bits[3];
        usedformat = D3DFMT_UNKNOWN;

        while ((curfmt = get_format_info_idx(i)))
        {
            unsigned int curchannels = !!curfmt->bits[0] + !!curfmt->bits[1]
                    + !!curfmt->bits[2] + !!curfmt->bits[3];
            int score;

            i++;

            if (curchannels < channels)
                continue;
            if (curfmt->bytes_per_pixel == 3 && !allow_24bits)
                continue;

            hr = IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                mode.Format, usage, D3DRTYPE_TEXTURE, curfmt->format);
            if (FAILED(hr))
                continue;

            /* This format can be used, let's evaluate it.
               Weights chosen quite arbitrarily... */
            score = 512 * (curfmt->type == fmt->type);
            score -= 32 * (curchannels - channels);

            for (j = 0; j < 4; j++)
            {
                int diff = curfmt->bits[j] - fmt->bits[j];
                score -= (diff < 0 ? -diff * 8 : diff) * (j == 0 ? 1 : 2);
            }

            if (score > bestscore)
            {
                bestscore = score;
                usedformat = curfmt->format;
                bestfmt = curfmt;
            }
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

    if (fmt->block_width != 1 || fmt->block_height != 1)
    {
        if (w % fmt->block_width)
            w += fmt->block_width - w % fmt->block_width;
        if (h % fmt->block_height)
            h += fmt->block_height - h % fmt->block_height;
    }

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

HRESULT WINAPI D3DXCheckCubeTextureRequirements(struct IDirect3DDevice9 *device, UINT *size,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
{
    D3DCAPS9 caps;
    UINT s = (size && *size) ? *size : 256;
    HRESULT hr;

    TRACE("(%p, %p, %p, %u, %p, %u)\n", device, size, miplevels, usage, format, pool);

    if (s == D3DX_DEFAULT)
        s = 256;

    if (!device || FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
        return D3DERR_NOTAVAILABLE;

    /* ensure width/height is power of 2 */
    if ((caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2) && (!is_pow2(s)))
        s = make_pow2(s);

    hr = D3DXCheckTextureRequirements(device, &s, &s, miplevels, usage, format, pool);

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

    TRACE("(%p, %p, %p, %p, %p, %u, %p, %u)\n", device, width, height, depth, miplevels,
          usage, format, pool);

    if (!device || FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
        return D3DERR_NOTAVAILABLE;

    hr = D3DXCheckTextureRequirements(device, &w, &h, NULL, usage, format, pool);
    if (d == D3DX_DEFAULT)
        d = 1;

    /* ensure width/height is power of 2 */
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

    TRACE("device %p, width %u, height %u, miplevels %u, usage %#x, format %#x, pool %#x, texture %p.\n",
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

    for (i = 0; i < sizeof(replacement_formats) / sizeof(replacement_formats[0]); ++i)
        if (replacement_formats[i].orig_format == format)
            return replacement_formats[i].replacement_format;
    return format;
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(struct IDirect3DDevice9 *device, const void *srcdata,
        UINT srcdatasize, UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    IDirect3DTexture9 **texptr;
    IDirect3DTexture9 *buftex;
    IDirect3DSurface9 *surface;
    BOOL dynamic_texture, format_specified = FALSE;
    D3DXIMAGE_INFO imginfo;
    UINT loaded_miplevels, skip_levels;
    D3DCAPS9 caps;
    HRESULT hr;

    TRACE("device %p, srcdata %p, srcdatasize %u, width %u, height %u, miplevels %u,"
            " usage %#x, format %#x, pool %#x, filter %#x, mipfilter %#x, colorkey %#x,"
            " srcinfo %p, palette %p, texture %p.\n",
            device, srcdata, srcdatasize, width, height, miplevels, usage, format, pool,
            filter, mipfilter, colorkey, srcinfo, palette, texture);

    /* check for invalid parameters */
    if (!device || !texture || !srcdata || !srcdatasize)
        return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(srcdata, srcdatasize, &imginfo);
    if (FAILED(hr))
    {
        FIXME("Unrecognized file format, returning failure.\n");
        *texture = NULL;
        return hr;
    }

    /* handle default values */
    if (width == 0 || width == D3DX_DEFAULT_NONPOW2)
        width = imginfo.Width;

    if (height == 0 || height == D3DX_DEFAULT_NONPOW2)
        height = imginfo.Height;

    if (width == D3DX_DEFAULT)
        width = make_pow2(imginfo.Width);

    if (height == D3DX_DEFAULT)
        height = make_pow2(imginfo.Height);

    if (format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = imginfo.Format;
    else
        format_specified = TRUE;

    if (width == D3DX_FROM_FILE)
    {
        width = imginfo.Width;
    }

    if (height == D3DX_FROM_FILE)
    {
        height = imginfo.Height;
    }

    if (format == D3DFMT_FROM_FILE)
    {
        format = imginfo.Format;
    }

    if (miplevels == D3DX_FROM_FILE)
    {
        miplevels = imginfo.MipLevels;
    }

    skip_levels = mipfilter != D3DX_DEFAULT ? mipfilter >> D3DX_SKIP_DDS_MIP_LEVELS_SHIFT : 0;
    if (skip_levels && imginfo.MipLevels > skip_levels)
    {
        TRACE("Skipping the first %u (of %u) levels of a DDS mipmapped texture.\n",
                skip_levels, imginfo.MipLevels);
        TRACE("Texture level 0 dimensions are %ux%u.\n", imginfo.Width, imginfo.Height);
        width >>= skip_levels;
        height >>= skip_levels;
        miplevels -= skip_levels;
    }
    else
    {
        skip_levels = 0;
    }

    /* fix texture creation parameters */
    hr = D3DXCheckTextureRequirements(device, &width, &height, &miplevels, usage, &format, pool);
    if (FAILED(hr))
    {
        FIXME("Couldn't find suitable texture parameters.\n");
        *texture = NULL;
        return hr;
    }

    if (colorkey && !format_specified)
        format = get_alpha_replacement_format(format);

    if (imginfo.MipLevels < miplevels && (D3DFMT_DXT1 <= imginfo.Format && imginfo.Format <= D3DFMT_DXT5))
    {
        FIXME("Generation of mipmaps for compressed pixel formats is not implemented yet\n");
        miplevels = imginfo.MipLevels;
    }
    if (imginfo.ResourceType == D3DRTYPE_VOLUMETEXTURE
            && D3DFMT_DXT1 <= imginfo.Format && imginfo.Format <= D3DFMT_DXT5 && miplevels > 1)
    {
        FIXME("Generation of mipmaps for compressed pixel formats is not implemented yet.\n");
        miplevels = 1;
    }

    if (FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    /* Create the to-be-filled texture */
    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, D3DPOOL_SYSTEMMEM, &buftex);
        texptr = &buftex;
    }
    else
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, texture);
        texptr = texture;
    }

    if (FAILED(hr))
    {
        FIXME("Texture creation failed.\n");
        *texture = NULL;
        return hr;
    }

    TRACE("Texture created correctly. Now loading the texture data into it.\n");
    if (imginfo.ImageFileFormat != D3DXIFF_DDS)
    {
        IDirect3DTexture9_GetSurfaceLevel(*texptr, 0, &surface);
        hr = D3DXLoadSurfaceFromFileInMemory(surface, palette, NULL, srcdata, srcdatasize, NULL, filter, colorkey, NULL);
        IDirect3DSurface9_Release(surface);
        loaded_miplevels = min(IDirect3DTexture9_GetLevelCount(*texptr), imginfo.MipLevels);
    }
    else
    {
        hr = load_texture_from_dds(*texptr, srcdata, palette, filter, colorkey, &imginfo, skip_levels,
                &loaded_miplevels);
    }

    if (FAILED(hr))
    {
        FIXME("Texture loading failed.\n");
        IDirect3DTexture9_Release(*texptr);
        *texture = NULL;
        return hr;
    }

    hr = D3DXFilterTexture((IDirect3DBaseTexture9*) *texptr, palette, loaded_miplevels - 1, mipfilter);
    if (FAILED(hr))
    {
        FIXME("Texture filtering failed.\n");
        IDirect3DTexture9_Release(*texptr);
        *texture = NULL;
        return hr;
    }

    /* Move the data to the actual texture if necessary */
    if (texptr == &buftex)
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, texture);

        if (FAILED(hr))
        {
            IDirect3DTexture9_Release(buftex);
            *texture = NULL;
            return hr;
        }

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)buftex, (IDirect3DBaseTexture9*)(*texture));
        IDirect3DTexture9_Release(buftex);
    }

    if (srcinfo)
        *srcinfo = imginfo;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemory(struct IDirect3DDevice9 *device,
        const void *srcdata, UINT srcdatasize, struct IDirect3DTexture9 **texture)
{
    TRACE("(%p, %p, %d, %p)\n", device, srcdata, srcdatasize, texture);

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

    TRACE("device %p, srcfile %s, width %u, height %u, miplevels %u, usage %#x, format %#x, "
            "pool %#x, filter %#x, mipfilter %#x, colorkey 0x%08x, srcinfo %p, palette %p, texture %p.\n",
            device, debugstr_w(srcfile), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    hr = map_view_of_file(srcfile, &buffer, &size);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

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

    TRACE("device %p, srcfile %s, width %u, height %u, miplevels %u, usage %#x, format %#x, "
            "pool %#x, filter %#x, mipfilter %#x, colorkey 0x%08x, srcinfo %p, palette %p, texture %p.\n",
            device, debugstr_a(srcfile), width, height, miplevels, usage, format,
            pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!device || !srcfile || !texture)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    widename = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*widename));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, widename, len);

    hr = D3DXCreateTextureFromFileExW(device, widename, width, height, miplevels,
                                      usage, format, pool, filter, mipfilter,
                                      colorkey, srcinfo, palette, texture);

    HeapFree(GetProcessHeap(), 0, widename);
    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileA(struct IDirect3DDevice9 *device,
        const char *srcfile, struct IDirect3DTexture9 **texture)
{
    TRACE("(%p, %s, %p)\n", device, debugstr_a(srcfile), texture);

    return D3DXCreateTextureFromFileExA(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromFileW(struct IDirect3DDevice9 *device,
        const WCHAR *srcfile, struct IDirect3DTexture9 **texture)
{
    TRACE("(%p, %s, %p)\n", device, debugstr_w(srcfile), texture);

    return D3DXCreateTextureFromFileExW(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}


HRESULT WINAPI D3DXCreateTextureFromResourceA(struct IDirect3DDevice9 *device,
        HMODULE srcmodule, const char *resource, struct IDirect3DTexture9 **texture)
{
    TRACE("(%p, %s): relay\n", srcmodule, debugstr_a(resource));

    return D3DXCreateTextureFromResourceExA(device, srcmodule, resource, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromResourceW(struct IDirect3DDevice9 *device,
        HMODULE srcmodule, const WCHAR *resource, struct IDirect3DTexture9 **texture)
{
    TRACE("(%p, %s): relay\n", srcmodule, debugstr_w(resource));

    return D3DXCreateTextureFromResourceExW(device, srcmodule, resource, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                            D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromResourceExA(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *resource, UINT width, UINT height, UINT miplevels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, DWORD filter, DWORD mipfilter, D3DCOLOR colorkey, D3DXIMAGE_INFO *srcinfo,
        PALETTEENTRY *palette, struct IDirect3DTexture9 **texture)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("device %p, srcmodule %p, resource %s, width %u, height %u, miplevels %u, usage %#x, format %#x, "
            "pool %#x, filter %#x, mipfilter %#x, colorkey 0x%08x, srcinfo %p, palette %p, texture %p.\n",
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

    TRACE("device %p, srcmodule %p, resource %s, width %u, height %u, miplevels %u, usage %#x, format %#x, "
            "pool %#x, filter %#x, mipfilter %#x, colorkey 0x%08x, srcinfo %p, palette %p, texture %p.\n",
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

    TRACE("(%p, %u, %u, %#x, %#x, %#x, %p)\n", device, size, miplevels, usage, format,
        pool, texture);

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
    TRACE("(%p, %p, %u, %p)\n", device, data, datasize, texture);

    return D3DXCreateCubeTextureFromFileInMemoryEx(device, data, datasize, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateVolumeTexture(struct IDirect3DDevice9 *device, UINT width, UINT height, UINT depth,
        UINT miplevels, DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DVolumeTexture9 **texture)
{
    HRESULT hr;

    TRACE("(%p, %u, %u, %u, %u, %#x, %#x, %#x, %p)\n", device, width, height, depth,
          miplevels, usage, format, pool, texture);

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

    TRACE("(%p, %s, %p): relay\n",
            device, debugstr_a(filename), volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = map_view_of_file(filenameW, &data, &data_size);
    HeapFree(GetProcessHeap(), 0, filenameW);
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

    TRACE("(%p, %s, %p): relay\n",
            device, debugstr_w(filename), volume_texture);

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

    TRACE("(%p, %s, %u, %u, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p): relay\n",
            device, debugstr_a(filename), width, height, depth, mip_levels,
            usage, format, pool, filter, mip_filter, color_key, src_info,
            palette, volume_texture);

    if (!filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = map_view_of_file(filenameW, &data, &data_size);
    HeapFree(GetProcessHeap(), 0, filenameW);
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

    TRACE("(%p, %s, %u, %u, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p): relay\n",
            device, debugstr_w(filename), width, height, depth, mip_levels,
            usage, format, pool, filter, mip_filter, color_key, src_info,
            palette, volume_texture);

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
    TRACE("(%p, %p, %u, %p): relay\n", device, data, data_size, volume_texture);

    return D3DXCreateVolumeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT,
        0, NULL, NULL, volume_texture);
}

HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemoryEx(IDirect3DDevice9 *device,
                                                         const void *data,
                                                         UINT data_size,
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
                                                         D3DXIMAGE_INFO *info,
                                                         PALETTEENTRY *palette,
                                                         IDirect3DVolumeTexture9 **volume_texture)
{
    HRESULT hr;
    D3DCAPS9 caps;
    D3DXIMAGE_INFO image_info;
    BOOL dynamic_texture;
    BOOL file_width = FALSE;
    BOOL file_height = FALSE;
    BOOL file_depth = FALSE;
    BOOL file_format = FALSE;
    BOOL file_mip_levels = FALSE;
    IDirect3DVolumeTexture9 *tex, *buftex;

    TRACE("(%p, %p, %u, %u, %u, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p)\n",
            device, data, data_size, width, height, depth, mip_levels, usage, format, pool,
            filter, mip_filter, color_key, info, palette, volume_texture);

    if (!device || !data || !data_size || !volume_texture)
        return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(data, data_size, &image_info);
    if (FAILED(hr)) return hr;

    if (image_info.ImageFileFormat != D3DXIFF_DDS)
        return D3DXERR_INVALIDDATA;

    if (width == 0 || width == D3DX_DEFAULT_NONPOW2)
        width = image_info.Width;
    if (width == D3DX_DEFAULT)
        width = make_pow2(image_info.Width);

    if (height == 0 || height == D3DX_DEFAULT_NONPOW2)
        height = image_info.Height;
    if (height == D3DX_DEFAULT)
        height = make_pow2(image_info.Height);

    if (depth == 0 || depth == D3DX_DEFAULT_NONPOW2)
        depth = image_info.Depth;
    if (depth == D3DX_DEFAULT)
        depth = make_pow2(image_info.Depth);

    if (format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = image_info.Format;

    if (width == D3DX_FROM_FILE)
    {
        file_width = TRUE;
        width = image_info.Width;
    }

    if (height == D3DX_FROM_FILE)
    {
        file_height = TRUE;
        height = image_info.Height;
    }

    if (depth == D3DX_FROM_FILE)
    {
        file_depth = TRUE;
        depth = image_info.Depth;
    }

    if (format == D3DFMT_FROM_FILE)
    {
        file_format = TRUE;
        format = image_info.Format;
    }

    if (mip_levels == D3DX_FROM_FILE)
    {
        file_mip_levels = TRUE;
        mip_levels = image_info.MipLevels;
    }

    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, &mip_levels, usage, &format, pool);
    if (FAILED(hr)) return hr;

    if ((file_width && width != image_info.Width)
            || (file_height && height != image_info.Height)
            || (file_depth && depth != image_info.Depth)
            || (file_format && format != image_info.Format)
            || (file_mip_levels && mip_levels != image_info.MipLevels))
        return D3DERR_NOTAVAILABLE;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr))
        return D3DERR_INVALIDCALL;

    if (mip_levels > image_info.MipLevels)
    {
        FIXME("Generation of mipmaps for volume textures is not implemented yet\n");
        mip_levels = image_info.MipLevels;
    }

    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, usage, format, D3DPOOL_SYSTEMMEM, &buftex);
        tex = buftex;
    }
    else
    {
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, usage, format, pool, &tex);
        buftex = NULL;
    }

    if (FAILED(hr)) return hr;

    hr = load_volume_texture_from_dds(tex, data, palette, filter, color_key, &image_info);
    if (FAILED(hr))
    {
        IDirect3DVolumeTexture9_Release(tex);
        return hr;
    }

    if (buftex)
    {
        hr = D3DXCreateVolumeTexture(device, width, height, depth, mip_levels, usage, format, pool, &tex);
        if (FAILED(hr))
        {
            IDirect3DVolumeTexture9_Release(buftex);
            return hr;
        }

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)buftex, (IDirect3DBaseTexture9 *)tex);
        IDirect3DVolumeTexture9_Release(buftex);
    }

    if (info)
        *info = image_info;

    *volume_texture = tex;
    return D3D_OK;
}

static inline void fill_texture(const struct pixel_format_desc *format, BYTE *pos, const D3DXVECTOR4 *value)
{
    DWORD c;

    for (c = 0; c < format->bytes_per_pixel; c++)
        pos[c] = 0;

    for (c = 0; c < 4; c++)
    {
        float comp_value;
        DWORD i, v = 0, mask32 = format->bits[c] == 32 ? ~0U : ((1 << format->bits[c]) - 1);

        switch (c)
        {
            case 0: /* Alpha */
                comp_value = value->w;
                break;
            case 1: /* Red */
                comp_value = value->x;
                break;
            case 2: /* Green */
                comp_value = value->y;
                break;
            case 3: /* Blue */
                comp_value = value->z;
                break;
        }

        if (format->type == FORMAT_ARGBF16)
            v = float_32_to_16(comp_value);
        else if (format->type == FORMAT_ARGBF)
            v = *(DWORD *)&comp_value;
        else if (format->type == FORMAT_ARGB)
            v = comp_value * ((1 << format->bits[c]) - 1) + 0.5f;
        else
            FIXME("Unhandled format type %#x\n", format->type);

        for (i = 0; i < format->bits[c] + format->shift[c]; i += 8)
        {
            BYTE byte, mask;

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
            pos[i / 8] |= byte;
        }
    }
}

HRESULT WINAPI D3DXFillTexture(struct IDirect3DTexture9 *texture, LPD3DXFILL2D function, void *funcdata)
{
    DWORD miplevels;
    DWORD m, x, y;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lock_rect;
    D3DXVECTOR4 value;
    D3DXVECTOR2 coord, size;
    const struct pixel_format_desc *format;
    BYTE *data;

    if (texture == NULL || function == NULL)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DBaseTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(IDirect3DTexture9_GetLevelDesc(texture, m, &desc)))
            return D3DERR_INVALIDCALL;

        format = get_format_info(desc.Format);
        if (format->type != FORMAT_ARGB && format->type != FORMAT_ARGBF16 && format->type != FORMAT_ARGBF)
        {
            FIXME("Unsupported texture format %#x\n", desc.Format);
            return D3DERR_INVALIDCALL;
        }

        if (FAILED(IDirect3DTexture9_LockRect(texture, m, &lock_rect, NULL, D3DLOCK_DISCARD)))
            return D3DERR_INVALIDCALL;

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
                coord.x = (x + 0.5f) / desc.Width;

                function(&value, &coord, &size, funcdata);

                fill_texture(format, data + y * lock_rect.Pitch + x * format->bytes_per_pixel, &value);
            }
        }
        IDirect3DTexture9_UnlockRect(texture, m);
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemoryEx(IDirect3DDevice9 *device,
                                                       const void *src_data,
                                                       UINT src_data_size,
                                                       UINT size,
                                                       UINT mip_levels,
                                                       DWORD usage,
                                                       D3DFORMAT format,
                                                       D3DPOOL pool,
                                                       DWORD filter,
                                                       DWORD mip_filter,
                                                       D3DCOLOR color_key,
                                                       D3DXIMAGE_INFO *src_info,
                                                       PALETTEENTRY *palette,
                                                       IDirect3DCubeTexture9 **cube_texture)
{
    HRESULT hr;
    D3DCAPS9 caps;
    UINT loaded_miplevels;
    D3DXIMAGE_INFO img_info;
    BOOL dynamic_texture;
    BOOL file_size = FALSE;
    BOOL file_format = FALSE;
    BOOL file_mip_levels = FALSE;
    IDirect3DCubeTexture9 *tex, *buftex;

    TRACE("(%p, %p, %u, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p)\n", device,
        src_data, src_data_size, size, mip_levels, usage, format, pool, filter, mip_filter,
        color_key, src_info, palette, cube_texture);

    if (!device || !cube_texture || !src_data || !src_data_size)
        return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(src_data, src_data_size, &img_info);
    if (FAILED(hr))
        return hr;

    if (img_info.ImageFileFormat != D3DXIFF_DDS)
        return D3DXERR_INVALIDDATA;

    if (img_info.Width != img_info.Height)
        return D3DXERR_INVALIDDATA;

    if (size == 0 || size == D3DX_DEFAULT_NONPOW2)
        size = img_info.Width;
    if (size == D3DX_DEFAULT)
        size = make_pow2(img_info.Width);

    if (format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = img_info.Format;

    if (size == D3DX_FROM_FILE)
    {
        file_size = TRUE;
        size = img_info.Width;
    }

    if (format == D3DFMT_FROM_FILE)
    {
        file_format = TRUE;
        format = img_info.Format;
    }

    if (mip_levels == D3DX_FROM_FILE)
    {
        file_mip_levels = TRUE;
        mip_levels = img_info.MipLevels;
    }

    hr = D3DXCheckCubeTextureRequirements(device, &size, &mip_levels, usage, &format, pool);
    if (FAILED(hr))
        return hr;

    if ((file_size && size != img_info.Width)
            || (file_format && format != img_info.Format)
            || (file_mip_levels && mip_levels != img_info.MipLevels))
        return D3DERR_NOTAVAILABLE;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr))
        return D3DERR_INVALIDCALL;

    if (mip_levels > img_info.MipLevels && (D3DFMT_DXT1 <= img_info.Format && img_info.Format <= D3DFMT_DXT5))
    {
        FIXME("Generation of mipmaps for compressed pixel formats not supported yet\n");
        mip_levels = img_info.MipLevels;
    }

    dynamic_texture = (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (usage & D3DUSAGE_DYNAMIC);
    if (pool == D3DPOOL_DEFAULT && !dynamic_texture)
    {
        hr = D3DXCreateCubeTexture(device, size, mip_levels, usage, format, D3DPOOL_SYSTEMMEM, &buftex);
        tex = buftex;
    }
    else
    {
        hr = D3DXCreateCubeTexture(device, size, mip_levels, usage, format, pool, &tex);
        buftex = NULL;
    }
    if (FAILED(hr))
        return hr;

    hr = load_cube_texture_from_dds(tex, src_data, palette, filter, color_key, &img_info);
    if (FAILED(hr))
    {
        IDirect3DCubeTexture9_Release(tex);
        return hr;
    }

    loaded_miplevels = min(IDirect3DCubeTexture9_GetLevelCount(tex), img_info.MipLevels);
    hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, palette, loaded_miplevels - 1, mip_filter);
    if (FAILED(hr))
    {
        IDirect3DCubeTexture9_Release(tex);
        return hr;
    }

    if (buftex)
    {
        hr = D3DXCreateCubeTexture(device, size, mip_levels, usage, format, pool, &tex);
        if (FAILED(hr))
        {
            IDirect3DCubeTexture9_Release(buftex);
            return hr;
        }

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)buftex, (IDirect3DBaseTexture9 *)tex);
        IDirect3DCubeTexture9_Release(buftex);
    }

    if (src_info)
        *src_info = img_info;

    *cube_texture = tex;
    return D3D_OK;
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

    TRACE("(%p, %s, %p): relay\n", device, wine_dbgstr_a(src_filename), cube_texture);

    if (!src_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, src_filename, -1, NULL, 0);
    filename = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, src_filename, -1, filename, len);

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, filename);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, cube_texture);

    UnmapViewOfFile(data);
    HeapFree(GetProcessHeap(), 0, filename);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileW(IDirect3DDevice9 *device,
                                              const WCHAR *src_filename,
                                              IDirect3DCubeTexture9 **cube_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("(%p, %s, %p): relay\n", device, wine_dbgstr_w(src_filename), cube_texture);

    hr = map_view_of_file(src_filename, &data, &data_size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, cube_texture);

    UnmapViewOfFile(data);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileExA(IDirect3DDevice9 *device,
                                                const char *src_filename,
                                                UINT size,
                                                UINT mip_levels,
                                                DWORD usage,
                                                D3DFORMAT format,
                                                D3DPOOL pool,
                                                DWORD filter,
                                                DWORD mip_filter,
                                                D3DCOLOR color_key,
                                                D3DXIMAGE_INFO *image_info,
                                                PALETTEENTRY *palette,
                                                IDirect3DCubeTexture9 **cube_texture)
{
    int len;
    HRESULT hr;
    WCHAR *filename;
    void *data;
    DWORD data_size;

    TRACE("(%p, %s, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p): relay\n",
            device, wine_dbgstr_a(src_filename), size, mip_levels, usage, format,
            pool, filter, mip_filter, color_key, image_info, palette, cube_texture);

    if (!src_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, src_filename, -1, NULL, 0);
    filename = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, src_filename, -1, filename, len);

    hr = map_view_of_file(filename, &data, &data_size);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, filename);
        return D3DXERR_INVALIDDATA;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, data, data_size, size, mip_levels,
        usage, format, pool, filter, mip_filter, color_key, image_info, palette, cube_texture);

    UnmapViewOfFile(data);
    HeapFree(GetProcessHeap(), 0, filename);
    return hr;
}

HRESULT WINAPI D3DXCreateCubeTextureFromFileExW(IDirect3DDevice9 *device,
                                                const WCHAR *src_filename,
                                                UINT size,
                                                UINT mip_levels,
                                                DWORD usage,
                                                D3DFORMAT format,
                                                D3DPOOL pool,
                                                DWORD filter,
                                                DWORD mip_filter,
                                                D3DCOLOR color_key,
                                                D3DXIMAGE_INFO *image_info,
                                                PALETTEENTRY *palette,
                                                IDirect3DCubeTexture9 **cube_texture)
{
    HRESULT hr;
    void *data;
    DWORD data_size;

    TRACE("(%p, %s, %u, %u, %#x, %#x, %#x, %#x, %#x, %#x, %p, %p, %p): relay\n",
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

    if (texture == NULL || function == NULL)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DBaseTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(IDirect3DCubeTexture9_GetLevelDesc(texture, m, &desc)))
            return D3DERR_INVALIDCALL;

        format = get_format_info(desc.Format);
        if (format->type != FORMAT_ARGB && format->type != FORMAT_ARGBF16 && format->type != FORMAT_ARGBF)
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
                    coord.x = get_cube_coord(coordmap[f][0], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;
                    coord.y = get_cube_coord(coordmap[f][1], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;
                    coord.z = get_cube_coord(coordmap[f][2], x, y, desc.Width) / desc.Width * 2.0f - 1.0f;

                    function(&value, &coord, &size, funcdata);

                    fill_texture(format, data + y * lock_rect.Pitch + x * format->bytes_per_pixel, &value);
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

    if (texture == NULL || function == NULL)
        return D3DERR_INVALIDCALL;

    miplevels = IDirect3DBaseTexture9_GetLevelCount(texture);

    for (m = 0; m < miplevels; m++)
    {
        if (FAILED(IDirect3DVolumeTexture9_GetLevelDesc(texture, m, &desc)))
            return D3DERR_INVALIDCALL;

        format = get_format_info(desc.Format);
        if (format->type != FORMAT_ARGB && format->type != FORMAT_ARGBF16 && format->type != FORMAT_ARGBF)
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
                    coord.x = (x + 0.5f) / desc.Width;

                    function(&value, &coord, &size, funcdata);

                    fill_texture(format, data + z * lock_box.SlicePitch + y * lock_box.RowPitch
                            + x * format->bytes_per_pixel, &value);
                }
            }
        }
        IDirect3DVolumeTexture9_UnlockBox(texture, m);
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXSaveTextureToFileA(const char *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    int len;
    WCHAR *filename;
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p): relay\n",
            wine_dbgstr_a(dst_filename), file_format, src_texture, src_palette);

    if (!dst_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, NULL, 0);
    filename = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, filename, len);

    hr = D3DXSaveTextureToFileInMemory(&buffer, file_format, src_texture, src_palette);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    HeapFree(GetProcessHeap(), 0, filename);
    return hr;
}

HRESULT WINAPI D3DXSaveTextureToFileW(const WCHAR *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DBaseTexture9 *src_texture, const PALETTEENTRY *src_palette)
{
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p): relay\n",
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

    TRACE("(%p, %#x, %p, %p)\n",
        dst_buffer, file_format, src_texture, src_palette);

    if (!dst_buffer || !src_texture) return D3DERR_INVALIDCALL;

    if (file_format == D3DXIFF_DDS)
        return save_dds_texture_to_memory(dst_buffer, src_texture, src_palette);

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
