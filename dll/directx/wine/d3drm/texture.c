/*
 * Implementation of IDirect3DRMTextureX interfaces
 *
 * Copyright 2012 Christian Costa
 * Copyright 2016 Aaryaman Vasishta
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

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

static inline struct d3drm_texture *impl_from_IDirect3DRMTexture(IDirect3DRMTexture *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_texture, IDirect3DRMTexture_iface);
}

static inline struct d3drm_texture *impl_from_IDirect3DRMTexture2(IDirect3DRMTexture2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_texture, IDirect3DRMTexture2_iface);
}

static inline struct d3drm_texture *impl_from_IDirect3DRMTexture3(IDirect3DRMTexture3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_texture, IDirect3DRMTexture3_iface);
}

static void d3drm_texture_destroy(struct d3drm_texture *texture)
{
    TRACE("texture %p is being destroyed.\n", texture);

    d3drm_object_cleanup((IDirect3DRMObject*)&texture->IDirect3DRMTexture_iface, &texture->obj);
    if (texture->image || texture->surface)
        IDirect3DRM_Release(texture->d3drm);
    if (texture->surface)
        IDirectDrawSurface_Release(texture->surface);
    free(texture);
}

static BOOL d3drm_validate_image(D3DRMIMAGE *image)
{
    if (!image)
        return FALSE;

    TRACE("size (%d, %d), aspect (%d, %d), depth %d, red %#lx, green %#lx, blue %#lx, "
          "buffer1 %p, buffer2 %p, rgb %d, pal %p, size %d\n",
          image->width, image->height, image->aspectx, image->aspecty,
          image->depth, image->red_mask, image->green_mask, image->blue_mask, image->buffer1,
          image->buffer2, image->rgb, image->palette, image->palette_size );

    if (!image->buffer1)
        return FALSE;

    if (image->rgb)
    {
        if (!image->red_mask || !image->green_mask || !image->blue_mask)
            return FALSE;
    }
    else
    {
        if (!image->palette || !image->palette_size)
            return FALSE;
    }

    return TRUE;
}

static BOOL d3drm_image_palettise(D3DRMIMAGE *image, unsigned char *src_data,
        SIZE_T w, SIZE_T h, BOOL flip)
{
    unsigned char *dst_data, *src_ptr, *dst_ptr;
    SIZE_T src_pitch, dst_pitch, i, x, y;
    D3DRMPALETTEENTRY *palette, *entry;
    unsigned int colour_count = 0;

    if (w > (~(SIZE_T)0 - 3) / h)
        return FALSE;

    src_pitch = flip ? -w * 3 : w * 3;
    dst_pitch = (w + 3) & ~3;

    if (!(dst_data = malloc(dst_pitch * h)))
    {
        WARN("Failed to allocate image buffer.\n");
        return FALSE;
    }
    memset(dst_data, 0xff, dst_pitch * h);

    if (!(palette = malloc(256 * sizeof(*palette))))
    {
        WARN("Failed to allocate palette.\n");
        free(dst_data);
        return FALSE;
    }

    src_ptr = flip ? &src_data[(h - 1) * w * 3] : src_data;
    dst_ptr = dst_data;

    for (y = 0; y < h; ++y)
    {
        for (x = 0; x < w; ++x)
        {
            for (i = 0; i < colour_count; ++i)
            {
                entry = &palette[i];
                if (entry->red == src_ptr[x * 3 + 2]
                        && entry->green == src_ptr[x * 3 + 1]
                        && entry->blue == src_ptr[x * 3 + 0])
                    break;
            }

            if (i == colour_count)
            {
                if (colour_count == 256)
                {
                    free(dst_data);
                    free(palette);
                    return FALSE;
                }

                entry = &palette[colour_count++];
                entry->red = src_ptr[x * 3 + 2];
                entry->green = src_ptr[x * 3 + 1];
                entry->blue = src_ptr[x * 3 + 0];
                entry->flags = D3DRMPALETTE_READONLY;
            }

            dst_ptr[x] = i;
        }

        src_ptr += src_pitch;
        dst_ptr += dst_pitch;
    }

    image->depth = 8;
    image->rgb = 0;
    image->bytes_per_line = dst_pitch;
    image->buffer1 = dst_data;
    image->red_mask = 0xff;
    image->green_mask = 0xff;
    image->blue_mask = 0xff;
    image->palette_size = colour_count;
    image->palette = palette;
    if ((palette = realloc(palette, colour_count * sizeof(*palette))))
        image->palette = palette;

    return TRUE;
}

static HRESULT d3drm_image_load_32(D3DRMIMAGE *image, unsigned char *src_data,
        LONGLONG src_data_size, SIZE_T w, SIZE_T h, BOOL flip)
{
    unsigned char *dst_data, *src_ptr, *dst_ptr;
    SIZE_T src_pitch, dst_pitch, x, y;

    if (d3drm_image_palettise(image, src_data, w, h, flip))
        return D3DRM_OK;

    if (w > (~(SIZE_T)0 / 4) / h)
        return D3DRMERR_BADALLOC;

    src_pitch = flip ? -w * 3 : w * 3;
    dst_pitch = w * 4;

    if (!(dst_data = malloc(dst_pitch * h)))
    {
        WARN("Failed to allocate image buffer.\n");
        return D3DRMERR_BADALLOC;
    }

    src_ptr = flip ? &src_data[(h - 1) * w * 3] : src_data;
    dst_ptr = dst_data;

    for (y = 0; y < h; ++y)
    {
        for (x = 0; x < w; ++x)
        {
            dst_ptr[x * 4 + 0] = src_ptr[x * 3 + 0];
            dst_ptr[x * 4 + 1] = src_ptr[x * 3 + 1];
            dst_ptr[x * 4 + 2] = src_ptr[x * 3 + 2];
            dst_ptr[x * 4 + 3] = 0xff;
        }

        src_ptr += src_pitch;
        dst_ptr += dst_pitch;
    }

    image->depth = 32;
    image->rgb = 1;
    image->bytes_per_line = dst_pitch;
    image->buffer1 = dst_data;
    image->red_mask = 0xff0000;
    image->green_mask = 0x00ff00;
    image->blue_mask = 0x0000ff;
    image->palette_size = 0;
    image->palette = NULL;

    return D3DRM_OK;
}

static HRESULT d3drm_image_load_8(D3DRMIMAGE *image, const RGBQUAD *palette,
        unsigned char *src_data, LONGLONG src_data_size, SIZE_T w, SIZE_T h, BOOL flip)
{
    unsigned char *dst_data;
    SIZE_T i;

    if (w > ~(SIZE_T)0 / h)
        return D3DRMERR_BADALLOC;

    if (!(dst_data = malloc(w * h)))
    {
        WARN("Failed to allocate image buffer.\n");
        return D3DRMERR_BADALLOC;
    }

    if (!(image->palette = malloc(256 * sizeof(*image->palette))))
    {
        WARN("Failed to allocate palette.\n");
        free(dst_data);
        return D3DRMERR_BADALLOC;
    }

    for (i = 0; i < 256; ++i)
    {
        image->palette[i].red = palette[i].rgbRed;
        image->palette[i].green = palette[i].rgbGreen;
        image->palette[i].blue = palette[i].rgbBlue;
        image->palette[i].flags = D3DRMPALETTE_READONLY;
    }

    if (flip)
    {
        for (i = 0; i < h; ++i)
        {
            memcpy(&dst_data[i * w], &src_data[(h - 1 - i) * w], w);
        }
    }
    else
    {
        memcpy(dst_data, src_data, w * h);
    }

    image->depth = 8;
    image->rgb = 0;
    image->bytes_per_line = w;
    image->buffer1 = dst_data;
    image->red_mask = 0xff;
    image->green_mask = 0xff;
    image->blue_mask = 0xff;
    image->palette_size = 256;

    return D3DRM_OK;
}

static void CDECL destroy_image_callback(IDirect3DRMObject *obj, void *arg)
{
    D3DRMIMAGE *image = arg;

    TRACE("texture object %p, image %p.\n", obj, image);

    free(image->buffer1);
    free(image);
}

static HRESULT d3drm_texture_load(struct d3drm_texture *texture,
        const char *path, BOOL flip, D3DRMIMAGE **image_out)
{
    BITMAPFILEHEADER *header;
    unsigned int w, h, bpp;
    HANDLE file, mapping;
    LARGE_INTEGER size;
    D3DRMIMAGE *image;
    BITMAPINFO *info;
    LONGLONG rem;
    HRESULT hr;

    if ((file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
        return D3DRMERR_BADOBJECT;

    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if (!mapping || mapping == INVALID_HANDLE_VALUE)
        return D3DRMERR_BADVALUE;

    if (!GetFileSizeEx(mapping, &size))
    {
        CloseHandle(mapping);
        return D3DRMERR_BADVALUE;
    }
    rem = size.QuadPart;

    header = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if (!header)
        return D3DRMERR_BADVALUE;

    hr = D3DRMERR_BADALLOC;
    if (!(image = calloc(1, sizeof(*image))))
        goto fail;

    hr = D3DRMERR_BADFILE;
    if (rem < sizeof(*header) || header->bfType != 0x4d42 /* BM */)
        goto fail;
    rem -= sizeof(*header);

    info = (BITMAPINFO *)&header[1];
    /* Only allow version 1 DIB's (BITMAPINFOHEADER) to be loaded. */
    if (rem < sizeof(info->bmiHeader) || info->bmiHeader.biSize != sizeof(info->bmiHeader))
        goto fail;
    rem -= sizeof(info->bmiHeader);

    w = info->bmiHeader.biWidth;
    h = abs(info->bmiHeader.biHeight);
    bpp = info->bmiHeader.biBitCount == 24 ? 32 : info->bmiHeader.biBitCount;
    if (bpp != 8 && bpp != 32)
        goto fail;

    image->width = w;
    image->height = h;
    image->aspectx = 1;
    image->aspecty = 1;
    if (bpp == 8)
    {
        rem -= 256 * sizeof(*info->bmiColors);
        if (w > rem / h)
            goto fail;
        hr = d3drm_image_load_8(image, info->bmiColors, (unsigned char *)&info->bmiColors[256], rem, w, h, flip);
    }
    else
    {
        if (w > (rem / 3) / h)
            goto fail;
        hr = d3drm_image_load_32(image, (unsigned char *)&info->bmiColors, rem, w, h, flip);
    }
    if (FAILED(hr))
        goto fail;

    /* Use an internal destroy callback to destroy the image struct. */
    hr = IDirect3DRMObject_AddDestroyCallback(&texture->IDirect3DRMTexture3_iface, destroy_image_callback, image);

    *image_out = image;

    UnmapViewOfFile(header);

    return hr;

fail:
    free(image);
    UnmapViewOfFile(header);

    return hr;
}

static HRESULT WINAPI d3drm_texture1_QueryInterface(IDirect3DRMTexture *iface, REFIID riid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IDirect3DRMTexture3_QueryInterface(&texture->IDirect3DRMTexture3_iface, riid, out);
}

static ULONG WINAPI d3drm_texture1_AddRef(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_AddRef(&texture->IDirect3DRMTexture3_iface);
}

static ULONG WINAPI d3drm_texture1_Release(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_Release(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture1_Clone(IDirect3DRMTexture *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    return IDirect3DRMTexture3_Clone(&texture->IDirect3DRMTexture3_iface, outer, iid, out);
}

static HRESULT WINAPI d3drm_texture1_AddDestroyCallback(IDirect3DRMTexture *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return IDirect3DRMTexture3_AddDestroyCallback(&texture->IDirect3DRMTexture3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_texture1_DeleteDestroyCallback(IDirect3DRMTexture *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return IDirect3DRMTexture3_DeleteDestroyCallback(&texture->IDirect3DRMTexture3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_texture1_SetAppData(IDirect3DRMTexture *iface, DWORD data)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return IDirect3DRMTexture3_SetAppData(&texture->IDirect3DRMTexture3_iface, data);
}

static DWORD WINAPI d3drm_texture1_GetAppData(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetAppData(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture1_SetName(IDirect3DRMTexture *iface, const char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return IDirect3DRMTexture3_SetName(&texture->IDirect3DRMTexture3_iface, name);
}

static HRESULT WINAPI d3drm_texture1_GetName(IDirect3DRMTexture *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMTexture3_GetName(&texture->IDirect3DRMTexture3_iface, size, name);
}

static HRESULT WINAPI d3drm_texture1_GetClassName(IDirect3DRMTexture *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMTexture3_GetClassName(&texture->IDirect3DRMTexture3_iface, size, name);
}

static HRESULT WINAPI d3drm_texture1_InitFromFile(IDirect3DRMTexture *iface, const char *filename)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);
    D3DRMIMAGE *image;
    HRESULT hr;

    TRACE("iface %p, filename %s.\n", iface, debugstr_a(filename));

    if (FAILED(hr = d3drm_texture_load(texture, filename, FALSE, &image)))
        return hr;

    return IDirect3DRMTexture3_InitFromImage(&texture->IDirect3DRMTexture3_iface, image);
}

static HRESULT WINAPI d3drm_texture1_InitFromSurface(IDirect3DRMTexture *iface,
        IDirectDrawSurface *surface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, surface %p.\n", iface, surface);

    return IDirect3DRMTexture3_InitFromSurface(&texture->IDirect3DRMTexture3_iface, surface);
}

static HRESULT WINAPI d3drm_texture1_InitFromResource(IDirect3DRMTexture *iface, HRSRC resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture1_Changed(IDirect3DRMTexture *iface, BOOL pixels, BOOL palette)
{
    FIXME("iface %p, pixels %#x, palette %#x stub!\n", iface, pixels, palette);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture1_SetColors(IDirect3DRMTexture *iface, DWORD max_colors)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, max_colors %lu.\n", iface, max_colors);

    return IDirect3DRMTexture3_SetColors(&texture->IDirect3DRMTexture3_iface, max_colors);
}

static HRESULT WINAPI d3drm_texture1_SetShades(IDirect3DRMTexture *iface, DWORD max_shades)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, max_shades %lu.\n", iface, max_shades);

    return IDirect3DRMTexture3_SetShades(&texture->IDirect3DRMTexture3_iface, max_shades);
}

static HRESULT WINAPI d3drm_texture1_SetDecalSize(IDirect3DRMTexture *iface, D3DVALUE width, D3DVALUE height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, width %.8e, height %.8e.\n", iface, width, height);

    return IDirect3DRMTexture3_SetDecalSize(&texture->IDirect3DRMTexture3_iface, width, height);
}

static HRESULT WINAPI d3drm_texture1_SetDecalOrigin(IDirect3DRMTexture *iface, LONG x, LONG y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, x %ld, y %ld.\n", iface, x, y);

    return IDirect3DRMTexture3_SetDecalOrigin(&texture->IDirect3DRMTexture3_iface, x, y);
}

static HRESULT WINAPI d3drm_texture1_SetDecalScale(IDirect3DRMTexture *iface, DWORD scale)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, scale %lu.\n", iface, scale);

    return IDirect3DRMTexture3_SetDecalScale(&texture->IDirect3DRMTexture3_iface, scale);
}

static HRESULT WINAPI d3drm_texture1_SetDecalTransparency(IDirect3DRMTexture *iface, BOOL transparency)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, transparency %#x.\n", iface, transparency);

    return IDirect3DRMTexture3_SetDecalTransparency(&texture->IDirect3DRMTexture3_iface, transparency);
}

static HRESULT WINAPI d3drm_texture1_SetDecalTransparentColor(IDirect3DRMTexture *iface, D3DCOLOR color)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    return IDirect3DRMTexture3_SetDecalTransparentColor(&texture->IDirect3DRMTexture3_iface, color);
}

static HRESULT WINAPI d3drm_texture1_GetDecalSize(IDirect3DRMTexture *iface, D3DVALUE *width, D3DVALUE *height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, width %p, height %p.\n", iface, width, height);

    return IDirect3DRMTexture3_GetDecalSize(&texture->IDirect3DRMTexture3_iface, width, height);
}

static HRESULT WINAPI d3drm_texture1_GetDecalOrigin(IDirect3DRMTexture *iface, LONG *x, LONG *y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return IDirect3DRMTexture3_GetDecalOrigin(&texture->IDirect3DRMTexture3_iface, x, y);
}

static D3DRMIMAGE * WINAPI d3drm_texture1_GetImage(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetImage(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture1_GetShades(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetShades(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture1_GetColors(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetColors(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture1_GetDecalScale(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalScale(&texture->IDirect3DRMTexture3_iface);
}

static BOOL WINAPI d3drm_texture1_GetDecalTransparency(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalTransparency(&texture->IDirect3DRMTexture3_iface);
}

static D3DCOLOR WINAPI d3drm_texture1_GetDecalTransparentColor(IDirect3DRMTexture *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalTransparentColor(&texture->IDirect3DRMTexture3_iface);
}

static const struct IDirect3DRMTextureVtbl d3drm_texture1_vtbl =
{
    d3drm_texture1_QueryInterface,
    d3drm_texture1_AddRef,
    d3drm_texture1_Release,
    d3drm_texture1_Clone,
    d3drm_texture1_AddDestroyCallback,
    d3drm_texture1_DeleteDestroyCallback,
    d3drm_texture1_SetAppData,
    d3drm_texture1_GetAppData,
    d3drm_texture1_SetName,
    d3drm_texture1_GetName,
    d3drm_texture1_GetClassName,
    d3drm_texture1_InitFromFile,
    d3drm_texture1_InitFromSurface,
    d3drm_texture1_InitFromResource,
    d3drm_texture1_Changed,
    d3drm_texture1_SetColors,
    d3drm_texture1_SetShades,
    d3drm_texture1_SetDecalSize,
    d3drm_texture1_SetDecalOrigin,
    d3drm_texture1_SetDecalScale,
    d3drm_texture1_SetDecalTransparency,
    d3drm_texture1_SetDecalTransparentColor,
    d3drm_texture1_GetDecalSize,
    d3drm_texture1_GetDecalOrigin,
    d3drm_texture1_GetImage,
    d3drm_texture1_GetShades,
    d3drm_texture1_GetColors,
    d3drm_texture1_GetDecalScale,
    d3drm_texture1_GetDecalTransparency,
    d3drm_texture1_GetDecalTransparentColor,
};

static HRESULT WINAPI d3drm_texture2_QueryInterface(IDirect3DRMTexture2 *iface, REFIID riid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IDirect3DRMTexture3_QueryInterface(&texture->IDirect3DRMTexture3_iface, riid, out);
}

static ULONG WINAPI d3drm_texture2_AddRef(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_AddRef(&texture->IDirect3DRMTexture3_iface);
}

static ULONG WINAPI d3drm_texture2_Release(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_Release(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture2_Clone(IDirect3DRMTexture2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    return IDirect3DRMTexture3_Clone(&texture->IDirect3DRMTexture3_iface, outer, iid, out);
}

static HRESULT WINAPI d3drm_texture2_AddDestroyCallback(IDirect3DRMTexture2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return IDirect3DRMTexture3_AddDestroyCallback(&texture->IDirect3DRMTexture3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_texture2_DeleteDestroyCallback(IDirect3DRMTexture2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return IDirect3DRMTexture3_DeleteDestroyCallback(&texture->IDirect3DRMTexture3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_texture2_SetAppData(IDirect3DRMTexture2 *iface, DWORD data)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return IDirect3DRMTexture3_SetAppData(&texture->IDirect3DRMTexture3_iface, data);
}

static DWORD WINAPI d3drm_texture2_GetAppData(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetAppData(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture2_SetName(IDirect3DRMTexture2 *iface, const char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return IDirect3DRMTexture3_SetName(&texture->IDirect3DRMTexture3_iface, name);
}

static HRESULT WINAPI d3drm_texture2_GetName(IDirect3DRMTexture2 *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMTexture3_GetName(&texture->IDirect3DRMTexture3_iface, size, name);
}

static HRESULT WINAPI d3drm_texture2_GetClassName(IDirect3DRMTexture2 *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMTexture3_GetClassName(&texture->IDirect3DRMTexture3_iface, size, name);
}

static HRESULT WINAPI d3drm_texture2_InitFromFile(IDirect3DRMTexture2 *iface, const char *filename)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, filename %s.\n", iface, debugstr_a(filename));

    return IDirect3DRMTexture3_InitFromFile(&texture->IDirect3DRMTexture3_iface, filename);
}

static HRESULT WINAPI d3drm_texture2_InitFromSurface(IDirect3DRMTexture2 *iface,
        IDirectDrawSurface *surface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, surface %p.\n", iface, surface);

    return IDirect3DRMTexture3_InitFromSurface(&texture->IDirect3DRMTexture3_iface, surface);
}

static HRESULT WINAPI d3drm_texture2_InitFromResource(IDirect3DRMTexture2 *iface, HRSRC resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_Changed(IDirect3DRMTexture2 *iface, BOOL pixels, BOOL palette)
{
    FIXME("iface %p, pixels %#x, palette %#x stub!\n", iface, pixels, palette);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetColors(IDirect3DRMTexture2 *iface, DWORD max_colors)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, max_colors %lu.\n", iface, max_colors);

    return IDirect3DRMTexture3_SetColors(&texture->IDirect3DRMTexture3_iface, max_colors);
}

static HRESULT WINAPI d3drm_texture2_SetShades(IDirect3DRMTexture2 *iface, DWORD max_shades)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, max_shades %lu.\n", iface, max_shades);

    return IDirect3DRMTexture3_SetShades(&texture->IDirect3DRMTexture3_iface, max_shades);
}

static HRESULT WINAPI d3drm_texture2_SetDecalSize(IDirect3DRMTexture2 *iface, D3DVALUE width, D3DVALUE height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, width %.8e, height %.8e.\n", iface, width, height);

    return IDirect3DRMTexture3_SetDecalSize(&texture->IDirect3DRMTexture3_iface, width, height);
}

static HRESULT WINAPI d3drm_texture2_SetDecalOrigin(IDirect3DRMTexture2 *iface, LONG x, LONG y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, x %ld, y %ld.\n", iface, x, y);

    return IDirect3DRMTexture3_SetDecalOrigin(&texture->IDirect3DRMTexture3_iface, x, y);
}

static HRESULT WINAPI d3drm_texture2_SetDecalScale(IDirect3DRMTexture2 *iface, DWORD scale)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, scale %lu.\n", iface, scale);

    return IDirect3DRMTexture3_SetDecalScale(&texture->IDirect3DRMTexture3_iface, scale);
}

static HRESULT WINAPI d3drm_texture2_SetDecalTransparency(IDirect3DRMTexture2 *iface, BOOL transparency)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, transparency %#x.\n", iface, transparency);

    return IDirect3DRMTexture3_SetDecalTransparency(&texture->IDirect3DRMTexture3_iface, transparency);
}

static HRESULT WINAPI d3drm_texture2_SetDecalTransparentColor(IDirect3DRMTexture2 *iface, D3DCOLOR color)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    return IDirect3DRMTexture3_SetDecalTransparentColor(&texture->IDirect3DRMTexture3_iface, color);
}

static HRESULT WINAPI d3drm_texture2_GetDecalSize(IDirect3DRMTexture2 *iface, D3DVALUE *width, D3DVALUE *height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, width %p, height %p.\n", iface, width, height);

    return IDirect3DRMTexture3_GetDecalSize(&texture->IDirect3DRMTexture3_iface, width, height);
}

static HRESULT WINAPI d3drm_texture2_GetDecalOrigin(IDirect3DRMTexture2 *iface, LONG *x, LONG *y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return IDirect3DRMTexture3_GetDecalOrigin(&texture->IDirect3DRMTexture3_iface, x, y);
}

static D3DRMIMAGE * WINAPI d3drm_texture2_GetImage(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetImage(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture2_GetShades(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetShades(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture2_GetColors(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetColors(&texture->IDirect3DRMTexture3_iface);
}

static DWORD WINAPI d3drm_texture2_GetDecalScale(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalScale(&texture->IDirect3DRMTexture3_iface);
}

static BOOL WINAPI d3drm_texture2_GetDecalTransparency(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalTransparency(&texture->IDirect3DRMTexture3_iface);
}

static D3DCOLOR WINAPI d3drm_texture2_GetDecalTransparentColor(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetDecalTransparentColor(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture2_InitFromImage(IDirect3DRMTexture2 *iface, D3DRMIMAGE *image)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, image %p.\n", iface, image);

    return IDirect3DRMTexture3_InitFromImage(&texture->IDirect3DRMTexture3_iface, image);
}

static HRESULT WINAPI d3drm_texture2_InitFromResource2(IDirect3DRMTexture2 *iface,
        HMODULE module, const char *name, const char *type)
{
    FIXME("iface %p, module %p, name %s, type %s stub!\n",
            iface, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GenerateMIPMap(IDirect3DRMTexture2 *iface, DWORD flags)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirect3DRMTexture3_GenerateMIPMap(&texture->IDirect3DRMTexture3_iface, flags);
}

static const struct IDirect3DRMTexture2Vtbl d3drm_texture2_vtbl =
{
    d3drm_texture2_QueryInterface,
    d3drm_texture2_AddRef,
    d3drm_texture2_Release,
    d3drm_texture2_Clone,
    d3drm_texture2_AddDestroyCallback,
    d3drm_texture2_DeleteDestroyCallback,
    d3drm_texture2_SetAppData,
    d3drm_texture2_GetAppData,
    d3drm_texture2_SetName,
    d3drm_texture2_GetName,
    d3drm_texture2_GetClassName,
    d3drm_texture2_InitFromFile,
    d3drm_texture2_InitFromSurface,
    d3drm_texture2_InitFromResource,
    d3drm_texture2_Changed,
    d3drm_texture2_SetColors,
    d3drm_texture2_SetShades,
    d3drm_texture2_SetDecalSize,
    d3drm_texture2_SetDecalOrigin,
    d3drm_texture2_SetDecalScale,
    d3drm_texture2_SetDecalTransparency,
    d3drm_texture2_SetDecalTransparentColor,
    d3drm_texture2_GetDecalSize,
    d3drm_texture2_GetDecalOrigin,
    d3drm_texture2_GetImage,
    d3drm_texture2_GetShades,
    d3drm_texture2_GetColors,
    d3drm_texture2_GetDecalScale,
    d3drm_texture2_GetDecalTransparency,
    d3drm_texture2_GetDecalTransparentColor,
    d3drm_texture2_InitFromImage,
    d3drm_texture2_InitFromResource2,
    d3drm_texture2_GenerateMIPMap,
};

static HRESULT WINAPI d3drm_texture3_QueryInterface(IDirect3DRMTexture3 *iface, REFIID riid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMTexture)
            || IsEqualGUID(riid, &IID_IDirect3DRMVisual)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &texture->IDirect3DRMTexture_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMTexture2))
    {
        *out = &texture->IDirect3DRMTexture2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMTexture3))
    {
        *out = &texture->IDirect3DRMTexture3_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(riid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI d3drm_texture3_AddRef(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    ULONG refcount = InterlockedIncrement(&texture->obj.ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_texture3_Release(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    ULONG refcount = InterlockedDecrement(&texture->obj.ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        d3drm_texture_destroy(texture);

    return refcount;
}

static HRESULT WINAPI d3drm_texture3_Clone(IDirect3DRMTexture3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_AddDestroyCallback(IDirect3DRMTexture3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&texture->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_texture3_DeleteDestroyCallback(IDirect3DRMTexture3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&texture->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_texture3_SetAppData(IDirect3DRMTexture3 *iface, DWORD data)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    texture->obj.appdata = data;

    return D3DRM_OK;
}

static DWORD WINAPI d3drm_texture3_GetAppData(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p.\n", iface);

    return texture->obj.appdata;
}

static HRESULT WINAPI d3drm_texture3_SetName(IDirect3DRMTexture3 *iface, const char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&texture->obj, name);
}

static HRESULT WINAPI d3drm_texture3_GetName(IDirect3DRMTexture3 *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&texture->obj, size, name);
}

static HRESULT WINAPI d3drm_texture3_GetClassName(IDirect3DRMTexture3 *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&texture->obj, size, name);
}

static HRESULT WINAPI d3drm_texture3_InitFromFile(IDirect3DRMTexture3 *iface, const char *filename)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    D3DRMIMAGE *image;
    HRESULT hr;

    TRACE("iface %p, filename %s.\n", iface, debugstr_a(filename));

    if (FAILED(hr = d3drm_texture_load(texture, filename, TRUE, &image)))
        return hr;

    return IDirect3DRMTexture3_InitFromImage(iface, image);
}

static HRESULT WINAPI d3drm_texture3_InitFromSurface(IDirect3DRMTexture3 *iface,
        IDirectDrawSurface *surface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, surface %p.\n", iface, surface);

    if (!surface)
        return D3DRMERR_BADOBJECT;

    /* d3drm intentionally leaks a reference to IDirect3DRM here if texture has already been initialized. */
    IDirect3DRM_AddRef(texture->d3drm);

    if (texture->image || texture->surface)
        return D3DRMERR_BADOBJECT;

    texture->surface = surface;
    IDirectDrawSurface_AddRef(texture->surface);
    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_texture3_InitFromResource(IDirect3DRMTexture3 *iface, HRSRC resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_Changed(IDirect3DRMTexture3 *iface,
        DWORD flags, DWORD rect_count, RECT *rects)
{
    FIXME("iface %p, flags %#lx, rect_count %lu, rects %p stub!\n", iface, flags, rect_count, rects);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetColors(IDirect3DRMTexture3 *iface, DWORD max_colors)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    TRACE("iface %p, max_colors %lu\n", iface, max_colors);

    texture->max_colors= max_colors;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_SetShades(IDirect3DRMTexture3 *iface, DWORD max_shades)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    TRACE("iface %p, max_shades %lu\n", iface, max_shades);

    texture->max_shades = max_shades;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_SetDecalSize(IDirect3DRMTexture3 *iface, D3DVALUE width, D3DVALUE height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, width %.8e, height %.8e.\n", iface, width, height);

    texture->decal_width = width;
    texture->decal_height = height;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_SetDecalOrigin(IDirect3DRMTexture3 *iface, LONG x, LONG y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, x %ld, y %ld\n", iface, x, y);

    texture->decal_x = x;
    texture->decal_y = y;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_SetDecalScale(IDirect3DRMTexture3 *iface, DWORD scale)
{
    FIXME("iface %p, scale %lu stub!\n", iface, scale);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalTransparency(IDirect3DRMTexture3 *iface, BOOL transparency)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, transparency %#x.\n", iface, transparency);

    texture->transparency = transparency;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_SetDecalTransparentColor(IDirect3DRMTexture3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetDecalSize(IDirect3DRMTexture3 *iface, D3DVALUE *width, D3DVALUE *height)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, width %p, height %p.\n", iface, width, height);

    *width = texture->decal_width;
    *height = texture->decal_height;

    return S_OK;
}

static HRESULT WINAPI d3drm_texture3_GetDecalOrigin(IDirect3DRMTexture3 *iface, LONG *x, LONG *y)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, x %p, y %p\n", iface, x, y);

    *x = texture->decal_x;
    *y = texture->decal_y;

    return S_OK;
}

static D3DRMIMAGE * WINAPI d3drm_texture3_GetImage(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p.\n", iface);

    return texture->image;
}

static DWORD WINAPI d3drm_texture3_GetShades(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    TRACE("iface %p\n", iface);
    return texture->max_shades;
}

static DWORD WINAPI d3drm_texture3_GetColors(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    TRACE("iface %p\n", iface);
    return texture->max_colors;
}

static DWORD WINAPI d3drm_texture3_GetDecalScale(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_texture3_GetDecalTransparency(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p.\n", iface);

    return texture->transparency;
}

static D3DCOLOR WINAPI d3drm_texture3_GetDecalTransparentColor(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_texture3_InitFromImage(IDirect3DRMTexture3 *iface, D3DRMIMAGE *image)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, image %p.\n", iface, image);

    if (!d3drm_validate_image(image))
        return D3DRMERR_BADOBJECT;

    /* d3drm intentionally leaks a reference to IDirect3DRM here if texture has already been initialized. */
    IDirect3DRM_AddRef(texture->d3drm);

    if (texture->image || texture->surface)
        return D3DRMERR_BADOBJECT;

    texture->image = image;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_texture3_InitFromResource2(IDirect3DRMTexture3 *iface,
        HMODULE module, const char *name, const char *type)
{
    FIXME("iface %p, module %p, name %s, type %s stub!\n",
            iface, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GenerateMIPMap(IDirect3DRMTexture3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#lx stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetSurface(IDirect3DRMTexture3 *iface,
        DWORD flags, IDirectDrawSurface **surface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, flags %#lx, surface %p.\n", iface, flags, surface);

    if (flags)
        FIXME("unexpected flags %#lx.\n", flags);

    if (!surface)
        return D3DRMERR_BADVALUE;

    if (texture->image)
        return D3DRMERR_NOTCREATEDFROMDDS;

    *surface = texture->surface;
    IDirectDrawSurface_AddRef(*surface);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_texture3_SetCacheOptions(IDirect3DRMTexture3 *iface, LONG importance, DWORD flags)
{
    FIXME("iface %p, importance %ld, flags %#lx stub!\n", iface, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetCacheOptions(IDirect3DRMTexture3 *iface,
        LONG *importance, DWORD *flags)
{
    FIXME("iface %p, importance %p, flags %p stub!\n", iface, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDownsampleCallback(IDirect3DRMTexture3 *iface,
        D3DRMDOWNSAMPLECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetValidationCallback(IDirect3DRMTexture3 *iface,
        D3DRMVALIDATIONCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static const struct IDirect3DRMTexture3Vtbl d3drm_texture3_vtbl =
{
    d3drm_texture3_QueryInterface,
    d3drm_texture3_AddRef,
    d3drm_texture3_Release,
    d3drm_texture3_Clone,
    d3drm_texture3_AddDestroyCallback,
    d3drm_texture3_DeleteDestroyCallback,
    d3drm_texture3_SetAppData,
    d3drm_texture3_GetAppData,
    d3drm_texture3_SetName,
    d3drm_texture3_GetName,
    d3drm_texture3_GetClassName,
    d3drm_texture3_InitFromFile,
    d3drm_texture3_InitFromSurface,
    d3drm_texture3_InitFromResource,
    d3drm_texture3_Changed,
    d3drm_texture3_SetColors,
    d3drm_texture3_SetShades,
    d3drm_texture3_SetDecalSize,
    d3drm_texture3_SetDecalOrigin,
    d3drm_texture3_SetDecalScale,
    d3drm_texture3_SetDecalTransparency,
    d3drm_texture3_SetDecalTransparentColor,
    d3drm_texture3_GetDecalSize,
    d3drm_texture3_GetDecalOrigin,
    d3drm_texture3_GetImage,
    d3drm_texture3_GetShades,
    d3drm_texture3_GetColors,
    d3drm_texture3_GetDecalScale,
    d3drm_texture3_GetDecalTransparency,
    d3drm_texture3_GetDecalTransparentColor,
    d3drm_texture3_InitFromImage,
    d3drm_texture3_InitFromResource2,
    d3drm_texture3_GenerateMIPMap,
    d3drm_texture3_GetSurface,
    d3drm_texture3_SetCacheOptions,
    d3drm_texture3_GetCacheOptions,
    d3drm_texture3_SetDownsampleCallback,
    d3drm_texture3_SetValidationCallback,
};

HRESULT d3drm_texture_create(struct d3drm_texture **texture, IDirect3DRM *d3drm)
{
    static const char classname[] = "Texture";
    struct d3drm_texture *object;

    TRACE("texture %p.\n", texture);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMTexture_iface.lpVtbl = &d3drm_texture1_vtbl;
    object->IDirect3DRMTexture2_iface.lpVtbl = &d3drm_texture2_vtbl;
    object->IDirect3DRMTexture3_iface.lpVtbl = &d3drm_texture3_vtbl;
    object->d3drm = d3drm;
    object->max_colors = 8;
    object->max_shades = 16;
    object->transparency = FALSE;
    object->decal_width = 1.0f;
    object->decal_height = 1.0f;

    d3drm_object_init(&object->obj, classname);

    *texture = object;

    return D3DRM_OK;
}
