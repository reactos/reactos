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
#include <stdio.h>
#include <math.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "wine/test.h"

static IWICImagingFactory *factory;

typedef struct bitmap_data {
    const WICPixelFormatGUID *format;
    UINT bpp;
    const BYTE *bits;
    UINT width;
    UINT height;
    double xres;
    double yres;
    const struct bitmap_data *alt_data;
} bitmap_data;

typedef struct BitmapTestSrc {
    IWICBitmapSource IWICBitmapSource_iface;
    LONG ref;
    const bitmap_data *data;
} BitmapTestSrc;

extern HRESULT STDMETHODCALLTYPE IWICBitmapFrameEncode_WriteSource_Proxy(IWICBitmapFrameEncode* This,
    IWICBitmapSource *pIBitmapSource, WICRect *prc);

static BOOL near_equal(float a, float b)
{
    return fabsf(a - b) < 0.001;
}

static inline BitmapTestSrc *impl_from_IWICBitmapSource(IWICBitmapSource *iface)
{
    return CONTAINING_RECORD(iface, BitmapTestSrc, IWICBitmapSource_iface);
}

static HRESULT WINAPI BitmapTestSrc_QueryInterface(IWICBitmapSource *iface, REFIID iid,
    void **ppv)
{
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapTestSrc_AddRef(IWICBitmapSource *iface)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI BitmapTestSrc_Release(IWICBitmapSource *iface)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    return ref;
}

static HRESULT WINAPI BitmapTestSrc_GetSize(IWICBitmapSource *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    *puiWidth = This->data->width;
    *puiHeight = This->data->height;
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_GetPixelFormat(IWICBitmapSource *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    memcpy(pPixelFormat, This->data->format, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_GetResolution(IWICBitmapSource *iface,
    double *pDpiX, double *pDpiY)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    *pDpiX = This->data->xres;
    *pDpiY = This->data->yres;
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_CopyPalette(IWICBitmapSource *iface,
    IWICPalette *palette)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);

    if (IsEqualGUID(This->data->format, &GUID_WICPixelFormat1bppIndexed) ||
        IsEqualGUID(This->data->format, &GUID_WICPixelFormat2bppIndexed) ||
        IsEqualGUID(This->data->format, &GUID_WICPixelFormat4bppIndexed) ||
        IsEqualGUID(This->data->format, &GUID_WICPixelFormat8bppIndexed))
    {
        WICColor colors[8];

        colors[0] = 0xff0000ff;
        colors[1] = 0xff00ff00;
        colors[2] = 0xffff0000;
        colors[3] = 0xff000000;
        colors[4] = 0xffffff00;
        colors[5] = 0xffff00ff;
        colors[6] = 0xff00ffff;
        colors[7] = 0xffffffff;
        return IWICPalette_InitializeCustom(palette, colors, 8);
    }

    /* unique error marker */
    return 0xdeadbeef;
}

static HRESULT WINAPI BitmapTestSrc_CopyPixels(IWICBitmapSource *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    UINT bytesperrow;
    UINT srcstride;
    UINT row_offset;
    WICRect rc;

    if (!prc)
    {
        rc.X = 0;
        rc.Y = 0;
        rc.Width = This->data->width;
        rc.Height = This->data->height;
        prc = &rc;
    }
    else
    {
        if (prc->X < 0 || prc->Y < 0 || prc->X+prc->Width > This->data->width || prc->Y+prc->Height > This->data->height)
            return E_INVALIDARG;
    }

    bytesperrow = ((This->data->bpp * prc->Width)+7)/8;
    srcstride = ((This->data->bpp * This->data->width)+7)/8;

    if (cbStride < bytesperrow)
        return E_INVALIDARG;

    if ((cbStride * prc->Height) > cbBufferSize)
        return E_INVALIDARG;

    row_offset = prc->X * This->data->bpp;

    if (row_offset % 8 == 0)
    {
        UINT row;
        const BYTE *src;
        BYTE *dst;

        src = This->data->bits + (row_offset / 8) + prc->Y * srcstride;
        dst = pbBuffer;
        for (row=0; row < prc->Height; row++)
        {
            memcpy(dst, src, bytesperrow);
            src += srcstride;
            dst += cbStride;
        }
        return S_OK;
    }
    else
    {
        ok(0, "bitmap %p was asked to copy pixels not aligned on a byte boundary\n", iface);
        return E_FAIL;
    }
}

static const IWICBitmapSourceVtbl BitmapTestSrc_Vtbl = {
    BitmapTestSrc_QueryInterface,
    BitmapTestSrc_AddRef,
    BitmapTestSrc_Release,
    BitmapTestSrc_GetSize,
    BitmapTestSrc_GetPixelFormat,
    BitmapTestSrc_GetResolution,
    BitmapTestSrc_CopyPalette,
    BitmapTestSrc_CopyPixels
};

static void CreateTestBitmap(const bitmap_data *data, BitmapTestSrc **This)
{
    *This = HeapAlloc(GetProcessHeap(), 0, sizeof(**This));

    if (*This)
    {
        (*This)->IWICBitmapSource_iface.lpVtbl = &BitmapTestSrc_Vtbl;
        (*This)->ref = 1;
        (*This)->data = data;
    }
}

static void DeleteTestBitmap(BitmapTestSrc *This)
{
    ok(This->IWICBitmapSource_iface.lpVtbl == &BitmapTestSrc_Vtbl, "test bitmap %p deleted with incorrect vtable\n", This);
    ok(This->ref == 1, "test bitmap %p deleted with %li references instead of 1\n", This, This->ref);
    HeapFree(GetProcessHeap(), 0, This);
}

static BOOL compare_bits(const struct bitmap_data *expect, UINT buffersize, const BYTE *converted_bits)
{
    BOOL equal;

    if (IsEqualGUID(expect->format, &GUID_WICPixelFormat32bppBGR))
    {
        /* ignore the padding byte when comparing data */
        UINT i;
        const DWORD *a=(const DWORD*)expect->bits, *b=(const DWORD*)converted_bits;
        equal=TRUE;
        for (i=0; i<(buffersize/4); i++)
            if ((a[i]&0xffffff) != (b[i]&0xffffff))
            {
                equal = FALSE;
                break;
            }
    }
    else if (IsEqualGUID(expect->format, &GUID_WICPixelFormat32bppGrayFloat))
    {
        UINT i;
        const float *a=(const float*)expect->bits, *b=(const float*)converted_bits;
        equal=TRUE;
        for (i=0; i<(buffersize/4); i++)
            if (!near_equal(a[i], b[i]))
            {
                equal = FALSE;
                break;
            }
    }
    else if (IsEqualGUID(expect->format, &GUID_WICPixelFormatBlackWhite) ||
             IsEqualGUID(expect->format, &GUID_WICPixelFormat1bppIndexed))
    {
        UINT i;
        const BYTE *a=(const BYTE*)expect->bits, *b=(const BYTE*)converted_bits;
        equal=TRUE;
        for (i=0; i<buffersize; i++)
            if (a[i] != b[i] && b[i] != 0xff /* BMP encoder B&W */)
            {
                equal = FALSE;
                break;
            }
    }
    else if (IsEqualGUID(expect->format, &GUID_WICPixelFormat2bppIndexed) ||
             IsEqualGUID(expect->format, &GUID_WICPixelFormat4bppIndexed) ||
             IsEqualGUID(expect->format, &GUID_WICPixelFormat8bppIndexed))
    {
        UINT i;
        const BYTE *a=(const BYTE*)expect->bits, *b=(const BYTE*)converted_bits;
        equal=TRUE;

        for (i=0; i<buffersize; i++)
            if (a[i] != b[i])
            {
                equal = FALSE;
                break;
            }
    }
    else
        equal = (memcmp(expect->bits, converted_bits, buffersize) == 0);

    if (!equal && expect->alt_data)
        equal = compare_bits(expect->alt_data, buffersize, converted_bits);

    if (!equal && winetest_debug > 1)
    {
        UINT i, bps;
        bps = expect->bpp / 8;
        if (!bps) bps = buffersize;
        printf("converted_bits (%u bytes):\n    ", buffersize);
        for (i = 0; i < buffersize; i++)
        {
            printf("%u,", converted_bits[i]);
            if (!((i + 1) % 32)) printf("\n    ");
            else if (!((i+1) % bps)) printf(" ");
        }
        printf("\n");
    }

    return equal;
}

static BOOL is_indexed_format(const GUID *format)
{
    if (IsEqualGUID(format, &GUID_WICPixelFormat1bppIndexed) ||
        IsEqualGUID(format, &GUID_WICPixelFormat2bppIndexed) ||
        IsEqualGUID(format, &GUID_WICPixelFormat4bppIndexed) ||
        IsEqualGUID(format, &GUID_WICPixelFormat8bppIndexed))
        return TRUE;

    return FALSE;
}

static void compare_bitmap_data(const struct bitmap_data *src, const struct bitmap_data *expect,
                                IWICBitmapSource *source, const char *name)
{
    BYTE *converted_bits;
    UINT width, height;
    double xres, yres;
    WICRect prc;
    UINT stride, buffersize;
    GUID dst_pixelformat;
    HRESULT hr;

    hr = IWICBitmapSource_GetSize(source, &width, &height);
    ok(SUCCEEDED(hr), "GetSize(%s) failed, hr=%lx\n", name, hr);
    ok(width == expect->width, "expecting %u, got %u (%s)\n", expect->width, width, name);
    ok(height == expect->height, "expecting %u, got %u (%s)\n", expect->height, height, name);

    hr = IWICBitmapSource_GetResolution(source, &xres, &yres);
    ok(SUCCEEDED(hr), "GetResolution(%s) failed, hr=%lx\n", name, hr);
    ok(fabs(xres - expect->xres) < 0.02, "expecting %0.2f, got %0.2f (%s)\n", expect->xres, xres, name);
    ok(fabs(yres - expect->yres) < 0.02, "expecting %0.2f, got %0.2f (%s)\n", expect->yres, yres, name);

    hr = IWICBitmapSource_GetPixelFormat(source, &dst_pixelformat);
    ok(SUCCEEDED(hr), "GetPixelFormat(%s) failed, hr=%lx\n", name, hr);
    ok(IsEqualGUID(&dst_pixelformat, expect->format), "got unexpected pixel format %s (%s)\n", wine_dbgstr_guid(&dst_pixelformat), name);

    prc.X = 0;
    prc.Y = 0;
    prc.Width = expect->width;
    prc.Height = expect->height;

    stride = (expect->bpp * expect->width + 7) / 8;
    buffersize = stride * expect->height;

    converted_bits = HeapAlloc(GetProcessHeap(), 0, buffersize);
    memset(converted_bits, 0xaa, buffersize);
    hr = IWICBitmapSource_CopyPixels(source, &prc, stride, buffersize, converted_bits);
    ok(SUCCEEDED(hr), "CopyPixels(%s) failed, hr=%lx\n", name, hr);

    /* The result of conversion of color to indexed formats depends on
     * optimized palette generation implementation. We either need to
     * assign our own palette, or just skip the comparison.
     */
    if (!(!is_indexed_format(src->format) && is_indexed_format(expect->format)))
        ok(compare_bits(expect, buffersize, converted_bits), "unexpected pixel data (%s)\n", name);

    /* Test with NULL rectangle - should copy the whole bitmap */
    memset(converted_bits, 0xaa, buffersize);
    hr = IWICBitmapSource_CopyPixels(source, NULL, stride, buffersize, converted_bits);
    ok(SUCCEEDED(hr), "CopyPixels(%s,rc=NULL) failed, hr=%lx\n", name, hr);
    /* see comment above */
    if (!(!is_indexed_format(src->format) && is_indexed_format(expect->format)))
        ok(compare_bits(expect, buffersize, converted_bits), "unexpected pixel data (%s)\n", name);

    HeapFree(GetProcessHeap(), 0, converted_bits);
}

/* some encoders (like BMP) require data to be 4-bytes aligned */
static const BYTE bits_1bpp[] = {
    0x55,0x55,0x55,0x55,  /*01010101*/
    0xaa,0xaa,0xaa,0xaa}; /*10101010*/
static const struct bitmap_data testdata_BlackWhite = {
    &GUID_WICPixelFormatBlackWhite, 1, bits_1bpp, 32, 2, 96.0, 96.0};
static const struct bitmap_data testdata_1bppIndexed = {
    &GUID_WICPixelFormat1bppIndexed, 1, bits_1bpp, 32, 2, 96.0, 96.0};

/* some encoders (like BMP) require data to be 4-bytes aligned */
static const BYTE bits_2bpp[] = {
    0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,
    0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24};
static const struct bitmap_data testdata_2bppIndexed = {
    &GUID_WICPixelFormat2bppIndexed, 2, bits_2bpp, 32, 2, 96.0, 96.0};

/* some encoders (like BMP) require data to be 4-bytes aligned */
static const BYTE bits_4bpp[] = {
    0x34,0x43,0x34,0x43,0x34,0x43,0x34,0x43,0x34,0x43,0x34,0x43,0x34,0x43,0x34,0x43,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44};

static const struct bitmap_data testdata_4bppIndexed = {
    &GUID_WICPixelFormat4bppIndexed, 4, bits_4bpp, 32, 2, 96.0, 96.0};

static const BYTE bits_8bpp_BW[] = {
    0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
static const struct bitmap_data testdata_8bppIndexed_BW = {
    &GUID_WICPixelFormat8bppIndexed, 8, bits_8bpp_BW, 32, 2, 96.0, 96.0};

static const BYTE bits_8bpp_4colors[] = {
    0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,
    3,2,1,3,3,2,1,3,3,2,1,3,3,2,1,3,3,2,1,3,3,2,1,3,3,2,1,3,3,2,1,3};
static const struct bitmap_data testdata_8bppIndexed_4colors = {
    &GUID_WICPixelFormat8bppIndexed, 8, bits_8bpp_4colors, 32, 2, 96.0, 96.0};

static const BYTE bits_8bpp[] = {
    0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static const struct bitmap_data testdata_8bppIndexed = {
    &GUID_WICPixelFormat8bppIndexed, 8, bits_8bpp, 32, 2, 96.0, 96.0};

static const BYTE bits_24bppBGR[] = {
    255,0,0, 0,255,0, 0,0,255, 0,0,0, 255,0,0, 0,255,0, 0,0,255, 0,0,0,
    255,0,0, 0,255,0, 0,0,255, 0,0,0, 255,0,0, 0,255,0, 0,0,255, 0,0,0,
    255,0,0, 0,255,0, 0,0,255, 0,0,0, 255,0,0, 0,255,0, 0,0,255, 0,0,0,
    255,0,0, 0,255,0, 0,0,255, 0,0,0, 255,0,0, 0,255,0, 0,0,255, 0,0,0,
    0,255,255, 255,0,255, 255,255,0, 255,255,255, 0,255,255, 255,0,255, 255,255,0, 255,255,255,
    0,255,255, 255,0,255, 255,255,0, 255,255,255, 0,255,255, 255,0,255, 255,255,0, 255,255,255,
    0,255,255, 255,0,255, 255,255,0, 255,255,255, 0,255,255, 255,0,255, 255,255,0, 255,255,255,
    0,255,255, 255,0,255, 255,255,0, 255,255,255, 0,255,255, 255,0,255, 255,255,0, 255,255,255};
static const struct bitmap_data testdata_24bppBGR = {
    &GUID_WICPixelFormat24bppBGR, 24, bits_24bppBGR, 32, 2, 96.0, 96.0};

static const BYTE bits_24bppRGB[] = {
    0,0,255, 0,255,0, 255,0,0, 0,0,0, 0,0,255, 0,255,0, 255,0,0, 0,0,0,
    0,0,255, 0,255,0, 255,0,0, 0,0,0, 0,0,255, 0,255,0, 255,0,0, 0,0,0,
    0,0,255, 0,255,0, 255,0,0, 0,0,0, 0,0,255, 0,255,0, 255,0,0, 0,0,0,
    0,0,255, 0,255,0, 255,0,0, 0,0,0, 0,0,255, 0,255,0, 255,0,0, 0,0,0,
    255,255,0, 255,0,255, 0,255,255, 255,255,255, 255,255,0, 255,0,255, 0,255,255, 255,255,255,
    255,255,0, 255,0,255, 0,255,255, 255,255,255, 255,255,0, 255,0,255, 0,255,255, 255,255,255,
    255,255,0, 255,0,255, 0,255,255, 255,255,255, 255,255,0, 255,0,255, 0,255,255, 255,255,255,
    255,255,0, 255,0,255, 0,255,255, 255,255,255, 255,255,0, 255,0,255, 0,255,255, 255,255,255 };
static const struct bitmap_data testdata_24bppRGB = {
    &GUID_WICPixelFormat24bppRGB, 24, bits_24bppRGB, 32, 2, 96.0, 96.0};

static const BYTE bits_32bppBGR[] = {
    255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80, 255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80,
    255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80, 255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80,
    255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80, 255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80,
    255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80, 255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80,
    0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80, 0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80,
    0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80, 0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80,
    0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80, 0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80,
    0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80, 0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80,
    3,3,3,80, 6,6,6,80, 12,12,12,80, 15,15,15,80, 19,19,19,80, 22,22,22,80, 28,28,28,80, 31,31,31,80,
    35,35,35,80, 38,38,38,80, 41,41,41,80, 47,47,47,80, 47,47,47,80, 54,54,54,80, 57,57,57,80, 63,63,63,80,
    66,66,66,80, 70,70,70,80, 73,73,73,80, 79,79,79,80, 82,82,82,80, 86,86,86,80, 89,89,89,80, 95,95,95,80,
    98,98,98,80, 98,98,98,80, 105,105,105,80, 108,108,108,80, 114,114,114,80, 117,117,117,80, 121,121,121,80, 124,124,124,80,
    130,130,130,80, 133,133,133,80, 137,137,137,80, 140,140,140,80, 146,146,146,80, 149,149,149,80, 156,156,156,80, 156,156,156,80,
    159,159,159,80, 165,165,165,80, 168,168,168,80, 172,172,172,80, 175,175,175,80, 181,181,181,80, 184,184,184,80, 188,188,188,80,
    191,191,191,80, 197,197,197,80, 200,200,200,80, 207,207,207,80, 207,207,207,80, 213,213,213,80, 216,216,216,80, 219,219,219,80,
    223,223,223,80, 226,226,226,80, 232,232,232,80, 235,235,235,80, 239,239,239,80, 242,242,242,80, 248,248,248,80, 251,251,251,80};
static const struct bitmap_data testdata_32bppBGR = {
    &GUID_WICPixelFormat32bppBGR, 32, bits_32bppBGR, 32, 2, 96.0, 96.0};
static const struct bitmap_data testdata_32bppBGRA80 = {
    &GUID_WICPixelFormat32bppBGRA, 32, bits_32bppBGR, 32, 4, 96.0, 96.0};
static const struct bitmap_data testdata_32bppRGBA80 = {
    &GUID_WICPixelFormat32bppRGBA, 32, bits_32bppBGR, 32, 4, 96.0, 96.0};

static const BYTE bits_32bppBGRA[] = {
    255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255, 255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255,
    255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255, 255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255,
    255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255, 255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255,
    255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255, 255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255,
    0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255, 0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255,
    0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255, 0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255,
    0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255, 0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255,
    0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255, 0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255,
    3,3,3,255, 6,6,6,255, 12,12,12,255, 15,15,15,255, 19,19,19,255, 22,22,22,255, 28,28,28,255, 31,31,31,80,
    35,35,35,255, 38,38,38,255, 41,41,41,255, 47,47,47,255, 47,47,47,255, 54,54,54,255, 57,57,57,255, 63,63,63,80,
    66,66,66,255, 70,70,70,255, 73,73,73,255, 79,79,79,255, 82,82,82,255, 86,86,86,255, 89,89,89,255, 95,95,95,80,
    98,98,98,255, 98,98,98,255, 105,105,105,255, 108,108,108,255, 114,114,114,255, 117,117,117,255, 121,121,121,255, 124,124,124,80,
    130,130,130,255, 133,133,133,255, 137,137,137,255, 140,140,140,255, 146,146,146,255, 149,149,149,255, 156,156,156,255, 156,156,156,80,
    159,159,159,255, 165,165,165,255, 168,168,168,255, 172,172,172,255, 175,175,175,255, 181,181,181,255, 184,184,184,255, 188,188,188,80,
    191,191,191,255, 197,197,197,255, 200,200,200,255, 207,207,207,255, 207,207,207,255, 213,213,213,255, 216,216,216,255, 219,219,219,80,
    223,223,223,255, 226,226,226,255, 232,232,232,255, 235,235,235,255, 239,239,239,255, 242,242,242,255, 248,248,248,255, 251,251,251,80};
static const BYTE bits_32bppRGBA[] = {
    0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255, 0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255,
    0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255, 0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255,
    0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255, 0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255,
    0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255, 0,0,255,255, 0,255,0,255, 255,0,0,255, 0,0,0,255,
    255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255, 255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255,
    255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255, 255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255,
    255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255, 255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255,
    255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255, 255,255,0,255, 255,0,255,255, 0,255,255,255, 255,255,255,255};

static const struct bitmap_data testdata_32bppBGRA = {
    &GUID_WICPixelFormat32bppBGRA, 32, bits_32bppBGRA, 32, 2, 96.0, 96.0};
static const struct bitmap_data testdata_32bppRGBA = {
    &GUID_WICPixelFormat32bppRGBA, 32, bits_32bppRGBA, 32, 2, 96.0, 96.0};
static const struct bitmap_data testdata_32bppRGB = {
    &GUID_WICPixelFormat32bppRGB, 32, bits_32bppRGBA, 32, 2, 96.0, 96.0};

static const BYTE bits_32bppPBGRA[] = {
    80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80, 80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80,
    80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80, 80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80,
    80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80, 80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80,
    80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80, 80,0,0,80, 0,80,0,80, 0,0,80,80, 0,0,0,80,
    0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80, 0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80,
    0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80, 0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80,
    0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80, 0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80,
    0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80, 0,80,80,80, 80,0,80,80, 80,80,0,80, 80,80,80,80,
    1,1,1,80, 2,2,2,80, 4,4,4,80, 5,5,5,80, 6,6,6,80, 7,7,7,80, 9,9,9,80, 10,10,10,80,
    11,11,11,80, 12,12,12,80, 13,13,13,80, 15,15,15,80, 15,15,15,80, 17,17,17,80, 18,18,18,80, 20,20,20,80,
    21,21,21,80, 22,22,22,80, 23,23,23,80, 25,25,25,80, 26,26,26,80, 27,27,27,80, 28,28,28,80, 30,30,30,80,
    31,31,31,80, 31,31,31,80, 33,33,33,80, 34,34,34,80, 36,36,36,80, 37,37,37,80, 38,38,38,80, 39,39,39,80,
    41,41,41,80, 42,42,42,80, 43,43,43,80, 44,44,44,80, 46,46,46,80, 47,47,47,80, 49,49,49,80, 49,49,49,80,
    50,50,50,80, 52,52,52,80, 53,53,53,80, 54,54,54,80, 55,55,55,80, 57,57,57,80, 58,58,58,80, 59,59,59,80,
    60,60,60,80, 62,62,62,80, 63,63,63,80, 65,65,65,80, 65,65,65,80, 67,67,67,80, 68,68,68,80, 69,69,69,80,
    70,70,70,80, 71,71,71,80, 73,73,73,80, 74,74,74,80, 75,75,75,80, 76,76,76,80, 78,78,78,80, 79,79,79,80};
static const struct bitmap_data testdata_32bppPBGRA = {
    &GUID_WICPixelFormat32bppPBGRA, 32, bits_32bppPBGRA, 32, 4, 96.0, 96.0};
static const struct bitmap_data testdata_32bppPRGBA = {
    &GUID_WICPixelFormat32bppPRGBA, 32, bits_32bppPBGRA, 32, 4, 96.0, 96.0};

static const BYTE bits_64bppRGBA[] = {
    128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255, 128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255,
    128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255, 128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255,
    128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255, 128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255,
    128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255, 128,0,128,0,128,255,128,255, 128,0,128,255,128,0,128,255, 128,255,128,0,128,0,128,255, 128,0,128,0,128,0,128,255,
    128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255, 128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255,
    128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255, 128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255,
    128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255, 128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255,
    128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255, 128,255,128,255,128,0,128,255, 128,255,128,0,128,255,128,255, 128,0,128,255,128,255,128,255, 128,255,128,255,128,255,128,255};
static const struct bitmap_data testdata_64bppRGBA = {
    &GUID_WICPixelFormat64bppRGBA, 64, bits_64bppRGBA, 32, 2, 96.0, 96.0};

/* XP and 2003 use linear color conversion, later versions use sRGB gamma */
static const float bits_32bppGrayFloat_xp[] = {
    0.114000f,0.587000f,0.299000f,0.000000f,0.114000f,0.587000f,0.299000f,0.000000f,
    0.114000f,0.587000f,0.299000f,0.000000f,0.114000f,0.587000f,0.299000f,0.000000f,
    0.114000f,0.587000f,0.299000f,0.000000f,0.114000f,0.587000f,0.299000f,0.000000f,
    0.114000f,0.587000f,0.299000f,0.000000f,0.114000f,0.587000f,0.299000f,0.000000f,
    0.886000f,0.413000f,0.701000f,1.000000f,0.886000f,0.413000f,0.701000f,1.000000f,
    0.886000f,0.413000f,0.701000f,1.000000f,0.886000f,0.413000f,0.701000f,1.000000f,
    0.886000f,0.413000f,0.701000f,1.000000f,0.886000f,0.413000f,0.701000f,1.000000f,
    0.886000f,0.413000f,0.701000f,1.000000f,0.886000f,0.413000f,0.701000f,1.000000f};
static const struct bitmap_data testdata_32bppGrayFloat_xp = {
    &GUID_WICPixelFormat32bppGrayFloat, 32, (const BYTE *)bits_32bppGrayFloat_xp, 32, 2, 96.0, 96.0};

static const float bits_32bppGrayFloat[] = {
    0.072200f,0.715200f,0.212600f,0.000000f,0.072200f,0.715200f,0.212600f,0.000000f,
    0.072200f,0.715200f,0.212600f,0.000000f,0.072200f,0.715200f,0.212600f,0.000000f,
    0.072200f,0.715200f,0.212600f,0.000000f,0.072200f,0.715200f,0.212600f,0.000000f,
    0.072200f,0.715200f,0.212600f,0.000000f,0.072200f,0.715200f,0.212600f,0.000000f,
    0.927800f,0.284800f,0.787400f,1.000000f,0.927800f,0.284800f,0.787400f,1.000000f,
    0.927800f,0.284800f,0.787400f,1.000000f,0.927800f,0.284800f,0.787400f,1.000000f,
    0.927800f,0.284800f,0.787400f,1.000000f,0.927800f,0.284800f,0.787400f,1.000000f,
    0.927800f,0.284800f,0.787400f,1.000000f,0.927800f,0.284800f,0.787400f,1.000000f};
static const struct bitmap_data testdata_32bppGrayFloat = {
    &GUID_WICPixelFormat32bppGrayFloat, 32, (const BYTE *)bits_32bppGrayFloat, 32, 2, 96.0, 96.0, &testdata_32bppGrayFloat_xp};

static const BYTE bits_4bppGray_xp[] = {
    77,112,77,112,77,112,77,112,77,112,77,112,77,112,77,112,249,
    239,249,239,249,239,249,239,249,239,249,239,249,239,249,239};
static const struct bitmap_data testdata_4bppGray_xp = {
    &GUID_WICPixelFormat4bppGray, 4, bits_4bppGray_xp, 32, 2, 96.0, 96.0};

static const BYTE bits_4bppGray[] = {
    77,112,77,112,77,112,77,112,77,112,77,112,77,112,77,112,249,
    239,249,239,249,239,249,239,249,239,249,239,249,239,249,239};
static const struct bitmap_data testdata_4bppGray = {
    &GUID_WICPixelFormat4bppGray, 4, bits_4bppGray, 32, 2, 96.0, 96.0, &testdata_4bppGray_xp};

static const BYTE bits_8bppGray_xp[] = {
    29,150,76,0,29,150,76,0,29,150,76,0,29,150,76,0,
    29,150,76,0,29,150,76,0,29,150,76,0,29,150,76,0,
    226,105,179,255,226,105,179,255,226,105,179,255,226,105,179,255,
    226,105,179,255,226,105,179,255,226,105,179,255,226,105,179,255};
static const struct bitmap_data testdata_8bppGray_xp = {
    &GUID_WICPixelFormat8bppGray, 8, bits_8bppGray_xp, 32, 2, 96.0, 96.0};

static const BYTE bits_8bppGray[] = {
    76,220,127,0,76,220,127,0,76,220,127,0,76,220,127,0,
    76,220,127,0,76,220,127,0,76,220,127,0,76,220,127,0,
    247,145,230,255,247,145,230,255,247,145,230,255,247,145,230,255,
    247,145,230,255,247,145,230,255,247,145,230,255,247,145,230,255};
static const struct bitmap_data testdata_8bppGray = {
    &GUID_WICPixelFormat8bppGray, 8, bits_8bppGray, 32, 2, 96.0, 96.0, &testdata_8bppGray_xp};

static const BYTE bits_24bppBGR_gray[] = {
    76,76,76, 220,220,220, 127,127,127, 0,0,0, 76,76,76, 220,220,220, 127,127,127, 0,0,0,
    76,76,76, 220,220,220, 127,127,127, 0,0,0, 76,76,76, 220,220,220, 127,127,127, 0,0,0,
    76,76,76, 220,220,220, 127,127,127, 0,0,0, 76,76,76, 220,220,220, 127,127,127, 0,0,0,
    76,76,76, 220,220,220, 127,127,127, 0,0,0, 76,76,76, 220,220,220, 127,127,127, 0,0,0,
    247,247,247, 145,145,145, 230,230,230, 255,255,255, 247,247,247, 145,145,145, 230,230,230, 255,255,255,
    247,247,247, 145,145,145, 230,230,230, 255,255,255, 247,247,247, 145,145,145, 230,230,230, 255,255,255,
    247,247,247, 145,145,145, 230,230,230, 255,255,255, 247,247,247, 145,145,145, 230,230,230, 255,255,255,
    247,247,247, 145,145,145, 230,230,230, 255,255,255, 247,247,247, 145,145,145, 230,230,230, 255,255,255};
static const struct bitmap_data testdata_24bppBGR_gray = {
    &GUID_WICPixelFormat24bppBGR, 24, bits_24bppBGR_gray, 32, 2, 96.0, 96.0};

#define TO_16bppBGRA5551(b,g,r,a) ( \
        ((a >> 7) << 15) | \
        ((r >> 3) << 10) | \
        ((g >> 3) << 5) | \
        ((b >> 3)) \
)

static const WORD bits_16bppBGRA5551[] = {
    TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255), TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255),
    TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255), TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255),
    TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255), TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255),
    TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255), TO_16bppBGRA5551(255,0,0,255), TO_16bppBGRA5551(0,255,0,255), TO_16bppBGRA5551(0,0,255,255), TO_16bppBGRA5551(0,0,0,255),
    TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255), TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255),
    TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255), TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255),
    TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255), TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255),
    TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255), TO_16bppBGRA5551(0,255,255,255), TO_16bppBGRA5551(255,0,255,255), TO_16bppBGRA5551(255,255,0,255), TO_16bppBGRA5551(255,255,255,255)};

static const struct bitmap_data testdata_16bppBGRA5551 = {
    &GUID_WICPixelFormat16bppBGRA5551, 16, (BYTE*)bits_16bppBGRA5551, 32, 2, 96.0, 96.0};

static const WORD bits_48bppRGB[] = {
    0,0,0, 0,65535,0, 32767,32768,32767,
    65535,65535,65535, 10,10,10, 0,0,10};

static const struct bitmap_data testdata_48bppRGB = {
    &GUID_WICPixelFormat48bppRGB, 48, (BYTE*)bits_48bppRGB, 3, 2, 96.0, 96.0};

static const WORD bits_64bppRGBA_2[] = {
    0,0,0,65535, 0,65535,0,65535, 32767,32768,32767,65535,
    65535,65535,65535,65535, 10,10,10,65535, 0,0,10,65535,};

static const struct bitmap_data testdata_64bppRGBA_2 = {
    &GUID_WICPixelFormat64bppRGBA, 64, (BYTE*)bits_64bppRGBA_2, 3, 2, 96.0, 96.0};

static void test_conversion(const struct bitmap_data *src, const struct bitmap_data *dst, const char *name, BOOL todo)
{
    BitmapTestSrc *src_obj;
    IWICBitmapSource *dst_bitmap;
    HRESULT hr;

    CreateTestBitmap(src, &src_obj);

    hr = WICConvertBitmapSource(dst->format, &src_obj->IWICBitmapSource_iface, &dst_bitmap);
    todo_wine_if (todo)
        ok(hr == S_OK ||
           broken(hr == E_INVALIDARG || hr == WINCODEC_ERR_COMPONENTNOTFOUND) /* XP */, "WICConvertBitmapSource(%s) failed, hr=%lx\n", name, hr);

    if (hr == S_OK)
    {
        compare_bitmap_data(src, dst, dst_bitmap, name);

        IWICBitmapSource_Release(dst_bitmap);
    }

    DeleteTestBitmap(src_obj);
}

static void test_invalid_conversion(void)
{
    BitmapTestSrc *src_obj;
    IWICBitmapSource *dst_bitmap;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    /* convert to a non-pixel-format GUID */
    hr = WICConvertBitmapSource(&GUID_VendorMicrosoft, &src_obj->IWICBitmapSource_iface, &dst_bitmap);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "WICConvertBitmapSource returned %lx\n", hr);

    DeleteTestBitmap(src_obj);
}

static void test_default_converter(void)
{
    BitmapTestSrc *src_obj;
    IWICFormatConverter *converter;
    BOOL can_convert = TRUE;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    hr = CoCreateInstance(&CLSID_WICDefaultFormatConverter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)&converter);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICFormatConverter_CanConvert(converter, &GUID_WICPixelFormat32bppBGRA,
            &GUID_WICPixelFormat32bppBGR, &can_convert);
        ok(SUCCEEDED(hr), "CanConvert returned %lx\n", hr);
        ok(can_convert, "expected TRUE, got %i\n", can_convert);

        hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
            &GUID_WICPixelFormat32bppBGR, WICBitmapDitherTypeNone, NULL, 0.0,
            WICBitmapPaletteTypeCustom);
        ok(SUCCEEDED(hr), "Initialize returned %lx\n", hr);

        if (SUCCEEDED(hr))
            compare_bitmap_data(&testdata_32bppBGRA, &testdata_32bppBGR, (IWICBitmapSource*)converter, "default converter");

        IWICFormatConverter_Release(converter);
    }

    DeleteTestBitmap(src_obj);
}

static void test_converter_4bppGray(void)
{
    BitmapTestSrc *src_obj;
    IWICFormatConverter *converter;
    BOOL can_convert = TRUE;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    hr = CoCreateInstance(&CLSID_WICDefaultFormatConverter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)&converter);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICFormatConverter_CanConvert(converter, &GUID_WICPixelFormat32bppBGRA,
            &GUID_WICPixelFormat4bppGray, &can_convert);
        ok(SUCCEEDED(hr), "CanConvert returned %lx\n", hr);
        todo_wine ok(can_convert, "expected TRUE, got %i\n", can_convert);

        hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
            &GUID_WICPixelFormat4bppGray, WICBitmapDitherTypeNone, NULL, 0.0,
            WICBitmapPaletteTypeCustom);
        todo_wine ok(SUCCEEDED(hr), "Initialize returned %lx\n", hr);

        if (SUCCEEDED(hr))
            compare_bitmap_data(&testdata_32bppBGRA, &testdata_4bppGray, (IWICBitmapSource*)converter, "4bppGray converter");

        IWICFormatConverter_Release(converter);
    }

    DeleteTestBitmap(src_obj);
}

static void test_converter_8bppGray(void)
{
    BitmapTestSrc *src_obj;
    IWICFormatConverter *converter;
    BOOL can_convert = TRUE;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    hr = CoCreateInstance(&CLSID_WICDefaultFormatConverter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)&converter);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICFormatConverter_CanConvert(converter, &GUID_WICPixelFormat32bppBGRA,
            &GUID_WICPixelFormat8bppGray, &can_convert);
        ok(SUCCEEDED(hr), "CanConvert returned %lx\n", hr);
        ok(can_convert, "expected TRUE, got %i\n", can_convert);

        hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
            &GUID_WICPixelFormat8bppGray, WICBitmapDitherTypeNone, NULL, 0.0,
            WICBitmapPaletteTypeCustom);
        ok(SUCCEEDED(hr), "Initialize returned %lx\n", hr);

        if (SUCCEEDED(hr))
            compare_bitmap_data(&testdata_32bppBGRA, &testdata_8bppGray, (IWICBitmapSource*)converter, "8bppGray converter");

        IWICFormatConverter_Release(converter);
    }

    DeleteTestBitmap(src_obj);
}

typedef struct property_opt_test_data
{
    LPCOLESTR name;
    VARTYPE var_type;
    VARTYPE initial_var_type;
    int i_init_val;
    float f_init_val;
    BOOL skippable;
} property_opt_test_data;

static const WCHAR wszTiffCompressionMethod[] = {'T','i','f','f','C','o','m','p','r','e','s','s','i','o','n','M','e','t','h','o','d',0};
static const WCHAR wszCompressionQuality[] = {'C','o','m','p','r','e','s','s','i','o','n','Q','u','a','l','i','t','y',0};
static const WCHAR wszInterlaceOption[] = {'I','n','t','e','r','l','a','c','e','O','p','t','i','o','n',0};
static const WCHAR wszFilterOption[] = {'F','i','l','t','e','r','O','p','t','i','o','n',0};
static const WCHAR wszImageQuality[] = {'I','m','a','g','e','Q','u','a','l','i','t','y',0};
static const WCHAR wszBitmapTransform[] = {'B','i','t','m','a','p','T','r','a','n','s','f','o','r','m',0};
static const WCHAR wszLuminance[] = {'L','u','m','i','n','a','n','c','e',0};
static const WCHAR wszChrominance[] = {'C','h','r','o','m','i','n','a','n','c','e',0};
static const WCHAR wszJpegYCrCbSubsampling[] = {'J','p','e','g','Y','C','r','C','b','S','u','b','s','a','m','p','l','i','n','g',0};
static const WCHAR wszSuppressApp0[] = {'S','u','p','p','r','e','s','s','A','p','p','0',0};
static const WCHAR wszEnableV5Header32bppBGRA[] = {'E','n','a','b','l','e','V','5','H','e','a','d','e','r','3','2','b','p','p','B','G','R','A',0};

static const struct property_opt_test_data testdata_tiff_props[] = {
    { wszTiffCompressionMethod, VT_UI1,         VT_UI1,  WICTiffCompressionDontCare },
    { wszCompressionQuality,    VT_R4,          VT_EMPTY },
    { NULL }
};

static const struct property_opt_test_data testdata_png_props[] = {
    { wszInterlaceOption, VT_BOOL, VT_BOOL, 0 },
    { wszFilterOption,    VT_UI1,  VT_UI1, WICPngFilterUnspecified, 0.0f, TRUE /* not supported on XP/2k3 */},
    { NULL }
};

static const struct property_opt_test_data testdata_jpeg_props[] = {
    { wszImageQuality,         VT_R4,           VT_EMPTY },
    { wszBitmapTransform,      VT_UI1,          VT_UI1, WICBitmapTransformRotate0 },
    { wszLuminance,            VT_I4|VT_ARRAY,  VT_EMPTY },
    { wszChrominance,          VT_I4|VT_ARRAY,  VT_EMPTY },
    { wszJpegYCrCbSubsampling, VT_UI1,          VT_UI1, WICJpegYCrCbSubsamplingDefault, 0.0f, TRUE }, /* not supported on XP/2k3 */
    { wszSuppressApp0,         VT_BOOL,         VT_BOOL, FALSE },
    { NULL }
};

static const struct property_opt_test_data testdata_bmp_props[] = {
    { wszEnableV5Header32bppBGRA, VT_BOOL, VT_BOOL, VARIANT_FALSE, 0.0f, TRUE }, /* Supported since Win7 */
    { NULL }
};

static int find_property_index(const WCHAR* name, PROPBAG2* all_props, int all_prop_cnt)
{
    int i;
    for (i=0; i < all_prop_cnt; i++)
    {
        if (lstrcmpW(name, all_props[i].pstrName) == 0)
            return i;
    }
    return -1;
}

static void test_specific_encoder_properties(IPropertyBag2 *options, const property_opt_test_data* data, PROPBAG2* all_props, int all_prop_cnt)
{
    HRESULT hr;
    int i = 0;
    VARIANT pvarValue;
    HRESULT phrError = S_OK;

    while (data[i].name)
    {
        int idx = find_property_index(data[i].name, all_props, all_prop_cnt);
        PROPBAG2 pb = {0};
        pb.pstrName = (LPOLESTR)data[i].name;

        hr = IPropertyBag2_Read(options, 1, &pb, NULL, &pvarValue, &phrError);

        if (data[i].skippable && idx == -1)
        {
            win_skip("Property %s is not supported on this machine.\n", wine_dbgstr_w(data[i].name));
            i++;
            continue;
        }

        ok(idx >= 0, "Property %s not in output of GetPropertyInfo\n",
           wine_dbgstr_w(data[i].name));
        if (idx >= 0)
        {
            ok(all_props[idx].vt == data[i].var_type, "Property %s has unexpected vt type, vt=%i\n",
               wine_dbgstr_w(data[i].name), all_props[idx].vt);
            ok(all_props[idx].dwType == PROPBAG2_TYPE_DATA, "Property %s has unexpected dw type, vt=%li\n",
               wine_dbgstr_w(data[i].name), all_props[idx].dwType);
            ok(all_props[idx].cfType == 0, "Property %s has unexpected cf type, vt=%i\n",
               wine_dbgstr_w(data[i].name), all_props[idx].cfType);
        }

        ok(SUCCEEDED(hr), "Reading property %s from bag failed, hr=%lx\n",
           wine_dbgstr_w(data[i].name), hr);

        if (SUCCEEDED(hr))
        {
            /* On XP the initial type is always VT_EMPTY */
            ok(V_VT(&pvarValue) == data[i].initial_var_type || V_VT(&pvarValue) == VT_EMPTY,
               "Property %s has unexpected initial type, V_VT=%i\n",
               wine_dbgstr_w(data[i].name), V_VT(&pvarValue));

            if(V_VT(&pvarValue) == data[i].initial_var_type)
            {
                switch (data[i].initial_var_type)
                {
                    case VT_BOOL:
                    case VT_UI1:
                        ok(V_UNION(&pvarValue, bVal) == data[i].i_init_val, "Property %s has an unexpected initial value, pvarValue=%i\n",
                           wine_dbgstr_w(data[i].name), V_UNION(&pvarValue, bVal));
                        break;
                    case VT_R4:
                        ok(V_UNION(&pvarValue, fltVal) == data[i].f_init_val, "Property %s has an unexpected initial value, pvarValue=%f\n",
                           wine_dbgstr_w(data[i].name), V_UNION(&pvarValue, fltVal));
                        break;
                    default:
                        break;
                }
            }

            VariantClear(&pvarValue);
        }

        i++;
    }
}

static void test_encoder_properties(const CLSID* clsid_encoder, IPropertyBag2 *options)
{
    HRESULT hr;
    ULONG cProperties = 0;
    ULONG cProperties2 = 0;
    PROPBAG2 all_props[64] = {{0}}; /* Should be enough for every encoder out there */
    int i;

    /* CountProperties */
    {
        hr = IPropertyBag2_CountProperties(options, &cProperties);
        ok(SUCCEEDED(hr), "Reading property count, hr=%lx\n", hr);
    }

    /* GetPropertyInfo */
    {
        hr = IPropertyBag2_GetPropertyInfo(options, cProperties, 1, all_props, &cProperties2);
        ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "IPropertyBag2::GetPropertyInfo - iProperty out of bounce handled wrong, hr=%lx\n", hr);

        hr = IPropertyBag2_GetPropertyInfo(options, 0, cProperties+1, all_props, &cProperties2);
        ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "IPropertyBag2::GetPropertyInfo - cProperty out of bounce handled wrong, hr=%lx\n", hr);

        if (cProperties == 0) /* GetPropertyInfo can be called for zero items on Windows 8 but not on Windows 7 (wine behaves like Win8) */
        {
            cProperties2 = cProperties;
            hr = S_OK;
        }
        else
        {
            hr = IPropertyBag2_GetPropertyInfo(options, 0, min(64, cProperties), all_props, &cProperties2);
            ok(SUCCEEDED(hr), "Reading infos from property bag failed, hr=%lx\n", hr);
        }

        if (FAILED(hr))
            return;

        ok(cProperties == cProperties2, "Mismatch of property count (IPropertyBag2::CountProperties=%i, IPropertyBag2::GetPropertyInfo=%i)\n",
           (int)cProperties, (int)cProperties2);
    }

    if (IsEqualCLSID(clsid_encoder, &CLSID_WICTiffEncoder))
        test_specific_encoder_properties(options, testdata_tiff_props, all_props, cProperties2);
    else if (IsEqualCLSID(clsid_encoder, &CLSID_WICPngEncoder))
        test_specific_encoder_properties(options, testdata_png_props, all_props, cProperties2);
    else if (IsEqualCLSID(clsid_encoder, &CLSID_WICJpegEncoder))
        test_specific_encoder_properties(options, testdata_jpeg_props, all_props, cProperties2);
    else if (IsEqualCLSID(clsid_encoder, &CLSID_WICBmpEncoder))
        test_specific_encoder_properties(options, testdata_bmp_props, all_props, cProperties2);

    for (i=0; i < cProperties2; i++)
    {
        ok(all_props[i].pstrName != NULL, "Unset property name in output of IPropertyBag2::GetPropertyInfo\n");
        CoTaskMemFree(all_props[i].pstrName);
    }
}

static void load_stream(IUnknown *reader, IStream *stream)
{
    HRESULT hr;
    IWICPersistStream *persist;

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    hr = IWICPersistStream_LoadEx(persist, stream, NULL, 0);
    ok(hr == S_OK, "LoadEx failed, hr=%lx\n", hr);

    IWICPersistStream_Release(persist);
}

static void check_tiff_format(IStream *stream, const WICPixelFormatGUID *format)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT id, value;
    struct
    {
        USHORT byte_order;
        USHORT version;
        ULONG  dir_offset;
    } tiff;
    LARGE_INTEGER pos;
    UINT count, i;
    int width, height, bps, photo, samples, colormap;
    struct
    {
        int id, *value;
    } tag[] =
    {
        { 0x100, &width }, { 0x101, &height }, { 0x102, &bps },
        { 0x106, &photo }, { 0x115, &samples }, { 0x140, &colormap }
    };

    memset(&tiff, 0, sizeof(tiff));
    hr = IStream_Read(stream, &tiff, sizeof(tiff), NULL);
    ok(hr == S_OK, "IStream_Read error %#lx\n", hr);
    ok(tiff.byte_order == MAKEWORD('I','I') || tiff.byte_order == MAKEWORD('M','M'),
       "wrong TIFF byte order mark %02x\n", tiff.byte_order);
    ok(tiff.version == 42, "wrong TIFF version %u\n", tiff.version);

    pos.QuadPart = tiff.dir_offset;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = CoCreateInstance(&CLSID_WICIfdMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    load_stream((IUnknown *)reader, stream);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#lx\n", hr);
    ok(count != 0, "wrong count %u\n", count);

    for (i = 0; i < ARRAY_SIZE(tag); i++)
    {
        PropVariantInit(&id);
        PropVariantInit(&value);

        id.vt = VT_UI2;
        id.uiVal = tag[i].id;
        hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
        ok(hr == S_OK || (tag[i].id == 0x140 && hr == WINCODEC_ERR_PROPERTYNOTFOUND),
           "GetValue(%04x) error %#lx\n", tag[i].id, hr);
        if (hr == S_OK)
        {
            ok(value.vt == VT_UI2 || value.vt == VT_UI4 || value.vt == (VT_UI2 | VT_VECTOR), "wrong vt: %d\n", value.vt);
            tag[i].value[0] = value.uiVal;
        }
        else
            tag[i].value[0] = -1;

        PropVariantClear(&value);
    }

    IWICMetadataReader_Release(reader);

    if (IsEqualGUID(format, &GUID_WICPixelFormatBlackWhite))
    {
        ok(width == 32, "wrong width %u\n", width);
        ok(height == 2, "wrong height %u\n", height);

        ok(bps == 1, "wrong bps %d\n", bps);
        ok(photo == 1, "wrong photometric %d\n", photo);
        ok(samples == 1, "wrong samples %d\n", samples);
        ok(colormap == -1, "wrong colormap %d\n", colormap);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat1bppIndexed))
    {
        ok(width == 32, "wrong width %u\n", width);
        ok(height == 2, "wrong height %u\n", height);

        ok(bps == 1, "wrong bps %d\n", bps);
        ok(photo == 3, "wrong photometric %d\n", photo);
        ok(samples == 1, "wrong samples %d\n", samples);
        ok(colormap == 6, "wrong colormap %d\n", colormap);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat4bppIndexed))
    {
        ok(width == 32, "wrong width %u\n", width);
        ok(height == 2, "wrong height %u\n", height);

        ok(bps == 4, "wrong bps %d\n", bps);
        ok(photo == 3, "wrong photometric %d\n", photo);
        ok(samples == 1, "wrong samples %d\n", samples);
        ok(colormap == 48, "wrong colormap %d\n", colormap);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat8bppIndexed))
    {
        ok(width == 32, "wrong width %u\n", width);
        ok(height == 2, "wrong height %u\n", height);

        ok(bps == 8, "wrong bps %d\n", bps);
        ok(photo == 3, "wrong photometric %d\n", photo);
        ok(samples == 1, "wrong samples %d\n", samples);
        ok(colormap == 768, "wrong colormap %d\n", colormap);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat24bppBGR))
    {
        ok(width == 32, "wrong width %u\n", width);
        ok(height == 2, "wrong height %u\n", height);

        ok(bps == 3, "wrong bps %d\n", bps);
        ok(photo == 2, "wrong photometric %d\n", photo);
        ok(samples == 3, "wrong samples %d\n", samples);
        ok(colormap == -1, "wrong colormap %d\n", colormap);
    }
    else
        ok(0, "unknown TIFF pixel format %s\n", wine_dbgstr_guid(format));
}

static void check_bmp_format(IStream *stream, const WICPixelFormatGUID *format)
{
    HRESULT hr;
    BITMAPFILEHEADER bfh;
    BITMAPV5HEADER bih;

    hr = IStream_Read(stream, &bfh, sizeof(bfh), NULL);
    ok(hr == S_OK, "IStream_Read error %#lx\n", hr);

    ok(bfh.bfType == 0x4d42, "wrong BMP signature %02x\n", bfh.bfType);
    ok(bfh.bfReserved1 == 0, "wrong bfReserved1 %02x\n", bfh.bfReserved1);
    ok(bfh.bfReserved2 == 0, "wrong bfReserved2 %02x\n", bfh.bfReserved2);

    hr = IStream_Read(stream, &bih, sizeof(bih), NULL);
    ok(hr == S_OK, "IStream_Read error %#lx\n", hr);

    if (IsEqualGUID(format, &GUID_WICPixelFormat1bppIndexed))
    {
        ok(bfh.bfOffBits == 0x0436, "wrong bfOffBits %08lx\n", bfh.bfOffBits);

        ok(bih.bV5Width == 32, "wrong width %lu\n", bih.bV5Width);
        ok(bih.bV5Height == 2, "wrong height %lu\n", bih.bV5Height);

        ok(bih.bV5Planes == 1, "wrong Planes %d\n", bih.bV5Planes);
        ok(bih.bV5BitCount == 1, "wrong BitCount %d\n", bih.bV5BitCount);
        ok(bih.bV5ClrUsed == 256, "wrong ClrUsed %ld\n", bih.bV5ClrUsed);
        ok(bih.bV5ClrImportant == 256, "wrong ClrImportant %ld\n", bih.bV5ClrImportant);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat4bppIndexed))
    {
        ok(bfh.bfOffBits == 0x0436, "wrong bfOffBits %08lx\n", bfh.bfOffBits);

        ok(bih.bV5Width == 32, "wrong width %lu\n", bih.bV5Width);
        ok(bih.bV5Height == 2, "wrong height %lu\n", bih.bV5Height);

        ok(bih.bV5Planes == 1, "wrong Planes %d\n", bih.bV5Planes);
        ok(bih.bV5BitCount == 4, "wrong BitCount %d\n", bih.bV5BitCount);
        ok(bih.bV5ClrUsed == 256, "wrong ClrUsed %ld\n", bih.bV5ClrUsed);
        ok(bih.bV5ClrImportant == 256, "wrong ClrImportant %ld\n", bih.bV5ClrImportant);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat8bppIndexed))
    {
        ok(bfh.bfOffBits == 0x0436, "wrong bfOffBits %08lx\n", bfh.bfOffBits);

        ok(bih.bV5Width == 32, "wrong width %lu\n", bih.bV5Width);
        ok(bih.bV5Height == 2, "wrong height %lu\n", bih.bV5Height);

        ok(bih.bV5Planes == 1, "wrong Planes %d\n", bih.bV5Planes);
        ok(bih.bV5BitCount == 8, "wrong BitCount %d\n", bih.bV5BitCount);
        ok(bih.bV5ClrUsed == 256, "wrong ClrUsed %ld\n", bih.bV5ClrUsed);
        ok(bih.bV5ClrImportant == 256, "wrong ClrImportant %ld\n", bih.bV5ClrImportant);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat32bppBGR))
    {
        ok(bfh.bfOffBits == 0x0036, "wrong bfOffBits %08lx\n", bfh.bfOffBits);

        ok(bih.bV5Width == 32, "wrong width %lu\n", bih.bV5Width);
        ok(bih.bV5Height == 2, "wrong height %lu\n", bih.bV5Height);

        ok(bih.bV5Planes == 1, "wrong Planes %d\n", bih.bV5Planes);
        ok(bih.bV5BitCount == 32, "wrong BitCount %d\n", bih.bV5BitCount);
        ok(bih.bV5ClrUsed == 0, "wrong ClrUsed %ld\n", bih.bV5ClrUsed);
        ok(bih.bV5ClrImportant == 0, "wrong ClrImportant %ld\n", bih.bV5ClrImportant);
    }
    else
        ok(0, "unknown BMP pixel format %s\n", wine_dbgstr_guid(format));
}

static unsigned be_uint(unsigned val)
{
    union
    {
        unsigned val;
        char c[4];
    } u;

    u.val = val;
    return (u.c[0] << 24) | (u.c[1] << 16) | (u.c[2] << 8) | u.c[3];
}

static void check_png_format(IStream *stream, const WICPixelFormatGUID *format)
{
    static const char png_sig[8] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a};
    static const char png_IHDR[8] = {0,0,0,0x0d,'I','H','D','R'};
    HRESULT hr;
    struct
    {
        char png_sig[8];
        char ihdr_sig[8];
        unsigned width, height;
        char bit_depth, color_type, compression, filter, interlace;
    } png;

    memset(&png, 0, sizeof(png));
    hr = IStream_Read(stream, &png, sizeof(png), NULL);
    ok(hr == S_OK, "IStream_Read error %#lx\n", hr);

    ok(!memcmp(png.png_sig, png_sig, sizeof(png_sig)), "expected PNG signature\n");
    ok(!memcmp(png.ihdr_sig, png_IHDR, sizeof(png_IHDR)), "expected PNG IHDR\n");

    if (IsEqualGUID(format, &GUID_WICPixelFormatBlackWhite))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 1, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 0, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat1bppIndexed))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 1, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat2bppIndexed))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 2, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat4bppIndexed))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 4, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat8bppIndexed))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 8, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat24bppBGR))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 8, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 2, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0 || png.interlace == 1, "wrong interlace %d\n", png.interlace);
    }
    else
        ok(0, "unknown PNG pixel format %s\n", wine_dbgstr_guid(format));
}

static void check_gif_format(IStream *stream, const WICPixelFormatGUID *format)
{
#include "pshpack1.h"
    struct logical_screen_descriptor
    {
        char signature[6];
        USHORT width;
        USHORT height;
        BYTE packed;
        /* global_color_table_flag : 1;
         * color_resolution : 3;
         * sort_flag : 1;
         * global_color_table_size : 3;
         */
        BYTE background_color_index;
        BYTE pixel_aspect_ratio;
    } lsd;
#include "poppack.h"
    UINT color_resolution;
    HRESULT hr;

    memset(&lsd, 0, sizeof(lsd));
    hr = IStream_Read(stream, &lsd, sizeof(lsd), NULL);
    ok(hr == S_OK, "IStream_Read error %#lx\n", hr);

    ok(!memcmp(lsd.signature, "GIF89a", 6), "wrong GIF signature %.6s\n", lsd.signature);

    ok(lsd.width == 32, "wrong width %u\n", lsd.width);
    ok(lsd.height == 2, "wrong height %u\n", lsd.height);
    color_resolution = 1 << (((lsd.packed >> 4) & 0x07) + 1);
    ok(color_resolution == 256, "wrong color resolution %u\n", color_resolution);
    ok(lsd.pixel_aspect_ratio == 0, "wrong pixel_aspect_ratio %u\n", lsd.pixel_aspect_ratio);
}

static void check_bitmap_format(IStream *stream, const CLSID *encoder, const WICPixelFormatGUID *format)
{
    HRESULT hr;
    LARGE_INTEGER pos;

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_SET, (ULARGE_INTEGER *)&pos);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    if (IsEqualGUID(encoder, &CLSID_WICPngEncoder))
        check_png_format(stream, format);
    else if (IsEqualGUID(encoder, &CLSID_WICBmpEncoder))
        check_bmp_format(stream, format);
    else if (IsEqualGUID(encoder, &CLSID_WICTiffEncoder))
        check_tiff_format(stream, format);
    else if (IsEqualGUID(encoder, &CLSID_WICGifEncoder))
        check_gif_format(stream, format);
    else
        ok(0, "unknown encoder %s\n", wine_dbgstr_guid(encoder));

    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);
}

struct setting {
    const WCHAR *name;
    PROPBAG2_TYPE type;
    VARTYPE vt;
    void *value;
};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static void test_set_frame_palette(IWICBitmapFrameEncode *frameencode)
{
    IWICComponentFactory *factory;
    IWICPalette *palette;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, NULL);
    ok(hr == E_INVALIDARG, "SetPalette failed, hr=%lx\n", hr);

    hr = IWICComponentFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette failed, hr=%lx\n", hr);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
    todo_wine
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr=%lx\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedHalftone256, FALSE);
    ok(hr == S_OK, "InitializePredefined failed, hr=%lx\n", hr);

    EXPECT_REF(palette, 1);
    hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
    ok(hr == S_OK, "SetPalette failed, hr=%lx\n", hr);
    EXPECT_REF(palette, 1);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, NULL);
    ok(hr == E_INVALIDARG, "SetPalette failed, hr=%lx\n", hr);

    IWICPalette_Release(palette);
    IWICComponentFactory_Release(factory);
}

static void test_multi_encoder_impl(const struct bitmap_data **srcs, const CLSID* clsid_encoder,
    const struct bitmap_data **dsts, const CLSID *clsid_decoder, WICRect *rc,
    const struct setting *settings, const char *name, IWICPalette *palette, BOOL set_size)
{
    const GUID *container_format = NULL;
    HRESULT hr;
    IWICBitmapEncoder *encoder;
    BitmapTestSrc *src_obj;
    HGLOBAL hglobal;
    IStream *stream;
    IWICBitmapFrameEncode *frameencode;
    IPropertyBag2 *options=NULL;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    WICPixelFormatGUID pixelformat;
    GUID guid;
    int i;

    hr = CoCreateInstance(clsid_encoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void **)&encoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);

    hr = IWICBitmapEncoder_GetContainerFormat(encoder, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    if (IsEqualGUID(clsid_encoder, &CLSID_WICPngEncoder))
        container_format = &GUID_ContainerFormatPng;
    else if (IsEqualGUID(clsid_encoder, &CLSID_WICBmpEncoder))
        container_format = &GUID_ContainerFormatBmp;
    else if (IsEqualGUID(clsid_encoder, &CLSID_WICTiffEncoder))
        container_format = &GUID_ContainerFormatTiff;
    else if (IsEqualGUID(clsid_encoder, &CLSID_WICJpegEncoder))
        container_format = &GUID_ContainerFormatJpeg;
    else if (IsEqualGUID(clsid_encoder, &CLSID_WICGifEncoder))
        container_format = &GUID_ContainerFormatGif;
    else
        ok(0, "Unknown encoder %s.\n", wine_dbgstr_guid(clsid_encoder));

    if (container_format)
    {
        memset(&guid, 0, sizeof(guid));
        hr = IWICBitmapEncoder_GetContainerFormat(encoder, &guid);
        ok(SUCCEEDED(hr), "Failed to get container format, hr %#lx.\n", hr);
        ok(IsEqualGUID(container_format, &guid), "Unexpected container format %s.\n", wine_dbgstr_guid(&guid));
    }

    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    ok(SUCCEEDED(hr), "Initialize failed, hr=%lx\n", hr);

    /* Encoder options are optional. */
    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, NULL);
    ok(SUCCEEDED(hr), "Failed to create encode frame, hr %#lx.\n", hr);

    IStream_Release(stream);
    IWICBitmapEncoder_Release(encoder);
    IWICBitmapFrameEncode_Release(frameencode);

    hr = CoCreateInstance(clsid_encoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void**)&encoder);
    ok(SUCCEEDED(hr), "CoCreateInstance(%s) failed, hr=%lx\n", wine_dbgstr_guid(clsid_encoder), hr);
    if (SUCCEEDED(hr))
    {
        hglobal = GlobalAlloc(GMEM_MOVEABLE, 0);
        ok(hglobal != NULL, "GlobalAlloc failed\n");
        if (hglobal)
        {
            hr = CreateStreamOnHGlobal(hglobal, TRUE, &stream);
            ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        }

        if (hglobal && SUCCEEDED(hr))
        {
            if (palette)
            {
                hr = IWICBitmapEncoder_SetPalette(encoder, palette);
                ok(hr == WINCODEC_ERR_NOTINITIALIZED, "wrong error %#lx (%s)\n", hr, name);
            }

            hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
            ok(SUCCEEDED(hr), "Initialize failed, hr=%lx\n", hr);

            if (palette)
            {
                hr = IWICBitmapEncoder_SetPalette(encoder, palette);
                if (IsEqualGUID(clsid_encoder, &CLSID_WICGifEncoder))
                    ok(hr == S_OK, "SetPalette failed, hr=%#lx\n", hr);
                else
                    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "wrong error %#lx\n", hr);
                hr = S_OK;
            }

            i=0;
            while (SUCCEEDED(hr) && srcs[i])
            {
                CreateTestBitmap(srcs[i], &src_obj);

                hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, &options);
                ok(SUCCEEDED(hr), "CreateFrame failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    ok(options != NULL, "Encoder initialization has not created an property bag\n");
                    if(options)
                        test_encoder_properties(clsid_encoder, options);

                    if (settings)
                    {
                        int j;
                        for (j=0; settings[j].name; j++)
                        {
                            PROPBAG2 propbag;
                            VARIANT var;

                            memset(&propbag, 0, sizeof(propbag));
                            memset(&var, 0, sizeof(var));
                            propbag.pstrName = (LPOLESTR)settings[j].name;
                            propbag.dwType = settings[j].type;
                            V_VT(&var) = settings[j].vt;
                            V_UNKNOWN(&var) = settings[j].value;

                            hr = IPropertyBag2_Write(options, 1, &propbag, &var);
                            ok(SUCCEEDED(hr), "Writing property %s failed, hr=%lx\n", wine_dbgstr_w(settings[j].name), hr);
                        }
                    }

                    if (palette)
                    {
                        hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
                        ok(hr == WINCODEC_ERR_NOTINITIALIZED, "wrong error %#lx\n", hr);
                    }

                    hr = IWICBitmapFrameEncode_Initialize(frameencode, options);
                    ok(SUCCEEDED(hr), "Initialize failed, hr=%lx\n", hr);

                    memcpy(&pixelformat, srcs[i]->format, sizeof(GUID));
                    hr = IWICBitmapFrameEncode_SetPixelFormat(frameencode, &pixelformat);
                    ok(SUCCEEDED(hr), "SetPixelFormat failed, hr=%lx\n", hr);
                    ok(IsEqualGUID(&pixelformat, dsts[i]->format) ||
                       (IsEqualGUID(clsid_encoder, &CLSID_WICTiffEncoder) && srcs[i]->bpp == 2 && IsEqualGUID(&pixelformat, &GUID_WICPixelFormat4bppIndexed)) ||
                       (IsEqualGUID(clsid_encoder, &CLSID_WICBmpEncoder) && srcs[i]->bpp == 2 && IsEqualGUID(&pixelformat, &GUID_WICPixelFormat4bppIndexed)),
                        "SetPixelFormat changed the format to %s (%s)\n", wine_dbgstr_guid(&pixelformat), name);

                    if (set_size)
                    {
                        hr = IWICBitmapFrameEncode_SetSize(frameencode, srcs[i]->width, srcs[i]->height);
                        ok(hr == S_OK, "SetSize failed, hr=%lx\n", hr);
                    }

                    if (IsEqualGUID(clsid_encoder, &CLSID_WICPngEncoder))
                        test_set_frame_palette(frameencode);

                    if (palette)
                    {
                        hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
                        ok(SUCCEEDED(hr), "SetPalette failed, hr=%lx (%s)\n", hr, name);
                    }

                    hr = IWICBitmapFrameEncode_WriteSource(frameencode, &src_obj->IWICBitmapSource_iface, rc);
                    if (rc && (rc->Width <= 0 || rc->Height <= 0))
                    {
                        /* WriteSource fails but WriteSource_Proxy succeeds. */
                        ok(hr == E_INVALIDARG, "WriteSource should fail, hr=%lx (%s)\n", hr, name);
                        hr = IWICBitmapFrameEncode_WriteSource_Proxy(frameencode, &src_obj->IWICBitmapSource_iface, rc);
                        if (!set_size && rc->Width < 0)
                            todo_wine
                            ok(hr == WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS ||
                               hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW) /* win11 */,
                               "WriteSource_Proxy(%dx%d) got unexpected hr %lx (%s)\n", rc->Width, rc->Height, hr, name);
                        else
                            ok(hr == S_OK, "WriteSource_Proxy failed, %dx%d, hr=%lx (%s)\n", rc->Width, rc->Height, hr, name);
                    }
                    else
                    {
                        if (rc)
                            ok(SUCCEEDED(hr), "WriteSource(%dx%d) failed, hr=%lx (%s)\n", rc->Width, rc->Height, hr, name);
                        else
                            todo_wine_if((IsEqualGUID(clsid_encoder, &CLSID_WICTiffEncoder) && srcs[i]->bpp == 2) ||
                                         (IsEqualGUID(clsid_encoder, &CLSID_WICBmpEncoder)  && srcs[i]->bpp == 2))
                            ok(hr == S_OK, "WriteSource(NULL) failed, hr=%lx (%s)\n", hr, name);

                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapFrameEncode_Commit(frameencode);
                        if (!set_size && rc && rc->Height < 0)
                            todo_wine
                            ok(hr == WINCODEC_ERR_UNEXPECTEDSIZE, "Commit got unexpected hr %lx (%s)\n", hr, name);
                        else
                            ok(hr == S_OK, "Commit failed, hr=%lx (%s)\n", hr, name);
                    }

                    IWICBitmapFrameEncode_Release(frameencode);
                    IPropertyBag2_Release(options);
                }

                DeleteTestBitmap(src_obj);

                i++;
            }

            if (clsid_decoder == NULL)
            {
                IStream_Release(stream);
                IWICBitmapEncoder_Release(encoder);
                return;
            }

            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapEncoder_Commit(encoder);
                ok(SUCCEEDED(hr), "Commit failed, hr=%lx\n", hr);

                if (IsEqualGUID(&pixelformat, dsts[0]->format))
                    check_bitmap_format(stream, clsid_encoder, dsts[0]->format);
            }

            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(clsid_decoder, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICBitmapDecoder, (void**)&decoder);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            }

            if (SUCCEEDED(hr))
            {
                IWICPalette *frame_palette;

                hr = IWICImagingFactory_CreatePalette(factory, &frame_palette);
                ok(hr == S_OK, "CreatePalette error %#lx\n", hr);

                hr = IWICBitmapDecoder_CopyPalette(decoder, frame_palette);
                if (IsEqualGUID(clsid_decoder, &CLSID_WICGifDecoder))
                    ok(hr == WINCODEC_ERR_WRONGSTATE, "wrong error %#lx\n", hr);
                else
                    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#lx\n", hr);

                hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
                ok(SUCCEEDED(hr), "Initialize failed, hr=%lx\n", hr);

                hr = IWICBitmapDecoder_CopyPalette(decoder, frame_palette);
                if (IsEqualGUID(clsid_decoder, &CLSID_WICGifDecoder))
                    ok(hr == S_OK || broken(hr == WINCODEC_ERR_FRAMEMISSING) /* XP */, "CopyPalette failed, hr=%#lx\n", hr);
                else
                    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#lx\n", hr);

                hr = S_OK;
                i=0;
                while (SUCCEEDED(hr) && dsts[i])
                {
                    hr = IWICBitmapDecoder_GetFrame(decoder, i, &framedecode);
                    ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx (%s)\n", hr, name);

                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &pixelformat);
                        ok(hr == S_OK, "GetPixelFormat) failed, hr=%lx (%s)\n", hr, name);
                        if (IsEqualGUID(&pixelformat, dsts[i]->format))
                            compare_bitmap_data(srcs[i], dsts[i], (IWICBitmapSource*)framedecode, name);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, frame_palette);
                        if (winetest_debug > 1)
                            trace("%s, bpp %d, %s, hr %#lx\n", name, dsts[i]->bpp, wine_dbgstr_guid(dsts[i]->format), hr);
                        if (dsts[i]->bpp > 8 || IsEqualGUID(dsts[i]->format, &GUID_WICPixelFormatBlackWhite))
                            ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#lx\n", hr);
                        else
                        {
                            UINT count, ret;
                            WICColor colors[256];

                            ok(hr == S_OK, "CopyPalette error %#lx (%s)\n", hr, name);

                            count = 0;
                            hr = IWICPalette_GetColorCount(frame_palette, &count);
                            ok(hr == S_OK, "GetColorCount error %#lx\n", hr);

                            memset(colors, 0, sizeof(colors));
                            ret = 0;
                            hr = IWICPalette_GetColors(frame_palette, count, colors, &ret);
                            ok(hr == S_OK, "GetColors error %#lx\n", hr);
                            ok(ret == count, "expected %u, got %u\n", count, ret);
                            if (IsEqualGUID(clsid_decoder, &CLSID_WICPngDecoder))
                            {
                                /* Newer libpng versions don't accept larger palettes than the declared
                                 * bit depth, so we need to generate the palette of the correct length.
                                 */
                                ok(count == 256 || (dsts[i]->bpp == 1 && count == 2) ||
                                   (dsts[i]->bpp == 2 && count == 4) || (dsts[i]->bpp == 4 && count == 16),
                                   "expected 256, got %u (%s)\n", count, name);

                                ok(colors[0] == 0x11111111, "got %08x (%s)\n", colors[0], name);
                                ok(colors[1] == 0x22222222, "got %08x (%s)\n", colors[1], name);
                                if (count > 2)
                                {
                                    ok(colors[2] == 0x33333333, "got %08x (%s)\n", colors[2], name);
                                    ok(colors[3] == 0x44444444, "got %08x (%s)\n", colors[3], name);
                                    if (count > 4)
                                    {
                                        ok(colors[4] == 0x55555555, "got %08x (%s)\n", colors[4], name);
                                        ok(colors[5] == 0, "got %08x (%s)\n", colors[5], name);
                                    }
                                }
                            }
                            else if (IsEqualGUID(clsid_decoder, &CLSID_WICBmpDecoder) ||
                                     IsEqualGUID(clsid_decoder, &CLSID_WICTiffDecoder) ||
                                     IsEqualGUID(clsid_decoder, &CLSID_WICGifDecoder))
                            {
                                if (IsEqualGUID(&pixelformat, &GUID_WICPixelFormatBlackWhite) ||
                                    IsEqualGUID(&pixelformat, &GUID_WICPixelFormat8bppIndexed))
                                {
                                    ok(count == 256, "expected 256, got %u (%s)\n", count, name);

                                    ok(colors[0] == 0xff111111, "got %08x (%s)\n", colors[0], name);
                                    ok(colors[1] == 0xff222222, "got %08x (%s)\n", colors[1], name);
                                    ok(colors[2] == 0xff333333, "got %08x (%s)\n", colors[2], name);
                                    ok(colors[3] == 0xff444444, "got %08x (%s)\n", colors[3], name);
                                    ok(colors[4] == 0xff555555, "got %08x (%s)\n", colors[4], name);
                                    ok(colors[5] == 0xff000000, "got %08x (%s)\n", colors[5], name);
                                }
                                else if (IsEqualGUID(&pixelformat, &GUID_WICPixelFormat4bppIndexed))
                                {
                                    ok(count == 16, "expected 16, got %u (%s)\n", count, name);

                                    ok(colors[0] == 0xff111111, "got %08x (%s)\n", colors[0], name);
                                    ok(colors[1] == 0xff222222, "got %08x (%s)\n", colors[1], name);
                                    ok(colors[2] == 0xff333333, "got %08x (%s)\n", colors[2], name);
                                    ok(colors[3] == 0xff444444, "got %08x (%s)\n", colors[3], name);
                                    ok(colors[4] == 0xff555555, "got %08x (%s)\n", colors[4], name);
                                    ok(colors[5] == 0xff000000, "got %08x (%s)\n", colors[5], name);
                                }
                                else
                                {
                                    ok(count == 2, "expected 2, got %u (%s)\n", count, name);

                                    ok(colors[0] == 0xff111111, "got %08x (%s)\n", colors[0], name);
                                    ok(colors[1] == 0xff222222, "got %08x (%s)\n", colors[1], name);
                                }
                            }
                            else
                            {
                                ok(count == 2, "expected 2, got %u (%s)\n", count, name);

                                ok(colors[0] == 0xff111111, "got %08x\n", colors[0]);
                                ok(colors[1] == 0xff222222, "got %08x\n", colors[1]);
                            }
                        }

                        IWICBitmapFrameDecode_Release(framedecode);
                    }

                    i++;
                }

                IWICPalette_Release(frame_palette);
                IWICBitmapDecoder_Release(decoder);
            }

            IStream_Release(stream);
        }

        IWICBitmapEncoder_Release(encoder);
    }
}

static void test_multi_encoder(const struct bitmap_data **srcs, const CLSID* clsid_encoder,
    const struct bitmap_data **dsts, const CLSID *clsid_decoder, WICRect *rc,
    const struct setting *settings, const char *name, IWICPalette *palette)
{
    test_multi_encoder_impl(srcs, clsid_encoder, dsts, clsid_decoder, rc, settings, name, palette, TRUE);
    test_multi_encoder_impl(srcs, clsid_encoder, dsts, clsid_decoder, rc, settings, name, palette, FALSE);
}

static void test_encoder(const struct bitmap_data *src, const CLSID* clsid_encoder,
    const struct bitmap_data *dst, const CLSID *clsid_decoder, const char *name)
{
    const struct bitmap_data *srcs[2];
    const struct bitmap_data *dsts[2];
    WICColor colors[256];
    IWICPalette *palette;
    HRESULT hr;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);

    memset(colors, 0, sizeof(colors));
    colors[0] = 0x11111111;
    colors[1] = 0x22222222;
    colors[2] = 0x33333333;
    colors[3] = 0x44444444;
    colors[4] = 0x55555555;
    /* TIFF decoder fails to decode a 8bpp frame if palette has less than 256 colors */
    hr = IWICPalette_InitializeCustom(palette, colors, 256);
    ok(hr == S_OK, "InitializeCustom error %#lx\n", hr);

    srcs[0] = src;
    srcs[1] = NULL;
    dsts[0] = dst;
    dsts[1] = NULL;

    test_multi_encoder(srcs, clsid_encoder, dsts, clsid_decoder, NULL, NULL, name, palette);

    IWICPalette_Release(palette);
}

static void test_encoder_rects(void)
{
    const struct bitmap_data *srcs[2];
    const struct bitmap_data *dsts[2];
    WICRect rc;

    srcs[0] = &testdata_24bppBGR;
    srcs[1] = NULL;
    dsts[0] = &testdata_24bppBGR;
    dsts[1] = NULL;

    rc.X = 0;
    rc.Y = 0;
    rc.Width = 32;
    rc.Height = 2;

    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects full", NULL);

    rc.Width = 0;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects width=0", NULL);

    rc.Width = -1;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects width=-1", NULL);

    rc.Width = 32;
    rc.Height = 0;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects height=0", NULL);

    rc.Height = -1;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects height=-1", NULL);
}

static const struct bitmap_data *multiple_frames[3] = {
    &testdata_24bppBGR,
    &testdata_24bppBGR,
    NULL};

static const struct bitmap_data *single_frame[2] = {
    &testdata_24bppBGR,
    NULL};

static const struct setting png_interlace_settings[] = {
    {wszInterlaceOption, PROPBAG2_TYPE_DATA, VT_BOOL, (void*)VARIANT_TRUE},
    {NULL}
};

static void test_converter_8bppIndexed(void)
{
    HRESULT hr;
    BitmapTestSrc *src_obj;
    IWICFormatConverter *converter;
    IWICPalette *palette;
    UINT count, i;
    BYTE buf[32 * 2 * 3]; /* enough to hold 32x2 24bppBGR data */

    CreateTestBitmap(&testdata_24bppBGR, &src_obj);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    count = 0xdeadbeef;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 0, "expected 0, got %u\n", count);

    /* NULL palette + Custom type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == 0xdeadbeef, "unexpected error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32 * 3, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    /* NULL palette + Custom type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == E_INVALIDARG, "unexpected error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "unexpected error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat4bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == E_INVALIDARG, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat2bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == E_INVALIDARG, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat1bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == E_INVALIDARG, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat1bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeMedianCut);
    todo_wine ok(hr == S_OK, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat1bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeFixedBW);
    todo_wine ok(hr == S_OK, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat1bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeFixedHalftone8);
    todo_wine ok(hr == E_INVALIDARG, "unexpected error %#lx\n", hr);
    IWICFormatConverter_Release(converter);

    /* empty palette + Custom type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        palette, 0.0, WICBitmapPaletteTypeCustom);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);
    count = 0xdeadbeef;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 0, "expected 0, got %u\n", count);
    memset(buf, 0xaa, sizeof(buf));
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    count = 0;
    for (i = 0; i < 32 * 2; i++)
        if (buf[i] != 0) count++;
    ok(count == 0, "expected 0\n");
    IWICFormatConverter_Release(converter);

    /* NULL palette + Predefined type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeFixedGray16);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);
    count = 0xdeadbeef;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 16, "expected 16, got %u\n", count);
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    count = 0;
    for (i = 0; i < 32 * 2; i++)
        if (buf[i] != 0) count++;
    ok(count != 0, "expected != 0\n");
    IWICFormatConverter_Release(converter);

    /* not empty palette + Predefined type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        palette, 0.0, WICBitmapPaletteTypeFixedHalftone64);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);
    count = 0xdeadbeef;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 16, "expected 16, got %u\n", count);
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    count = 0;
    for (i = 0; i < 32 * 2; i++)
        if (buf[i] != 0) count++;
    ok(count != 0, "expected != 0\n");
    IWICFormatConverter_Release(converter);

    /* not empty palette + MedianCut type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        palette, 0.0, WICBitmapPaletteTypeMedianCut);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);
    hr = IWICFormatConverter_CopyPalette(converter, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);
    count = 0xdeadbeef;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 16, "expected 16, got %u\n", count);
    hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    count = 0;
    for (i = 0; i < 32 * 2; i++)
        if (buf[i] != 0) count++;
    ok(count != 0, "expected != 0\n");
    IWICFormatConverter_Release(converter);

    /* NULL palette + MedianCut type */
    hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    ok(hr == S_OK, "CreateFormatConverter error %#lx\n", hr);
    hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
                                        &GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
                                        NULL, 0.0, WICBitmapPaletteTypeMedianCut);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* XP */, "Initialize error %#lx\n", hr);
    if (hr == S_OK)
    {
        hr = IWICFormatConverter_CopyPalette(converter, palette);
        ok(hr == S_OK, "CopyPalette error %#lx\n", hr);
        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette, &count);
        ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
        ok(count == 8, "expected 8, got %u\n", count);
        hr = IWICFormatConverter_CopyPixels(converter, NULL, 32, sizeof(buf), buf);
        ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
        count = 0;
        for (i = 0; i < 32 * 2; i++)
            if (buf[i] != 0) count++;
        ok(count != 0, "expected != 0\n");
    }
    IWICFormatConverter_Release(converter);

    IWICPalette_Release(palette);
    DeleteTestBitmap(src_obj);
}

START_TEST(converter)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "failed to create factory: %#lx\n", hr);

    test_conversion(&testdata_24bppRGB, &testdata_1bppIndexed, "24bppRGB -> 1bppIndexed", TRUE);
    test_conversion(&testdata_24bppRGB, &testdata_2bppIndexed, "24bppRGB -> 2bppIndexed", TRUE);
    test_conversion(&testdata_24bppRGB, &testdata_4bppIndexed, "24bppRGB -> 4bppIndexed", TRUE);
    test_conversion(&testdata_24bppRGB, &testdata_8bppIndexed, "24bppRGB -> 8bppIndexed", FALSE);

    test_conversion(&testdata_BlackWhite, &testdata_8bppIndexed_BW, "BlackWhite -> 8bppIndexed", TRUE);
    test_conversion(&testdata_1bppIndexed, &testdata_8bppIndexed_BW, "1bppIndexed -> 8bppIndexed", TRUE);
    test_conversion(&testdata_2bppIndexed, &testdata_8bppIndexed_4colors, "2bppIndexed -> 8bppIndexed", TRUE);
    test_conversion(&testdata_4bppIndexed, &testdata_8bppIndexed, "4bppIndexed -> 8bppIndexed", TRUE);

    test_conversion(&testdata_32bppBGRA, &testdata_32bppBGR, "BGRA -> BGR", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_32bppBGRA, "BGR -> BGRA", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_32bppBGRA, "BGRA -> BGRA", FALSE);
    test_conversion(&testdata_32bppBGRA80, &testdata_32bppPBGRA, "BGRA -> PBGRA", FALSE);

    test_conversion(&testdata_32bppRGBA, &testdata_32bppRGB, "RGBA -> RGB", FALSE);
    test_conversion(&testdata_32bppRGB, &testdata_32bppRGBA, "RGB -> RGBA", FALSE);
    test_conversion(&testdata_32bppRGBA, &testdata_32bppRGBA, "RGBA -> RGBA", FALSE);
    test_conversion(&testdata_32bppRGBA80, &testdata_32bppPRGBA, "RGBA -> PRGBA", FALSE);

    test_conversion(&testdata_24bppBGR, &testdata_24bppBGR, "24bppBGR -> 24bppBGR", FALSE);
    test_conversion(&testdata_24bppBGR, &testdata_24bppRGB, "24bppBGR -> 24bppRGB", FALSE);

    test_conversion(&testdata_24bppRGB, &testdata_24bppRGB, "24bppRGB -> 24bppRGB", FALSE);
    test_conversion(&testdata_24bppRGB, &testdata_24bppBGR, "24bppRGB -> 24bppBGR", FALSE);

    test_conversion(&testdata_32bppBGR, &testdata_24bppRGB, "32bppBGR -> 24bppRGB", FALSE);
    test_conversion(&testdata_24bppRGB, &testdata_32bppBGR, "24bppRGB -> 32bppBGR", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_24bppRGB, "32bppBGRA -> 24bppRGB", FALSE);
    test_conversion(&testdata_32bppRGBA, &testdata_24bppBGR, "32bppRGBA -> 24bppBGR", FALSE);

    test_conversion(&testdata_32bppRGBA, &testdata_32bppBGRA, "32bppRGBA -> 32bppBGRA", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_32bppRGBA, "32bppBGRA -> 32bppRGBA", FALSE);

    test_conversion(&testdata_64bppRGBA, &testdata_32bppRGBA, "64bppRGBA -> 32bppRGBA", FALSE);
    test_conversion(&testdata_64bppRGBA, &testdata_32bppRGB, "64bppRGBA -> 32bppRGB", FALSE);

    test_conversion(&testdata_24bppRGB, &testdata_32bppGrayFloat, "24bppRGB -> 32bppGrayFloat", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_32bppGrayFloat, "32bppBGR -> 32bppGrayFloat", FALSE);

    test_conversion(&testdata_24bppBGR, &testdata_8bppGray, "24bppBGR -> 8bppGray", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_8bppGray, "32bppBGR -> 8bppGray", FALSE);
    test_conversion(&testdata_32bppGrayFloat, &testdata_24bppBGR_gray, "32bppGrayFloat -> 24bppBGR gray", FALSE);
    test_conversion(&testdata_32bppGrayFloat, &testdata_8bppGray, "32bppGrayFloat -> 8bppGray", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_16bppBGRA5551, "32bppBGRA -> 16bppBGRA5551", FALSE);
    test_conversion(&testdata_48bppRGB, &testdata_64bppRGBA_2, "48bppRGB -> 64bppRGBA", FALSE);

    test_invalid_conversion();
    test_default_converter();
    test_converter_4bppGray();
    test_converter_8bppGray();
    test_converter_8bppIndexed();

    test_encoder(&testdata_8bppIndexed, &CLSID_WICGifEncoder,
                 &testdata_8bppIndexed, &CLSID_WICGifDecoder, "GIF encoder 8bppIndexed");

    test_encoder(&testdata_BlackWhite, &CLSID_WICPngEncoder,
                 &testdata_BlackWhite, &CLSID_WICPngDecoder, "PNG encoder BlackWhite");
    test_encoder(&testdata_1bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_1bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 1bppIndexed");
    test_encoder(&testdata_2bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_2bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 2bppIndexed");
    test_encoder(&testdata_4bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_4bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 4bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_8bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 8bppIndexed");
    test_encoder(&testdata_24bppBGR, &CLSID_WICPngEncoder,
                 &testdata_24bppBGR, &CLSID_WICPngDecoder, "PNG encoder 24bppBGR");
if (!strcmp(winetest_platform, "windows")) /* FIXME: enable once implemented in Wine */
{
    test_encoder(&testdata_32bppBGR, &CLSID_WICPngEncoder,
                 &testdata_24bppBGR, &CLSID_WICPngDecoder, "PNG encoder 32bppBGR");
}

    test_encoder(&testdata_BlackWhite, &CLSID_WICBmpEncoder,
                 &testdata_1bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder BlackWhite");
    test_encoder(&testdata_1bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_1bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 1bppIndexed");
    test_encoder(&testdata_2bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_4bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 2bppIndexed");
    test_encoder(&testdata_4bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_4bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 4bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_8bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 8bppIndexed");
    test_encoder(&testdata_32bppBGR, &CLSID_WICBmpEncoder,
                 &testdata_32bppBGR, &CLSID_WICBmpDecoder, "BMP encoder 32bppBGR");

    test_encoder(&testdata_BlackWhite, &CLSID_WICTiffEncoder,
                 &testdata_BlackWhite, &CLSID_WICTiffDecoder, "TIFF encoder BlackWhite");
    test_encoder(&testdata_1bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_1bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 1bppIndexed");
    test_encoder(&testdata_2bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_4bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 2bppIndexed");
    test_encoder(&testdata_4bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_4bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 4bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_8bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 8bppIndexed");
    test_encoder(&testdata_24bppBGR, &CLSID_WICTiffEncoder,
                 &testdata_24bppBGR, &CLSID_WICTiffDecoder, "TIFF encoder 24bppBGR");

    test_encoder(&testdata_24bppBGR, &CLSID_WICJpegEncoder,
                 &testdata_24bppBGR, NULL, "JPEG encoder 24bppBGR");

    test_multi_encoder(multiple_frames, &CLSID_WICTiffEncoder,
                       multiple_frames, &CLSID_WICTiffDecoder, NULL, NULL, "TIFF encoder multi-frame", NULL);

    test_encoder_rects();

    test_multi_encoder(single_frame, &CLSID_WICPngEncoder,
                       single_frame, &CLSID_WICPngDecoder, NULL, png_interlace_settings, "PNG encoder interlaced", NULL);

    IWICImagingFactory_Release(factory);

    CoUninitialize();
}
