/*
 * Copyright 2009 Vincent Povirk
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

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
    format_24bppBGR,
    format_32bppBGR,
    format_32bppBGRA,
    format_48bppRGB,
    format_64bppRGBA,
};

typedef HRESULT (*copyfunc)(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format);

struct pixelformatinfo {
    enum pixelformat format;
    const WICPixelFormatGUID *guid;
    copyfunc copy_function;
};

typedef struct FormatConverter {
    const IWICFormatConverterVtbl *lpVtbl;
    LONG ref;
    IWICBitmapSource *source;
    const struct pixelformatinfo *dst_format, *src_format;
    WICBitmapDitherType dither;
    double alpha_threshold;
    WICBitmapPaletteType palette_type;
    CRITICAL_SECTION lock; /* must be held when initialized */
} FormatConverter;

static void make_grayscale_palette(WICColor *colors, UINT num_colors)
{
    int i, v;
    for (i=0; i<num_colors; i++)
    {
        v = i * 255 / (num_colors-1);
        colors[i] = 0xff000000 | v<<16 | v<<8 | v;
    }
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
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[2];
            IWICPalette *palette;
            UINT actualcolors;

            if (source_format == format_1bppIndexed)
            {
                res = PaletteImpl_Create(&palette);
                if (FAILED(res)) return res;

                res = IWICBitmapSource_CopyPalette(This->source, palette);
                if (SUCCEEDED(res))
                    res = IWICPalette_GetColors(palette, 2, colors, &actualcolors);

                IWICPalette_Release(palette);

                if (FAILED(res)) return res;
            }
            else
            {
                colors[0] = 0xff000000;
                colors[1] = 0xffffffff;
            }

            srcstride = (prc->Width+7)/8;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_2bppIndexed:
    case format_2bppGray:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[4];
            IWICPalette *palette;
            UINT actualcolors;

            if (source_format == format_2bppIndexed)
            {
                res = PaletteImpl_Create(&palette);
                if (FAILED(res)) return res;

                res = IWICBitmapSource_CopyPalette(This->source, palette);
                if (SUCCEEDED(res))
                    res = IWICPalette_GetColors(palette, 4, colors, &actualcolors);

                IWICPalette_Release(palette);

                if (FAILED(res)) return res;
            }
            else
                make_grayscale_palette(colors, 4);

            srcstride = (prc->Width+3)/4;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x+=4) {
                        BYTE srcval;
                        srcval=*srcbyte++;
                        *dstpixel++ = colors[srcval>>6];
                        if (x+1 < prc->Width) *dstpixel++ = colors[srcval>>4&0x3];
                        if (x+2 < prc->Width) *dstpixel++ = colors[srcval>>2&0x3];
                        if (x+1 < prc->Width) *dstpixel++ = colors[srcval&0x3];
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_4bppIndexed:
    case format_4bppGray:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;
            WICColor colors[16];
            IWICPalette *palette;
            UINT actualcolors;

            if (source_format == format_4bppIndexed)
            {
                res = PaletteImpl_Create(&palette);
                if (FAILED(res)) return res;

                res = IWICBitmapSource_CopyPalette(This->source, palette);
                if (SUCCEEDED(res))
                    res = IWICPalette_GetColors(palette, 16, colors, &actualcolors);

                IWICPalette_Release(palette);

                if (FAILED(res)) return res;
            }
            else
                make_grayscale_palette(colors, 16);

            srcstride = (prc->Width+1)/2;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_8bppGray:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_8bppIndexed:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
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

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++)
                        *dstpixel++ = colors[*srcbyte++];
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_16bppGray:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcbyte;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = prc->Width * 2;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
            if (!srcdata) return E_OUTOFMEMORY;

            res = IWICBitmapSource_CopyPixels(This->source, prc, srcstride, srcdatasize, srcdata);

            if (SUCCEEDED(res))
            {
                srcrow = srcdata;
                dstrow = pbBuffer;
                for (y=0; y<prc->Height; y++) {
                    srcbyte=(const BYTE*)srcrow;
                    dstpixel=(DWORD*)dstrow;
                    for (x=0; x<prc->Width; x++)
                    {
                        *dstpixel++ = 0xff000000|(*srcbyte<<16)|(*srcbyte<<8)|*srcbyte;
                        srcbyte+=2;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_16bppBGR555:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const WORD *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 2 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_16bppBGR565:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const WORD *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 2 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_24bppBGR:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            BYTE *dstpixel;

            srcstride = 3 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
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

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_32bppBGR:
        if (prc)
        {
            HRESULT res;
            UINT x, y;

            res = IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
            if (FAILED(res)) return res;

            /* set all alpha values to 255 */
            for (y=0; y<prc->Height; y++)
                for (x=0; x<prc->Width; x++)
                    pbBuffer[cbStride*y+4*x+3] = 0xff;
        }
        return S_OK;
    case format_32bppBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    case format_48bppRGB:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 6 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
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
                        red = *srcpixel++; srcpixel++;
                        green = *srcpixel++; srcpixel++;
                        blue = *srcpixel++; srcpixel++;
                        *dstpixel++=0xff000000|red<<16|green<<8|blue;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    case format_64bppRGBA:
        if (prc)
        {
            HRESULT res;
            UINT x, y;
            BYTE *srcdata;
            UINT srcstride, srcdatasize;
            const BYTE *srcrow;
            const BYTE *srcpixel;
            BYTE *dstrow;
            DWORD *dstpixel;

            srcstride = 8 * prc->Width;
            srcdatasize = srcstride * prc->Height;

            srcdata = HeapAlloc(GetProcessHeap(), 0, srcdatasize);
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
                        red = *srcpixel++; srcpixel++;
                        green = *srcpixel++; srcpixel++;
                        blue = *srcpixel++; srcpixel++;
                        alpha = *srcpixel++; srcpixel++;
                        *dstpixel++=alpha<<24|red<<16|green<<8|blue;
                    }
                    srcrow += srcstride;
                    dstrow += cbStride;
                }
            }

            HeapFree(GetProcessHeap(), 0, srcdata);

            return res;
        }
        return S_OK;
    default:
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
}

static HRESULT copypixels_to_32bppBGR(struct FormatConverter *This, const WICRect *prc,
    UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer, enum pixelformat source_format)
{
    switch (source_format)
    {
    case format_32bppBGR:
    case format_32bppBGRA:
        if (prc)
            return IWICBitmapSource_CopyPixels(This->source, prc, cbStride, cbBufferSize, pbBuffer);
        return S_OK;
    default:
        return copypixels_to_32bppBGRA(This, prc, cbStride, cbBufferSize, pbBuffer, source_format);
    }
}

static const struct pixelformatinfo supported_formats[] = {
    {format_1bppIndexed, &GUID_WICPixelFormat1bppIndexed, NULL},
    {format_2bppIndexed, &GUID_WICPixelFormat2bppIndexed, NULL},
    {format_4bppIndexed, &GUID_WICPixelFormat4bppIndexed, NULL},
    {format_8bppIndexed, &GUID_WICPixelFormat8bppIndexed, NULL},
    {format_BlackWhite, &GUID_WICPixelFormatBlackWhite, NULL},
    {format_2bppGray, &GUID_WICPixelFormat2bppGray, NULL},
    {format_4bppGray, &GUID_WICPixelFormat4bppGray, NULL},
    {format_8bppGray, &GUID_WICPixelFormat8bppGray, NULL},
    {format_16bppGray, &GUID_WICPixelFormat16bppGray, NULL},
    {format_16bppBGR555, &GUID_WICPixelFormat16bppBGR555, NULL},
    {format_16bppBGR565, &GUID_WICPixelFormat16bppBGR565, NULL},
    {format_24bppBGR, &GUID_WICPixelFormat24bppBGR, NULL},
    {format_32bppBGR, &GUID_WICPixelFormat32bppBGR, copypixels_to_32bppBGR},
    {format_32bppBGRA, &GUID_WICPixelFormat32bppBGRA, copypixels_to_32bppBGRA},
    {format_48bppRGB, &GUID_WICPixelFormat48bppRGB, NULL},
    {format_64bppRGBA, &GUID_WICPixelFormat64bppRGBA, NULL},
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
    FormatConverter *This = (FormatConverter*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICFormatConverter, iid))
    {
        *ppv = This;
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
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverter_Release(IWICFormatConverter *iface)
{
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->source) IWICBitmapSource_Release(This->source);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI FormatConverter_GetSize(IWICFormatConverter *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (This->source)
        return IWICBitmapSource_GetSize(This->source, puiWidth, puiHeight);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_GetPixelFormat(IWICFormatConverter *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p): stub\n", iface, pPixelFormat);

    if (This->source)
        memcpy(pPixelFormat, This->dst_format->guid, sizeof(GUID));
    else
        return WINCODEC_ERR_NOTINITIALIZED;

    return S_OK;
}

static HRESULT WINAPI FormatConverter_GetResolution(IWICFormatConverter *iface,
    double *pDpiX, double *pDpiY)
{
    FormatConverter *This = (FormatConverter*)iface;

    TRACE("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);

    if (This->source)
        return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_CopyPalette(IWICFormatConverter *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_CopyPixels(IWICFormatConverter *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FormatConverter *This = (FormatConverter*)iface;
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (This->source)
        return This->dst_format->copy_function(This, prc, cbStride, cbBufferSize,
            pbBuffer, This->src_format->format);
    else
        return WINCODEC_ERR_NOTINITIALIZED;
}

static HRESULT WINAPI FormatConverter_Initialize(IWICFormatConverter *iface,
    IWICBitmapSource *pISource, REFWICPixelFormatGUID dstFormat, WICBitmapDitherType dither,
    IWICPalette *pIPalette, double alphaThresholdPercent, WICBitmapPaletteType paletteTranslate)
{
    FormatConverter *This = (FormatConverter*)iface;
    const struct pixelformatinfo *srcinfo, *dstinfo;
    static INT fixme=0;
    GUID srcFormat;
    HRESULT res=S_OK;

    TRACE("(%p,%p,%s,%u,%p,%0.1f,%u)\n", iface, pISource, debugstr_guid(dstFormat),
        dither, pIPalette, alphaThresholdPercent, paletteTranslate);

    if (pIPalette && !fixme++) FIXME("ignoring palette\n");

    EnterCriticalSection(&This->lock);

    if (This->source)
    {
        res = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    res = IWICBitmapSource_GetPixelFormat(pISource, &srcFormat);
    if (FAILED(res)) goto end;

    srcinfo = get_formatinfo(&srcFormat);
    if (!srcinfo)
    {
        res = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        goto end;
    }

    dstinfo = get_formatinfo(dstFormat);
    if (!dstinfo)
    {
        res = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        goto end;
    }

    if (dstinfo->copy_function)
    {
        IWICBitmapSource_AddRef(pISource);
        This->src_format = srcinfo;
        This->dst_format = dstinfo;
        This->dither = dither;
        This->alpha_threshold = alphaThresholdPercent;
        This->palette_type = paletteTranslate;
        This->source = pISource;
    }
    else
        res = WINCODEC_ERR_UNSUPPORTEDOPERATION;

end:

    LeaveCriticalSection(&This->lock);

    return res;
}

static HRESULT WINAPI FormatConverter_CanConvert(IWICFormatConverter *iface,
    REFWICPixelFormatGUID srcPixelFormat, REFWICPixelFormatGUID dstPixelFormat,
    BOOL *pfCanConvert)
{
    FormatConverter *This = (FormatConverter*)iface;
    const struct pixelformatinfo *srcinfo, *dstinfo;

    TRACE("(%p,%s,%s,%p)\n", iface, debugstr_guid(srcPixelFormat),
        debugstr_guid(dstPixelFormat), pfCanConvert);

    srcinfo = get_formatinfo(srcPixelFormat);
    if (!srcinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    dstinfo = get_formatinfo(dstPixelFormat);
    if (!dstinfo) return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;

    if (dstinfo->copy_function &&
        SUCCEEDED(dstinfo->copy_function(This, NULL, 0, 0, NULL, dstinfo->format)))
        *pfCanConvert = TRUE;
    else
        *pfCanConvert = FALSE;

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

HRESULT FormatConverter_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    FormatConverter *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FormatConverter));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &FormatConverter_Vtbl;
    This->ref = 1;
    This->source = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FormatConverter.lock");

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
