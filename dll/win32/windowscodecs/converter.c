/*
 * Copyright 2009 Vincent Povirk
 * Copyright 2016 Dmitry Timoshkov
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

#include <stdarg.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct FormatConverter;

enum pixelformat {
    format_1bppIndexed,
    format_2bppIndexed,
    format_4bppIndexed,
    format_8bppIndexed,
    format_BlackWhite,
    format_2bppGray,
    format_4bppGray,
    format_8bppGray,
    format_16bppGray,
    format_16bppBGR555,
    format_16bppBGR565,
    format_16bppBGRA5551,
    format_24bppBGR,
    format_24bppRGB,
    format_32bppGrayFloat,
    format_32bppBGR,
    format_32bppRGB,
    format_32bppBGRA,
    format_32bppRGBA,
    format_32bppPBGRA,
    format_32bppPRGBA,
    format_48bppRGB,
    format_64bppRGBA,
    format_32bppCMYK,
};

typedef HRESULT (*copyfunc)(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format);

struct pixelformatinfo {
    enum pixelformat format;
    const WICPixelFormatGUID *guid;
    copyfunc copy_function;
    BOOL is_indexed_format;
};

typedef struct FormatConverter {
    IWICFormatConverter IWICFormatConverter_iface;
    LONG ref;
    IWICBitmapSource *source;
    const struct pixelformatinfo *dst_format, *src_format;
    WICBitmapDitherType dither;
    double alpha_threshold;
    IWICPalette *palette;
    CRITICAL_SECTION lock; /* must be held when initialized */
} FormatConverter;

/* https://www.w3.org/Graphics/Color/srgb */
static inline float to_sRGB_component(float f)
{
    if (f <= 0.0031308f) return 12.92f * f;
    return 1.055f * powf(f, 1.0f/2.4f) - 0.055f;
}

#if 0 /* FIXME: enable once needed */
static inline float from_sRGB_component(float f)
{
    if (f <= 0.04045f) return f / 12.92f;
    return powf((f + 0.055f) / 1.055f, 2.4f);
}

static void from_sRGB(BYTE *bgr)
{
    float r, g, b;

    r = bgr[2] / 255.0f;
    g = bgr[1] / 255.0f;
    b = bgr[0] / 255.0f;

    r = from_sRGB_component(r);
    g = from_sRGB_component(g);
    b = from_sRGB_component(b);

    bgr[2] = (BYTE)(r * 255.0f);
    bgr[1] = (BYTE)(g * 255.0f);
    bgr[0] = (BYTE)(b * 255.0f);
}

static void to_sRGB(BYTE *bgr)
{
    float r, g, b;

    r = bgr[2] / 255.0f;
    g = bgr[1] / 255.0f;
    b = bgr[0] / 255.0f;

    r = to_sRGB_component(r);
    g = to_sRGB_component(g);
    b = to_sRGB_component(b);

    bgr[2] = (BYTE)(r * 255.0f);
    bgr[1] = (BYTE)(g * 255.0f);
    bgr[0] = (BYTE)(b * 255.0f);
}
#endif

static inline FormatConverter *impl_from_IWICFormatConverter(IWICFormatConverter *iface)
{
    return CONTAINING_RECORD(iface, FormatConverter, IWICFormatConverter_iface);
}

static HRESULT copypixels_to_32bppBGRA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_1bppIndexed:
    case format_BlackWhite:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[2];
            IWICPalette *palette;
            UINT actualcolors;

            res = PaletteImpl_Create(&palette);
            if (FAILED(res)) return res;

            if (source_format == format_1bppIndexed)
                res = IWICBitmapSource_CopyPalette(This->source, palette);
            else
                res = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedBW, FALSE);

            if (SUCCEEDED(res))
                res = IWICPalette_GetColors(palette, 2, colors, &actualcolors);

            IWICPalette_Release(palette);
            if (FAILED(res)) return res;

            srcstride = (prc->Width+7)/8;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x+=8) {
                        BYTE srcval;
                        srcval=*srcbyte++;
                        *dstpixel++ = colors[srcval>>7&1];
                        if (x+1 < prc->Width) *dstpixel++ = colors[srcval>>6&1];
                        if (x+2 < prc->Width) *dstpixel++ = colors[srcval>>5&1];
                        if (x+3 < prc->Width) *dstpixel++ = colors[srcval>>4&1];
                        if (x+4 < prc->Width) *dstpixel++ = colors[srcval>>3&1];
                        if (x+5 < prc->Width) *dstpixel++ = colors[srcval>>2&1];
                        if (x+6 < prc->Width) *dstpixel++ = colors[srcval>>1&1];
                        if (x+7 < prc->Width) *dstpixel++ = colors[srcval&1];
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_2bppIndexed:
    case format_2bppGray:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[4];
            IWICPalette *palette;
            UINT actualcolors;

            res = PaletteImpl_Create(&palette);
            if (FAILED(res)) return res;

            if (source_format == format_2bppIndexed)
                res = IWICBitmapSource_CopyPalette(This->source, palette);
            else
                res = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray4, FALSE);

            if (SUCCEEDED(res))
                res = IWICPalette_GetColors(palette, 4, colors, &actualcolors);

            IWICPalette_Release(palette);
            if (FAILED(res)) return res;

            srcstride = (prc->Width+3)/4;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x+=4) {
                        BYTE srcval;
                        srcval=*srcbyte++;
                        *dstpixel++ = colors[srcval>>6];
                        if (x+1 < prc->Width) *dstpixel++ = colors[srcval>>4&0x3];
                        if (x+2 < prc->Width) *dstpixel++ = colors[srcval>>2&0x3];
                        if (x+3 < prc->Width) *dstpixel++ = colors[srcval&0x3];
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_4bppIndexed:
    case format_4bppGray:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[16];
            IWICPalette *palette;
            UINT actualcolors;

            res = PaletteImpl_Create(&palette);
            if (FAILED(res)) return res;

            if (source_format == format_4bppIndexed)
                res = IWICBitmapSource_CopyPalette(This->source, palette);
            else
                res = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray16, FALSE);

            if (SUCCEEDED(res))
                res = IWICPalette_GetColors(palette, 16, colors, &actualcolors);

            IWICPalette_Release(palette);
            if (FAILED(res)) return res;

            srcstride = (prc->Width+1)/2;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x+=2) {
                        BYTE srcval;
                        srcval=*srcbyte++;
                        *dstpixel++ = colors[srcval>>4];
                        if (x+1 < prc->Width) *dstpixel++ = colors[srcval&0xf];
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_8bppGray:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++)
                    {
                        *dstpixel++ = 0xff000000|(*srcbyte<<16)|(*srcbyte<<8)|*srcbyte;
                        srcbyte++;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_8bppIndexed:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[256];
            IWICPalette *palette;
            UINT actualcolors;

            res = PaletteImpl_Create(&palette);
            if (FAILED(res)) return res;

            res = IWICBitmapSource_CopyPalette(This->source, palette);
            if (SUCCEEDED(res))
                res = IWICPalette_GetColors(palette, 256, colors, &actualcolors);

            IWICPalette_Release(palette);

            if (FAILED(res)) return res;

            srcstride = prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++)
                        *dstpixel++ = colors[*srcbyte++];
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_16bppGray:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = prc->Width * 2;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte = srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++)
                    {
                        srcbyte++;
                        *dstpixel++ = 0xff000000|(*srcbyte<<16)|(*srcbyte<<8)|*srcbyte;
                        srcbyte++;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_16bppBGR555:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const WORD *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 2 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=(const WORD*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++) {
                        WORD srcval;
                        srcval=*srcpixel++;
                        *dstpixel++=0xff000000 | /* constant 255 alpha */
                                    ((srcval << 9) & 0xf80000) | /* r */
                                    ((srcval << 4) & 0x070000) | /* r - 3 bits */
                                    ((srcval << 6) & 0x00f800) | /* g */
                                    ((srcval << 1) & 0x000700) | /* g - 3 bits */
                                    ((srcval << 3) & 0x0000f8) | /* b */
                                    ((srcval >> 2) & 0x000007);  /* b - 3 bits */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_16bppBGR565:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const WORD *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 2 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=(const WORD*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++) {
                        WORD srcval;
                        srcval=*srcpixel++;
                        *dstpixel++=0xff000000 | /* constant 255 alpha */
                                    ((srcval << 8) & 0xf80000) | /* r */
                                    ((srcval << 3) & 0x070000) | /* r - 3 bits */
                                    ((srcval << 5) & 0x00fc00) | /* g */
                                    ((srcval >> 1) & 0x000300) | /* g - 2 bits */
                                    ((srcval << 3) & 0x0000f8) | /* b */
                                    ((srcval >> 2) & 0x000007);  /* b - 3 bits */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_16bppBGRA5551:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const WORD *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 2 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=(const WORD*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++) {
                        WORD srcval;
                        srcval=*srcpixel++;
                        *dstpixel++=((srcval & 0x8000) ? 0xff000000 : 0) | /* alpha */
                                    ((srcval << 9) & 0xf80000) | /* r */
                                    ((srcval << 4) & 0x070000) | /* r - 3 bits */
                                    ((srcval << 6) & 0x00f800) | /* g */
                                    ((srcval << 1) & 0x000700) | /* g - 3 bits */
                                    ((srcval << 3) & 0x0000f8) | /* b */
                                    ((srcval >> 2) & 0x000007);  /* b - 3 bits */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_24bppBGR:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            BYTE *dstpixel;

            srcstride = 3 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=srcrow;
                    dstpixel=dstrow;
                    for (x=0; x<prc->Width; x++) {
                        *dstpixel++=*srcpixel++; /* blue */
                        *dstpixel++=*srcpixel++; /* green */
                        *dstpixel++=*srcpixel++; /* red */
                        *dstpixel++=255; /* alpha */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_24bppRGB:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            BYTE *dstpixel;
            BYTE tmppixel[3];

            srcstride = 3 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=srcrow;
                    dstpixel=dstrow;
                    for (x=0; x<prc->Width; x++) {
                        tmppixel[0]=*srcpixel++; /* red */
                        tmppixel[1]=*srcpixel++; /* green */
                        tmppixel[2]=*srcpixel++; /* blue */

                        *dstpixel++=tmppixel[2]; /* blue */
                        *dstpixel++=tmppixel[1]; /* green */
                        *dstpixel++=tmppixel[0]; /* red */
                        *dstpixel++=255; /* alpha */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_32bppBGR:
        if (prc)
        {
            HRESULT res;
            INT x, y;

            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;

            /* set all alpha values to 255 */
            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                    pbBuffer[cbStride*y+4*x+3] = 0xff;
        }
        return S_OK;
    case format_32bppRGBA:
        if (prc)
        {
            HRESULT res;
            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;
            reverse_bgr8(4, pbBuffer, prc->Width, prc->Height, cbStride);
        }
        return S_OK;
    case format_32bppBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    case format_32bppPBGRA:
        if (prc)
        {
            HRESULT res;
            INT x, y;

            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;

            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                {
                    BYTE alpha = pbBuffer[cbStride*y+4*x+3];
                    if (alpha != 0 && alpha != 255)
                    {
                        pbBuffer[cbStride*y+4*x] = pbBuffer[cbStride*y+4*x] * 255 / alpha;
                        pbBuffer[cbStride*y+4*x+1] = pbBuffer[cbStride*y+4*x+1] * 255 / alpha;
                        pbBuffer[cbStride*y+4*x+2] = pbBuffer[cbStride*y+4*x+2] * 255 / alpha;
                    }
                }
        }
        return S_OK;
    case format_48bppRGB:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 6 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++) {
                        BYTE red, green, blue;
                        srcpixel++; red = *srcpixel++;
                        srcpixel++; green = *srcpixel++;
                        srcpixel++; blue = *srcpixel++;
                        *dstpixel++=0xff000000|red<<16|green<<8|blue;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_64bppRGBA:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 8 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++) {
                        BYTE red, green, blue, alpha;
                        srcpixel++; red = *srcpixel++;
                        srcpixel++; green = *srcpixel++;
                        srcpixel++; blue = *srcpixel++;
                        srcpixel++; alpha = *srcpixel++;
                        *dstpixel++=alpha<<24|red<<16|green<<8|blue;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    case format_32bppCMYK:
        if (prc)
        {
            HRESULT res;
            UINT x, y;

            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;

            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                {
                    BYTE *pixel = pbBuffer+cbStride*y+4*x;
                    BYTE c=pixel[0], m=pixel[1], y=pixel[2], k=pixel[3];
                    pixel[0] = (255-y)*(255-k)/255; /* blue */
                    pixel[1] = (255-m)*(255-k)/255; /* green */
                    pixel[2] = (255-c)*(255-k)/255; /* red */
                    pixel[3] = 255; /* alpha */
                }
        }
        return S_OK;
    default:
        FIXME("Unimplemented conversion path!\n");
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_32bppRGBA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_32bppRGB:
        if (prc)
        {
            INT x, y;

            hr = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(hr)) return hr;

            /* set all alpha values to 255 */
            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                    pbBuffer[cbStride*y+4*x+3] = 0xff;
        }
        return S_OK;

    case format_32bppRGBA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;

    case format_32bppPRGBA:
        if (prc)
        {
            INT x, y;

            hr = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(hr)) return hr;

            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                {
                    BYTE alpha = pbBuffer[cbStride*y+4*x+3];
                    if (alpha != 0 && alpha != 255)
                    {
                        pbBuffer[cbStride*y+4*x] = pbBuffer[cbStride*y+4*x] * 255 / alpha;
                        pbBuffer[cbStride*y+4*x+1] = pbBuffer[cbStride*y+4*x+1] * 255 / alpha;
                        pbBuffer[cbStride*y+4*x+2] = pbBuffer[cbStride*y+4*x+2] * 255 / alpha;
                    }
                }
        }
        return S_OK;

    default:
        hr = copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
        if (SUCCEEDED(hr) && prc)
              reverse_bgr8(4, pbBuffer, prc->Width, prc->Height, cbStride);
        return hr;
    }
}

static HRESULT copypixels_to_32bppBGR(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_32bppBGR:
    case format_32bppBGRA:
    case format_32bppPBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        return copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
    }
}

static HRESULT copypixels_to_32bppRGB(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_32bppRGB:
    case format_32bppRGBA:
    case format_32bppPRGBA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        return copypixels_to_32bppRGBA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
    }
}

static HRESULT copypixels_to_32bppPBGRA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_32bppPBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        hr = copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
        if (SUCCEEDED(hr) && prc)
        {
            INT x, y;

            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                {
                    BYTE alpha = pbBuffer[cbStride*y+4*x+3];
                    if (alpha != 255)
                    {
                        pbBuffer[cbStride*y+4*x] = (pbBuffer[cbStride*y+4*x] * alpha + 127) / 255;
                        pbBuffer[cbStride*y+4*x+1] = (pbBuffer[cbStride*y+4*x+1] * alpha + 127) / 255;
                        pbBuffer[cbStride*y+4*x+2] = (pbBuffer[cbStride*y+4*x+2] * alpha + 127) / 255;
                    }
                }
        }
        return hr;
    }
}

static HRESULT copypixels_to_32bppPRGBA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_32bppPRGBA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        hr = copypixels_to_32bppRGBA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
        if (SUCCEEDED(hr) && prc)
        {
            INT x, y;

            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                {
                    BYTE alpha = pbBuffer[cbStride*y+4*x+3];
                    if (alpha != 255)
                    {
                        pbBuffer[cbStride*y+4*x] = (pbBuffer[cbStride*y+4*x] * alpha + 127) / 255;
                        pbBuffer[cbStride*y+4*x+1] = (pbBuffer[cbStride*y+4*x+1] * alpha + 127) / 255;
                        pbBuffer[cbStride*y+4*x+2] = (pbBuffer[cbStride*y+4*x+2] * alpha + 127) / 255;
                    }
                }
        }
        return hr;
    }
}

static HRESULT copypixels_to_24bppBGR(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_24bppBGR:
    case format_24bppRGB:
        if (prc)
        {
            hr = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (SUCCEEDED(hr) && source_format == format_24bppRGB)
              reverse_bgr8(3, pbBuffer, prc->Width, prc->Height, cbStride);
            return hr;
        }
        return S_OK;
    case format_32bppBGR:
    case format_32bppBGRA:
    case format_32bppPBGRA:
    case format_32bppRGBA:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            BYTE *dstpixel;

            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;

                if (source_format == format_32bppRGBA)
                {
                    for (y = 0; y < prc->Height; y++)
                    {
                        srcpixel = srcrow;
                        dstpixel = dstrow;
                        for (x = 0; x < prc->Width; x++) {
                            *dstpixel++ = srcpixel[2]; /* blue */
                            *dstpixel++ = srcpixel[1]; /* green */
                            *dstpixel++ = srcpixel[0]; /* red */
                            srcpixel += 4;
                        }
                        srcrow += srcstride;
                        dstrow += cbStride;
                    }
                }
                else
                {
                    for (y = 0; y < prc->Height; y++)
                    {
                        srcpixel = srcrow;
                        dstpixel = dstrow;
                        for (x = 0; x < prc->Width; x++) {
                            *dstpixel++ = *srcpixel++; /* blue */
                            *dstpixel++ = *srcpixel++; /* green */
                            *dstpixel++ = *srcpixel++; /* red */
                            srcpixel++; /* alpha */
                        }
                        srcrow += srcstride;
                        dstrow += cbStride;
                    }
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;

    case format_32bppGrayFloat:
        if (prc)
        {
            BYTE *srcdata;
            UINT srcstride, srcdatasize;

            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            hr = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(hr))
            {
                INT x, y;
                BYTE *src = srcdata, *dst = pbBuffer;

                for (y = 0; y < prc->Height; y++)
                {
                    float *gray_float = (float *)src;
                    BYTE *bgr = dst;

                    for (x = 0; x < prc->Width; x++)
                    {
                        BYTE gray = (BYTE)floorf(to_sRGB_component(gray_float[x]) * 255.0f + 0.51f);
                        *bgr++ = gray;
                        *bgr++ = gray;
                        *bgr++ = gray;
                    }
                    src += srcstride;
                    dst += cbStride;
                }
            }

            free(srcdata);

            return hr;
        }
        return S_OK;

    case format_32bppCMYK:
        if (prc)
        {
            BYTE *srcdata;
            UINT srcstride, srcdatasize;

            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            hr = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);
            if (SUCCEEDED(hr))
            {
                INT x, y;
                BYTE *src = srcdata, *dst = pbBuffer;

                for (y = 0; y < prc->Height; y++)
                {
                    BYTE *cmyk = src;
                    BYTE *bgr = dst;

                    for (x = 0; x < prc->Width; x++)
                    {
                        BYTE c = cmyk[0], m = cmyk[1], y = cmyk[2], k = cmyk[3];
                        bgr[0] = (255 - y) * (255 - k) / 255; /* B */
                        bgr[1] = (255 - m) * (255 - k) / 255; /* G */
                        bgr[2] = (255 - c) * (255 - k) / 255; /* R */
                        cmyk += 4;
                        bgr += 3;
                    }
                    src += srcstride;
                    dst += cbStride;
                }
            }

            free(srcdata);
            return hr;
        }
        return S_OK;

    default:
        FIXME("Unimplemented conversion path!\n");
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_24bppRGB(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_24bppBGR:
    case format_24bppRGB:
        if (prc)
        {
            hr = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (SUCCEEDED(hr) && source_format == format_24bppBGR)
              reverse_bgr8(3, pbBuffer, prc->Width, prc->Height, cbStride);
            return hr;
        }
        return S_OK;
    case format_32bppBGR:
    case format_32bppBGRA:
    case format_32bppPBGRA:
        if (prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            BYTE *dstpixel;
            BYTE tmppixel[3];

            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcpixel=srcrow;
                    dstpixel=dstrow;
                    for (x=0; x<prc->Width; x++) {
                        tmppixel[0]=*srcpixel++; /* blue */
                        tmppixel[1]=*srcpixel++; /* green */
                        tmppixel[2]=*srcpixel++; /* red */
                        srcpixel++; /* alpha */

                        *dstpixel++=tmppixel[2]; /* red */
                        *dstpixel++=tmppixel[1]; /* green */
                        *dstpixel++=tmppixel[0]; /* blue */
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            free(srcdata);

            return res;
        }
        return S_OK;
    default:
        FIXME("Unimplemented conversion path!\n");
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_32bppGrayFloat(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_32bppBGR:
    case format_32bppBGRA:
    case format_32bppPBGRA:
    case format_32bppGrayFloat:
        if (prc)
        {
            hr = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            break;
        }
        return S_OK;

    default:
        hr = copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
        break;
    }

    if (SUCCEEDED(hr) && prc && source_format != format_32bppGrayFloat)
    {
        INT x, y;
        BYTE *p = pbBuffer;

        for (y = 0; y < prc->Height; y++)
        {
            BYTE *bgr = p;
            for (x = 0; x < prc->Width; x++)
            {
                float gray = (bgr[2] * 0.2126f + bgr[1] * 0.7152f + bgr[0] * 0.0722f) / 255.0f;
                *(float *)bgr = gray;
                bgr += 4;
            }
            p += cbStride;
        }
    }
    return hr;
}

static HRESULT copypixels_to_8bppGray(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;
    BYTE *srcdata;
    UINT srcstride, srcdatasize;

    if (source_format == format_8bppGray)
    {
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);

        return S_OK;
    }

    if (source_format == format_32bppGrayFloat)
    {
        hr = S_OK;

        if (prc)
        {
            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            hr = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);
            if (SUCCEEDED(hr))
            {
                INT x, y;
                BYTE *src = srcdata, *dst = pbBuffer;

                for (y=0; y < prc->Height; y++)
                {
                    float *srcpixel = (float*)src;
                    BYTE *dstpixel = dst;

                    for (x=0; x < prc->Width; x++)
                        *dstpixel++ = (BYTE)floorf(to_sRGB_component(*srcpixel++) * 255.0f + 0.51f);

                    src += srcstride;
                    dst += cbStride;
                }
            }

            free(srcdata);
        }

        return hr;
    }

    if (!prc)
        return copypixels_to_24bppBGR(This, NULL, cbStride, cbBufferSize, pbBuffer, source_format);

    srcstride = 3 * prc->Width;
    srcdatasize = srcstride * prc->Height;

    srcdata = malloc(srcdatasize);
    if (!srcdata) return E_OUTOFMEMORY;

    hr = copypixels_to_24bppBGR(This, prc, srcstride, srcdatasize, srcdata, source_format);
    if (SUCCEEDED(hr))
    {
        INT x, y;
        BYTE *src = srcdata, *dst = pbBuffer;

        for (y = 0; y < prc->Height; y++)
        {
            BYTE *bgr = src;

            for (x = 0; x < prc->Width; x++)
            {
                float gray = (bgr[2] * 0.2126f + bgr[1] * 0.7152f + bgr[0] * 0.0722f) / 255.0f;

                gray = to_sRGB_component(gray) * 255.0f;
                dst[x] = (BYTE)floorf(gray + 0.51f);
                bgr += 3;
            }
            src += srcstride;
            dst += cbStride;
        }
    }

    free(srcdata);
    return hr;
}

static UINT rgb_to_palette_index(BYTE bgr[3], WICColor *colors, UINT count)
{
    UINT best_diff, best_index, i;

    best_diff = ~0;
    best_index = 0;

    for (i = 0; i < count; i++)
    {
        BYTE pal_r, pal_g, pal_b;
        UINT diff_r, diff_g, diff_b, diff;

        pal_r = colors[i] >> 16;
        pal_g = colors[i] >> 8;
        pal_b = colors[i];

        diff_r = bgr[2] - pal_r;
        diff_g = bgr[1] - pal_g;
        diff_b = bgr[0] - pal_b;

        diff = diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
        if (diff == 0) return i;

        if (diff < best_diff)
        {
            best_diff = diff;
            best_index = i;
        }
    }

    return best_index;
}

static HRESULT copypixels_to_8bppIndexed(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;
    BYTE *srcdata;
    WICColor colors[256];
    UINT srcstride, srcdatasize, count;

    if (source_format == format_8bppIndexed)
    {
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);

        return S_OK;
    }

    if (!prc)
        return copypixels_to_24bppBGR(This, NULL, cbStride, cbBufferSize, pbBuffer, source_format);

    if (!This->palette) return WINCODEC_ERR_WRONGSTATE;

    hr = IWICPalette_GetColors(This->palette, 256, colors, &count);
    if (hr != S_OK) return hr;

    srcstride = 3 * prc->Width;
    srcdatasize = srcstride * prc->Height;

    srcdata = malloc(srcdatasize);
    if (!srcdata) return E_OUTOFMEMORY;

    hr = copypixels_to_24bppBGR(This, prc, srcstride, srcdatasize, srcdata, source_format);
    if (SUCCEEDED(hr))
    {
        INT x, y;
        BYTE *src = srcdata, *dst = pbBuffer;

        for (y = 0; y < prc->Height; y++)
        {
            BYTE *bgr = src;

            for (x = 0; x < prc->Width; x++)
            {
                dst[x] = rgb_to_palette_index(bgr, colors, count);
                bgr += 3;
            }
            src += srcstride;
            dst += cbStride;
        }
    }

    free(srcdata);
    return hr;
}

static HRESULT copypixels_to_16bppBGRA5551(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_16bppBGRA5551:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    case format_32bppBGRA:
        if(prc)
        {
            HRESULT res;
            INT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const DWORD *srcpixel;
            BYTE *dstrow;
            DWORD srcval = 0;
            WORD *dstpixel;

            int a, r, g, b;

            srcstride = 4 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = malloc(srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);
            if(SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for(y=0; y< prc->Height; y++) {
                    srcpixel = (const DWORD*)srcrow;
                    dstpixel = (WORD *)dstrow;
                    for(x=0; x<prc->Width; x++) {
                        srcval=*srcpixel++;
                        a = (srcval & 0xff000000) >> 24;
                        r = (srcval & 0x00ff0000) >> 16;
                        g = (srcval & 0x0000ff00) >> 8;
                        b = (srcval & 0x000000ff);
                        a = (a >> 7) << 15;
                        r = (r >> 3) << 10;
                        g = (g >> 3) << 5;
                        b = (b >> 3);
                        *dstpixel++ = (a|r|g|b);
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }
            free(srcdata);
        }
        return S_OK;
    default:
        FIXME("Unimplemented conversion path! %d\n", source_format);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_64bppRGBA(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    HRESULT hr;

    switch (source_format)
    {
    case format_64bppRGBA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;

    case format_48bppRGB:
    {
        UINT srcstride, srcdatasize;
        const USHORT *srcpixel;
        const BYTE *srcrow;
        USHORT *dstpixel;
        BYTE *srcdata;
        BYTE *dstrow;
        INT x, y;

        if (!prc)
            return S_OK;

        srcstride = 6 * prc->Width;
        srcdatasize = srcstride * prc->Height;

        srcdata = malloc(srcdatasize);
        if (!srcdata) return E_OUTOFMEMORY;

        hr = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);
        if (SUCCEEDED(hr))
        {
            srcrow = srcdata;
            dstrow = pbBuffer;
            for (y = 0; y < prc->Height; y++)
            {
                srcpixel = (USHORT *)srcrow;
                dstpixel= (USHORT *)dstrow;
                for (x = 0; x < prc->Width; x++)
                {
                    *dstpixel++ = *srcpixel++;
                    *dstpixel++ = *srcpixel++;
                    *dstpixel++ = *srcpixel++;
                    *dstpixel++ = 65535;
                }
                srcrow += srcstride;
                dstrow += cbStride;
            }
        }
        free(srcdata);
        return hr;
    }
    default:
        FIXME("Unimplemented conversion path %d.\n", source_format);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static const struct pixelformatinfo supported_formats[] = {
    {format_1bppIndexed, &GUID_WICPixelFormat1bppIndexed, NULL, TRUE},
    {format_2bppIndexed, &GUID_WICPixelFormat2bppIndexed, NULL, TRUE},
    {format_4bppIndexed, &GUID_WICPixelFormat4bppIndexed, NULL, TRUE},
    {format_8bppIndexed, &GUID_WICPixelFormat8bppIndexed, copypixels_to_8bppIndexed, TRUE},
    {format_BlackWhite, &GUID_WICPixelFormatBlackWhite, NULL},
    {format_2bppGray, &GUID_WICPixelFormat2bppGray, NULL},
    {format_4bppGray, &GUID_WICPixelFormat4bppGray, NULL},
    {format_8bppGray, &GUID_WICPixelFormat8bppGray, copypixels_to_8bppGray},
    {format_16bppGray, &GUID_WICPixelFormat16bppGray, NULL},
    {format_16bppBGR555, &GUID_WICPixelFormat16bppBGR555, NULL},
    {format_16bppBGR565, &GUID_WICPixelFormat16bppBGR565, NULL},
    {format_16bppBGRA5551, &GUID_WICPixelFormat16bppBGRA5551, copypixels_to_16bppBGRA5551},
    {format_24bppBGR, &GUID_WICPixelFormat24bppBGR, copypixels_to_24bppBGR},
    {format_24bppRGB, &GUID_WICPixelFormat24bppRGB, copypixels_to_24bppRGB},
    {format_32bppGrayFloat, &GUID_WICPixelFormat32bppGrayFloat, copypixels_to_32bppGrayFloat},
    {format_32bppBGR, &GUID_WICPixelFormat32bppBGR, copypixels_to_32bppBGR},
    {format_32bppRGB, &GUID_WICPixelFormat32bppRGB, copypixels_to_32bppRGB},
    {format_32bppBGRA, &GUID_WICPixelFormat32bppBGRA, copypixels_to_32bppBGRA},
    {format_32bppRGBA, &GUID_WICPixelFormat32bppRGBA, copypixels_to_32bppRGBA},
    {format_32bppPBGRA, &GUID_WICPixelFormat32bppPBGRA, copypixels_to_32bppPBGRA},
    {format_32bppPRGBA, &GUID_WICPixelFormat32bppPRGBA, copypixels_to_32bppPRGBA},
    {format_48bppRGB, &GUID_WICPixelFormat48bppRGB, NULL},
    {format_64bppRGBA, &GUID_WICPixelFormat64bppRGBA, copypixels_to_64bppRGBA},
    {format_32bppCMYK, &GUID_WICPixelFormat32bppCMYK, NULL},
    {0}
};

static const struct pixelformatinfo *get_formatinfo(const WICPixelFormatGUID *format)
{
    UINT i;

    for (i=0; supported_formats[i].guid; i++)
        if (IsEqualGUID(supported_formats[i].guid, format)) return &supported_formats[i];

    return NULL;
}

static HRESULT WINAPI FormatConverter_QueryInterface(IWICFormatConverter *iface, REFIID iid,
    void **ppv)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICFormatConverter, iid))
    {
        *ppv = &This->IWICFormatConverter_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI FormatConverter_AddRef(IWICFormatConverter *iface)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverter_Release(IWICFormatConverter *iface)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->source) IWICBitmapSource_Release(This->source);
        if (This->palette) IWICPalette_Release(This->palette);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI FormatConverter_GetSize(IWICFormatConverter *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);

    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (This->source)
        return IWICBitmapSource_GetSize(This->source, puiWidth, puiHeight);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_GetPixelFormat(IWICFormatConverter *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);

    TRACE("(%p,%p)\n", iface, pPixelFormat);

    if (This->source)
        memcpy(pPixelFormat, This->dst_format->guid, sizeof(GUID));
    else
        return WINCODEC_ERR_NOTINITIALIZED;

    return S_OK;
}

static HRESULT WINAPI FormatConverter_GetResolution(IWICFormatConverter *iface,
    double *pDpiX, double *pDpiY)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);

    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    if (This->source)
        return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_CopyPalette(IWICFormatConverter *iface,
    IWICPalette *palette)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette) return E_INVALIDARG;
    if (!This->source) return WINCODEC_ERR_WRONGSTATE;

    if (!This->palette)
    {
        if (This->dst_format->is_indexed_format) return WINCODEC_ERR_WRONGSTATE;
        return IWICBitmapSource_CopyPalette(This->source, palette);
    }

    return IWICPalette_InitializeFromPalette(palette, This->palette);
}

static HRESULT WINAPI FormatConverter_CopyPixels(IWICFormatConverter *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    WICRect rc;
    HRESULT hr;
    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    if (This->source)
    {
        if (!prc)
        {
            UINT width, height;
            hr = IWICBitmapSource_GetSize(This->source, &width, &height);
            if (FAILED(hr)) return hr;
            rc.X = 0;
            rc.Y = 0;
            rc.Width = width;
            rc.Height = height;
            prc = &rc;
        }

        return This->dst_format->copy_function(This, prc, cbStride, cbBufferSize,
            pbBuffer, This->src_format->format);
    }
    else
        return WINCODEC_ERR_WRONGSTATE;
}

static HRESULT WINAPI FormatConverter_Initialize(IWICFormatConverter *iface,
    IWICBitmapSource *source, REFWICPixelFormatGUID dstFormat, WICBitmapDitherType dither,
    IWICPalette *palette, double alpha_threshold, WICBitmapPaletteType palette_type)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    const struct pixelformatinfo *srcinfo, *dstinfo;
    GUID srcFormat;
    HRESULT res;

    TRACE("(%p,%p,%s,%u,%p,%0.3f,%u)\n", iface, source, debugstr_guid(dstFormat),
        dither, palette, alpha_threshold, palette_type);

    dstinfo = get_formatinfo(dstFormat);
    if (!dstinfo)
    {
        FIXME("Unsupported destination format %s\n", debugstr_guid(dstFormat));
        return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }

    if (!palette)
    {
        UINT bpp;
        res = get_pixelformat_bpp(dstFormat, &bpp);
        if (res != S_OK) return res;

        res = PaletteImpl_Create(&palette);
        if (res != S_OK) return res;

        switch (palette_type)
        {
        case WICBitmapPaletteTypeCustom:
            IWICPalette_Release(palette);
            palette = NULL;

            /* Indexed types require a palette */
            if (dstinfo->is_indexed_format)
                return E_INVALIDARG;
            break;

        case WICBitmapPaletteTypeMedianCut:
        {
            if (dstinfo->is_indexed_format)
                res = IWICPalette_InitializeFromBitmap(palette, source, 1 << bpp, FALSE);
            break;
        }

        default:
            if (dstinfo->is_indexed_format)
                res = IWICPalette_InitializePredefined(palette, palette_type, FALSE);
            break;
        }

        if (res != S_OK)
        {
            IWICPalette_Release(palette);
            return res;
        }
    }
    else
        IWICPalette_AddRef(palette);

    EnterCriticalSection(&This->lock);

    if (This->source)
    {
        res = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    res = IWICBitmapSource_GetPixelFormat(source, &srcFormat);
    if (FAILED(res)) goto end;

    srcinfo = get_formatinfo(&srcFormat);
    if (!srcinfo)
    {
        res = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        FIXME("Unsupported source format %s\n", debugstr_guid(&srcFormat));
        goto end;
    }

    if (dstinfo->copy_function)
    {
        IWICBitmapSource_AddRef(source);
        This->src_format = srcinfo;
        This->dst_format = dstinfo;
        This->dither = dither;
        This->alpha_threshold = alpha_threshold;
        This->palette = palette;
        This->source = source;
    }
    else
    {
        FIXME("Unsupported conversion %s -> %s\n", debugstr_guid(&srcFormat), debugstr_guid(dstFormat));
        res = WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

end:

    LeaveCriticalSection(&This->lock);

    if (res != S_OK && palette)
        IWICPalette_Release(palette);

    return res;
}

static HRESULT WINAPI FormatConverter_CanConvert(IWICFormatConverter *iface,
    REFWICPixelFormatGUID srcPixelFormat, REFWICPixelFormatGUID dstPixelFormat,
    BOOL *pfCanConvert)
{
    FormatConverter *This = impl_from_IWICFormatConverter(iface);
    const struct pixelformatinfo *srcinfo, *dstinfo;

    TRACE("(%p,%s,%s,%p)\n", iface, debugstr_guid(srcPixelFormat),
        debugstr_guid(dstPixelFormat), pfCanConvert);

    srcinfo = get_formatinfo(srcPixelFormat);
    if (!srcinfo)
    {
        FIXME("Unsupported source format %s\n", debugstr_guid(srcPixelFormat));
        return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }

    dstinfo = get_formatinfo(dstPixelFormat);
    if (!dstinfo)
    {
        FIXME("Unsupported destination format %s\n", debugstr_guid(dstPixelFormat));
        return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }

    if (dstinfo->copy_function &&
        SUCCEEDED(dstinfo->copy_function(This, NULL, 0, 0, NULL, dstinfo->format)))
        *pfCanConvert = TRUE;
    else
    {
        FIXME("Unsupported conversion %s -> %s\n", debugstr_guid(srcPixelFormat), debugstr_guid(dstPixelFormat));
        *pfCanConvert = FALSE;
    }

    return S_OK;
}

static const IWICFormatConverterVtbl FormatConverter_Vtbl = {
    FormatConverter_QueryInterface,
    FormatConverter_AddRef,
    FormatConverter_Release,
    FormatConverter_GetSize,
    FormatConverter_GetPixelFormat,
    FormatConverter_GetResolution,
    FormatConverter_CopyPalette,
    FormatConverter_CopyPixels,
    FormatConverter_Initialize,
    FormatConverter_CanConvert
};

HRESULT FormatConverter_CreateInstance(REFIID iid, void** ppv)
{
    FormatConverter *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(FormatConverter));
    if (!This) return E_OUTOFMEMORY;

    This->IWICFormatConverter_iface.lpVtbl = &FormatConverter_Vtbl;
    This->ref = 1;
    This->source = NULL;
    This->palette = NULL;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FormatConverter.lock");

    ret = IWICFormatConverter_QueryInterface(&This->IWICFormatConverter_iface, iid, ppv);
    IWICFormatConverter_Release(&This->IWICFormatConverter_iface);

    return ret;
}
