/*
 * Copyright 2014 Michael Müller
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
#include "wined3d_private.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static void* txc_dxtn_handle;
static void (*pfetch_2d_texel_rgba_dxt1)(int srcRowStride, const BYTE *pixData, int i, int j, DWORD *texel);
static void (*pfetch_2d_texel_rgba_dxt3)(int srcRowStride, const BYTE *pixData, int i, int j, DWORD *texel);
static void (*pfetch_2d_texel_rgba_dxt5)(int srcRowStride, const BYTE *pixData, int i, int j, DWORD *texel);
static void (*ptx_compress_dxtn)(int comps, int width, int height, const BYTE *srcPixData,
                                 GLenum destformat, BYTE *dest, int dstRowStride);

static inline BOOL dxt1_to_x8r8g8b8(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt1 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt1(0, src + (y / 4) * pitch_in + (x / 4) * 8,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = (color & 0xff00ff00) | ((color & 0xff) << 16) |
                              ((color & 0xff0000) >> 16);
            }
            else
            {
                dst_line[x] = 0xff000000 | ((color & 0xff) << 16) |
                              (color & 0xff00) | ((color & 0xff0000) >> 16);
            }
        }
    }

    return TRUE;
}

static inline BOOL dxt1_to_x4r4g4b4(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt1 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt1(0, src + (y / 4) * pitch_in + (x / 4) * 16,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = ((color & 0xf0000000) >> 16) | ((color & 0xf00000) >> 20) |
                              ((color & 0xf000) >> 8) | ((color & 0xf0) << 4);
            }
            else
            {
                dst_line[x] = 0xf000  | ((color & 0xf00000) >> 20) |
                              ((color & 0xf000) >> 8) | ((color & 0xf0) << 4);
            }
        }
    }

    return TRUE;
}

static inline BOOL dxt1_to_x1r5g5b5(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt1 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt1(0, src + (y / 4) * pitch_in + (x / 4) * 16,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = ((color & 0x80000000) >> 16) | ((color & 0xf80000) >> 19) |
                              ((color & 0xf800) >> 6) | ((color & 0xf8) << 7);
            }
            else
            {
                dst_line[x] = 0x8000 | ((color & 0xf80000) >> 19) |
                              ((color & 0xf800) >> 6) | ((color & 0xf8) << 7);
            }
        }
    }

    return TRUE;
}

static inline BOOL dxt3_to_x8r8g8b8(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt3 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt3(0, src + (y / 4) * pitch_in + (x / 4) * 16,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = (color & 0xff00ff00) | ((color & 0xff) << 16) |
                              ((color & 0xff0000) >> 16);
            }
            else
            {
                dst_line[x] = 0xff000000 | ((color & 0xff) << 16) |
                              (color & 0xff00) | ((color & 0xff0000) >> 16);
            }
        }
    }

    return TRUE;
}

static inline BOOL dxt3_to_x4r4g4b4(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt3 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt3(0, src + (y / 4) * pitch_in + (x / 4) * 16,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = ((color & 0xf0000000) >> 16) | ((color & 0xf00000) >> 20) |
                              ((color & 0xf000) >> 8) | ((color & 0xf0) << 4);
            }
            else
            {
                dst_line[x] = 0xf000  | ((color & 0xf00000) >> 20) |
                              ((color & 0xf000) >> 8) | ((color & 0xf0) << 4);
            }
        }
    }

    return TRUE;
}

static inline BOOL dxt5_to_x8r8g8b8(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, BOOL alpha)
{
    unsigned int x, y;
    DWORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* pfetch_2d_texel_rgba_dxt5 doesn't correctly handle pitch */
            pfetch_2d_texel_rgba_dxt5(0, src + (y / 4) * pitch_in + (x / 4) * 16,
                                      x & 3, y & 3, &color);
            if (alpha)
            {
                dst_line[x] = (color & 0xff00ff00) | ((color & 0xff) << 16) |
                              ((color & 0xff0000) >> 16);
            }
            else
            {
                dst_line[x] = 0xff000000 | ((color & 0xff) << 16) |
                              (color & 0xff00) | ((color & 0xff0000) >> 16);
            }
        }
    }

    return TRUE;
}

static inline BOOL x8r8g8b8_to_dxtn(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, GLenum destformat, BOOL alpha)
{
    unsigned int x, y;
    DWORD color, *tmp;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    tmp = HeapAlloc(GetProcessHeap(), 0, h * w * sizeof(DWORD));
    if (!tmp)
    {
        ERR("Failed to allocate memory for conversion\n");
        return FALSE;
    }

    for (y = 0; y < h; ++y)
    {
        const DWORD *src_line = (const DWORD *)(src + y * pitch_in);
        DWORD *dst_line = tmp + y * w;
        for (x = 0; x < w; ++x)
        {
            color = src_line[x];
            if (alpha)
            {
                dst_line[x] = (color & 0xff00ff00) | ((color & 0xff) << 16) |
                              ((color & 0xff0000) >> 16);
            }
            else
            {
                dst_line[x] = 0xff000000 | ((color & 0xff) << 16) |
                              (color & 0xff00) | ((color & 0xff0000) >> 16);
            }
        }
    }

    ptx_compress_dxtn(4, w, h, (BYTE *)tmp, destformat, dst, pitch_out);
    HeapFree(GetProcessHeap(), 0, tmp);
    return TRUE;
}

static inline BOOL x1r5g5b5_to_dxtn(const BYTE *src, BYTE *dst, DWORD pitch_in,
        DWORD pitch_out, unsigned int w, unsigned int h, GLenum destformat, BOOL alpha)
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    unsigned int x, y;
    DWORD *tmp;
    WORD color;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    tmp = HeapAlloc(GetProcessHeap(), 0, h * w * sizeof(DWORD));
    if (!tmp)
    {
        ERR("Failed to allocate memory for conversion\n");
        return FALSE;
    }

    for (y = 0; y < h; ++y)
    {
        const WORD *src_line = (const WORD *)(src + y * pitch_in);
        DWORD *dst_line = tmp + y * w;
        for (x = 0; x < w; ++x)
        {
            color = src_line[x];
            if (alpha)
            {
                dst_line[x] = ((color & 0x8000) ? 0xff000000 : 0) |
                              convert_5to8[(color & 0x001f)] << 16 |
                              convert_5to8[(color & 0x03e0) >> 5] << 8 |
                              convert_5to8[(color & 0x7c00) >> 10];
            }
            else
            {
                dst_line[x] = 0xff000000 |
                              convert_5to8[(color & 0x001f)] << 16 |
                              convert_5to8[(color & 0x03e0) >> 5] << 8 |
                              convert_5to8[(color & 0x7c00) >> 10];
            }
        }
    }

    ptx_compress_dxtn(4, w, h, (BYTE *)tmp, destformat, dst, pitch_out);
    HeapFree(GetProcessHeap(), 0, tmp);
    return TRUE;
}

BOOL wined3d_dxt1_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return dxt1_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return dxt1_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, FALSE);
        case WINED3DFMT_B4G4R4A4_UNORM:
            return dxt1_to_x4r4g4b4(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B4G4R4X4_UNORM:
            return dxt1_to_x4r4g4b4(src, dst, pitch_in, pitch_out, w, h, FALSE);
        case WINED3DFMT_B5G5R5A1_UNORM:
            return dxt1_to_x1r5g5b5(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B5G5R5X1_UNORM:
            return dxt1_to_x1r5g5b5(src, dst, pitch_in, pitch_out, w, h, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format DXT1 to %s.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxt3_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return dxt3_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return dxt3_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, FALSE);
        case WINED3DFMT_B4G4R4A4_UNORM:
            return dxt3_to_x4r4g4b4(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B4G4R4X4_UNORM:
            return dxt3_to_x4r4g4b4(src, dst, pitch_in, pitch_out, w, h, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format DXT3 to %s.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxt5_decode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return dxt5_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return dxt5_to_x8r8g8b8(src, dst, pitch_in, pitch_out, w, h, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format DXT5 to %s.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxt1_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGB_S3TC_DXT1_EXT, FALSE);
        case WINED3DFMT_B5G5R5A1_UNORM:
            return x1r5g5b5_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, TRUE);
        case WINED3DFMT_B5G5R5X1_UNORM:
            return x1r5g5b5_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGB_S3TC_DXT1_EXT, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format %s to DXT1.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxt3_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format %s to DXT3.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxt5_encode(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
        enum wined3d_format_id format, unsigned int w, unsigned int h)
{
    if (!txc_dxtn_handle)
        return FALSE;

    switch (format)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, TRUE);
        case WINED3DFMT_B8G8R8X8_UNORM:
            return x8r8g8b8_to_dxtn(src, dst, pitch_in, pitch_out, w, h,
                                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, FALSE);
        default:
            break;
    }

    FIXME("Cannot find a conversion function from format %s to DXT5.\n", debug_d3dformat(format));
    return FALSE;
}

BOOL wined3d_dxtn_init(void)
{
    static const char *soname[] =
    {
#ifdef SONAME_LIBTXC_DXTN
        SONAME_LIBTXC_DXTN,
#endif
#ifdef __APPLE__
        "libtxc_dxtn.dylib",
        "libtxc_dxtn_s2tc.dylib",
#endif
        "libtxc_dxtn.so",
        "libtxc_dxtn_s2tc.so.0"
    };
    int i;

    for (i = 0; i < sizeof(soname)/sizeof(soname[0]); i++)
    {
        txc_dxtn_handle = wine_dlopen(soname[i], RTLD_NOW, NULL, 0);
        if (txc_dxtn_handle) break;
    }

    if (!txc_dxtn_handle)
    {
        FIXME("Wine cannot find the txc_dxtn library, DXTn software support unavailable.\n");
        return FALSE;
    }

    #define LOAD_FUNCPTR(f) \
        if (!(p##f = wine_dlsym(txc_dxtn_handle, #f, NULL, 0))) \
        { \
            ERR("Can't find symbol %s , DXTn software support unavailable.\n", #f); \
            goto error; \
        }

    LOAD_FUNCPTR(fetch_2d_texel_rgba_dxt1);
    LOAD_FUNCPTR(fetch_2d_texel_rgba_dxt3);
    LOAD_FUNCPTR(fetch_2d_texel_rgba_dxt5);
    LOAD_FUNCPTR(tx_compress_dxtn);

    #undef LOAD_FUNCPTR
    return TRUE;

error:
    wine_dlclose(txc_dxtn_handle, NULL, 0);
    txc_dxtn_handle = NULL;
    return FALSE;
}

BOOL wined3d_dxtn_supported(void)
{
    return (txc_dxtn_handle != NULL);
}

void wined3d_dxtn_free(void)
{
    if (txc_dxtn_handle)
        wine_dlclose(txc_dxtn_handle, NULL, 0);
}
