/*
 * Copyright 2012 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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
#include <assert.h>
#include <math.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(IID_CMetaBitmapRenderTarget, 0x0ccd7824,0xdc16,0x4d09,0xbc,0xa8,0x6b,0x09,0xc4,0xef,0x55,0x35);

#ifndef IID_IMILBitmap
#include <initguid.h>
DEFINE_GUID(IID_IMILBitmap,0xb1784d3f,0x8115,0x4763,0x13,0xaa,0x32,0xed,0xdb,0x68,0x29,0x4a);
DEFINE_GUID(IID_IMILBitmapSource,0x7543696a,0xbc8d,0x46b0,0x5f,0x81,0x8d,0x95,0x72,0x89,0x72,0xbe);
DEFINE_GUID(IID_IMILBitmapLock,0xa67b2b53,0x8fa1,0x4155,0x8f,0x64,0x0c,0x24,0x7a,0x8f,0x84,0xcd);
DEFINE_GUID(IID_IMILBitmapScaler,0xa767b0f0,0x1c8c,0x4aef,0x56,0x8f,0xad,0xf9,0x6d,0xcf,0xd5,0xcb);
DEFINE_GUID(IID_IMILFormatConverter,0x7e2a746f,0x25c5,0x4851,0xb3,0xaf,0x44,0x3b,0x79,0x63,0x9e,0xc0);
DEFINE_GUID(IID_IMILPalette,0xca8e206f,0xf22c,0x4af7,0x6f,0xba,0x7b,0xed,0x5e,0xb1,0xc9,0x2f);
#else
extern IID IID_IMILBitmap;
extern IID IID_IMILBitmapSource;
extern IID IID_IMILBitmapLock;
extern IID IID_IMILBitmapScaler;
extern IID IID_IMILFormatConverter;
extern IID IID_IMILPalette;
#endif

#undef INTERFACE
#define INTERFACE IMILBitmapSource
DECLARE_INTERFACE_(IMILBitmapSource,IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
};

#undef INTERFACE
#define INTERFACE IMILBitmap
DECLARE_INTERFACE_(IMILBitmap,IMILBitmapSource)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
    /*** IMILBitmap methods ***/
    STDMETHOD_(HRESULT,unknown1)(THIS_ void **) PURE;
    STDMETHOD_(HRESULT,Lock)(THIS_ const WICRect *,DWORD,IWICBitmapLock **) PURE;
    STDMETHOD_(HRESULT,Unlock)(THIS_ IWICBitmapLock *) PURE;
    STDMETHOD_(HRESULT,SetPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,SetResolution)(THIS_ double,double) PURE;
    STDMETHOD_(HRESULT,AddDirtyRect)(THIS_ const WICRect *) PURE;
};

#undef INTERFACE
#define INTERFACE IMILBitmapScaler
DECLARE_INTERFACE_(IMILBitmapScaler,IMILBitmapSource)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
    /*** IMILBitmapScaler methods ***/
    STDMETHOD_(HRESULT,unknown1)(THIS_ void **) PURE;
    STDMETHOD_(HRESULT,Initialize)(THIS_ IMILBitmapSource *,UINT,UINT,WICBitmapInterpolationMode);
};

static IWICImagingFactory *factory;

static HRESULT WINAPI bitmapsource_QueryInterface(IWICBitmapSource *iface, REFIID iid, void **ppv)
{
    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI bitmapsource_AddRef(IWICBitmapSource *iface)
{
    return 2;
}

static ULONG WINAPI bitmapsource_Release(IWICBitmapSource *iface)
{
    return 1;
}

static HRESULT WINAPI bitmapsource_GetSize(IWICBitmapSource *iface, UINT *width, UINT *height)
{
    *width = *height = 10;
    return S_OK;
}

static HRESULT WINAPI bitmapsource_GetPixelFormat(IWICBitmapSource *iface,
    WICPixelFormatGUID *format)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI bitmapsource_GetResolution(IWICBitmapSource *iface,
    double *dpiX, double *dpiY)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI bitmapsource_CopyPalette(IWICBitmapSource *iface,
    IWICPalette *palette)
{
    return E_NOTIMPL;
}

static WICRect g_rect;
static BOOL called_CopyPixels;

static HRESULT WINAPI bitmapsource_CopyPixels(IWICBitmapSource *iface,
    const WICRect *rc, UINT stride, UINT buffer_size, BYTE *buffer)
{
    if (rc) g_rect = *rc;
    called_CopyPixels = TRUE;
    return S_OK;
}

static const IWICBitmapSourceVtbl sourcevtbl = {
    bitmapsource_QueryInterface,
    bitmapsource_AddRef,
    bitmapsource_Release,
    bitmapsource_GetSize,
    bitmapsource_GetPixelFormat,
    bitmapsource_GetResolution,
    bitmapsource_CopyPalette,
    bitmapsource_CopyPixels
};

static IWICBitmapSource bitmapsource = { &sourcevtbl };

static HBITMAP create_dib(int width, int height, int bpp, LOGPALETTE *pal, const void *data)
{
    char bmibuf[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255];
    BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;
    void *bits;
    HBITMAP hdib;
    BITMAP bm;

    memset(bmibuf, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biHeight = -height;
    bmi->bmiHeader.biBitCount = bpp;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    if (pal)
    {
        WORD i;

        assert(pal->palNumEntries <= 256);
        for (i = 0; i < pal->palNumEntries; i++)
        {
            bmi->bmiColors[i].rgbRed = pal->palPalEntry[i].peRed;
            bmi->bmiColors[i].rgbGreen = pal->palPalEntry[i].peGreen;
            bmi->bmiColors[i].rgbBlue = pal->palPalEntry[i].peBlue;
            bmi->bmiColors[i].rgbReserved = 0;
        }

        bmi->bmiHeader.biClrUsed = pal->palNumEntries;
        bmi->bmiHeader.biClrImportant = pal->palNumEntries;
    }
    hdib = CreateDIBSection(0, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(hdib != 0, "CreateDIBSection(%dx%d,%d bpp) failed\n", width, height, bpp);

    GetObjectW(hdib, sizeof(bm), &bm);
    ok(bm.bmWidth == width, "expected %d, got %ld\n", width, bm.bmWidth);
    ok(bm.bmHeight == height, "expected %d, got %ld\n", height, bm.bmHeight);
    ok(bm.bmPlanes == 1, "expected 1, got %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == bpp, "expected %d, got %d\n", bpp, bm.bmBitsPixel);

    if (data) memcpy(bits, data, bm.bmWidthBytes * bm.bmHeight);

    return hdib;
}

static void test_createbitmap(void)
{
    HRESULT hr;
    IWICBitmap *bitmap;
    IWICPalette *palette;
    IWICBitmapLock *lock, *lock2;
    WICBitmapPaletteType palettetype;
    int i;
    WICRect rc;
    const BYTE bitmap_data[27] = {
        128,128,255, 128,128,128, 128,255,128,
        128,128,128, 128,128,128, 255,255,255,
        255,128,128, 255,255,255, 255,255,255};
    BYTE returned_data[27] = {0};
    BYTE *lock_buffer=NULL, *base_lock_buffer=NULL;
    UINT lock_buffer_size=0;
    UINT lock_buffer_stride=0;
    WICPixelFormatGUID pixelformat = {0};
    UINT width=0, height=0;
    double dpix=10.0, dpiy=10.0;
    int can_lock_null = 1;

    hr = IWICImagingFactory_CreateBitmap(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
        WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%lx\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%lx\n", hr);

    /* Palette is unavailable until explicitly set */
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "IWICBitmap_CopyPalette failed hr=%lx\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%lx\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%lx\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray4, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%lx\n", hr);

    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_CopyPalette failed hr=%lx\n", hr);

    hr = IWICPalette_GetType(palette, &palettetype);
    ok(hr == S_OK, "IWICPalette_GetType failed hr=%lx\n", hr);
    ok(palettetype == WICBitmapPaletteTypeFixedGray256,
        "expected WICBitmapPaletteTypeFixedGray256, got %x\n", palettetype);

    IWICPalette_Release(palette);

    /* pixel data is initially zeroed */
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 9, 27, returned_data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%lx\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == 0, "returned_data[%i] == %i\n", i, returned_data[i]);

    /* Invalid lock rects */
    rc.X = rc.Y = 0;
    rc.Width = 4;
    rc.Height = 3;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.Width = 3;
    rc.Height = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.Height = 3;
    rc.X = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.X = 0;
    rc.Y = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    /* NULL lock rect */
    hr = IWICBitmap_Lock(bitmap, NULL, WICBitmapLockRead, &lock);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* winxp */, "IWICBitmap_Lock failed hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        /* entire bitmap is locked */
        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%lx\n", hr);
        ok(width == 3, "got %d, expected 3\n", width);
        ok(height == 3, "got %d, expected 3\n", height);

        IWICBitmapLock_Release(lock);
    }
    else
        can_lock_null = 0;

    /* lock with a valid rect */
    rc.Y = 0;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%lx\n", hr);
        /* stride is divisible by 4 */
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%lx\n", hr);
        /* buffer size does not include padding from the last row */
        ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
        ok(lock_buffer != NULL, "got NULL data pointer\n");
        base_lock_buffer = lock_buffer;

        hr = IWICBitmapLock_GetPixelFormat(lock, &pixelformat);
        ok(hr == S_OK, "IWICBitmapLock_GetPixelFormat failed hr=%lx\n", hr);
        ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%lx\n", hr);
        ok(width == 3, "got %d, expected 3\n", width);
        ok(height == 3, "got %d, expected 3\n", height);

        /* We can have multiple simultaneous read locks */
        hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock2);
        ok(hr == S_OK, "IWICBitmap_Lock failed hr=%lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapLock_GetDataPointer(lock2, &lock_buffer_size, &lock_buffer);
            ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%lx\n", hr);
            ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
            ok(lock_buffer == base_lock_buffer, "got %p, expected %p\n", lock_buffer, base_lock_buffer);

            IWICBitmapLock_Release(lock2);
        }

        if (can_lock_null) /* this hangs on xp/vista */
        {
            /* But not a read and a write lock */
            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%lx\n", hr);
        }

        /* But we don't need a write lock to write */
        if (base_lock_buffer)
        {
            for (i=0; i<3; i++)
                memcpy(base_lock_buffer + lock_buffer_stride*i, bitmap_data + i*9, 9);
        }

        IWICBitmapLock_Release(lock);
    }

    /* test that the data we wrote is returned by CopyPixels */
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 9, 27, returned_data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%lx\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == bitmap_data[i], "returned_data[%i] == %i\n", i, returned_data[i]);

    /* try a valid partial rect, and write mode */
    rc.X = 2;
    rc.Y = 0;
    rc.Width = 1;
    rc.Height = 2;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        if (can_lock_null) /* this hangs on xp/vista */
        {
            /* Can't lock again while locked for writing */
            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%lx\n", hr);

            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%lx\n", hr);
        }

        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%lx\n", hr);
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%lx\n", hr);
        ok(lock_buffer_size == 15, "got %i, expected 15\n", lock_buffer_size);
        ok(lock_buffer == base_lock_buffer+6, "got %p, expected %p+6\n", lock_buffer, base_lock_buffer);

        hr = IWICBitmapLock_GetPixelFormat(lock, &pixelformat);
        ok(hr == S_OK, "IWICBitmapLock_GetPixelFormat failed hr=%lx\n", hr);
        ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%lx\n", hr);
        ok(width == 1, "got %d, expected 1\n", width);
        ok(height == 2, "got %d, expected 2\n", height);

        IWICBitmapLock_Release(lock);
    }

    hr = IWICBitmap_GetPixelFormat(bitmap, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%lx\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%lx\n", hr);
    ok(dpix == 0.0, "got %f, expected 0.0\n", dpix);
    ok(dpiy == 0.0, "got %f, expected 0.0\n", dpiy);

    hr = IWICBitmap_SetResolution(bitmap, 12.0, 34.0);
    ok(hr == S_OK, "IWICBitmap_SetResolution failed hr=%lx\n", hr);

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%lx\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%lx\n", hr);
    ok(width == 3, "got %d, expected 3\n", width);
    ok(height == 3, "got %d, expected 3\n", height);

    IWICBitmap_Release(bitmap);
}

static void test_createbitmapfromsource(void)
{
    HRESULT hr;
    IWICBitmap *bitmap, *bitmap2;
    IWICPalette *palette;
    IWICBitmapLock *lock;
    int i;
    WICRect rc;
    const BYTE bitmap_data[27] = {
        128,128,255, 128,128,128, 128,255,128,
        128,128,128, 128,128,128, 255,255,255,
        255,128,128, 255,255,255, 255,255,255};
    BYTE returned_data[27] = {0};
    BYTE *lock_buffer=NULL;
    UINT lock_buffer_stride=0;
    UINT lock_buffer_size=0;
    WICPixelFormatGUID pixelformat = {0};
    UINT width=0, height=0;
    double dpix=10.0, dpiy=10.0;
    UINT count;
    WICBitmapPaletteType palette_type;

    hr = IWICImagingFactory_CreateBitmap(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
        WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%lx\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%lx\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%lx\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%lx\n", hr);

    IWICPalette_Release(palette);

    rc.X = rc.Y = 0;
    rc.Width = 3;
    rc.Height = 3;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%lx\n", hr);
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%lx\n", hr);
        ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
        ok(lock_buffer != NULL, "got NULL data pointer\n");

        for (i=0; i<3; i++)
            memcpy(lock_buffer + lock_buffer_stride*i, bitmap_data + i*9, 9);

        IWICBitmapLock_Release(lock);
    }

    hr = IWICBitmap_SetResolution(bitmap, 12.0, 34.0);
    ok(hr == S_OK, "IWICBitmap_SetResolution failed hr=%lx\n", hr);

    /* WICBitmapNoCache */
    hr = IWICImagingFactory_CreateBitmapFromSource(factory, (IWICBitmapSource *)bitmap,
        WICBitmapNoCache, &bitmap2);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromSource failed hr=%lx\n", hr);
    ok(bitmap2 == bitmap, "Unexpected bitmap instance.\n");

    IWICBitmap_Release(bitmap2);

    bitmap2 = (void *)0xdeadbeef;
    hr = IWICImagingFactory_CreateBitmapFromSource(factory, &bitmapsource, WICBitmapNoCache, &bitmap2);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(bitmap2 == (void *)0xdeadbeef, "Unexpected pointer %p.\n", bitmap2);

    hr = IWICImagingFactory_CreateBitmapFromSource(factory, (IWICBitmapSource*)bitmap,
        WICBitmapCacheOnLoad, &bitmap2);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromSource failed hr=%lx\n", hr);

    IWICBitmap_Release(bitmap);

    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%lx\n", hr);

    /* palette isn't copied for non-indexed formats? */
    hr = IWICBitmap_CopyPalette(bitmap2, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "IWICBitmap_CopyPalette failed hr=%lx\n", hr);

    IWICPalette_Release(palette);

    hr = IWICBitmap_CopyPixels(bitmap2, NULL, 9, 27, returned_data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%lx\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == bitmap_data[i], "returned_data[%i] == %i\n", i, returned_data[i]);

    hr = IWICBitmap_GetPixelFormat(bitmap2, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%lx\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

    hr = IWICBitmap_GetResolution(bitmap2, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%lx\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = IWICBitmap_GetSize(bitmap2, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%lx\n", hr);
    ok(width == 3, "got %d, expected 3\n", width);
    ok(height == 3, "got %d, expected 3\n", height);

    IWICBitmap_Release(bitmap2);

    /* Ensure palette is copied for indexed formats */
    hr = IWICImagingFactory_CreateBitmap(factory, 3, 3, &GUID_WICPixelFormat4bppIndexed,
        WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%lx\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%lx\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%lx\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%lx\n", hr);

    IWICPalette_Release(palette);

    hr = IWICImagingFactory_CreateBitmapFromSource(factory, (IWICBitmapSource*)bitmap,
        WICBitmapCacheOnLoad, &bitmap2);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromSource failed hr=%lx\n", hr);

    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%lx\n", hr);

    hr = IWICBitmap_CopyPalette(bitmap2, palette);
    ok(hr == S_OK, "IWICBitmap_CopyPalette failed hr=%lx\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "IWICPalette_GetColorCount failed hr=%lx\n", hr);
    ok(count == 256, "unexpected count %d\n", count);

    hr = IWICPalette_GetType(palette, &palette_type);
    ok(hr == S_OK, "IWICPalette_GetType failed hr=%lx\n", hr);
    ok(palette_type == WICBitmapPaletteTypeFixedGray256, "unexpected palette type %d\n", palette_type);

    IWICPalette_Release(palette);

    hr = IWICBitmap_GetPixelFormat(bitmap2, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%lx\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat4bppIndexed), "unexpected pixel format\n");

    hr = IWICBitmap_GetSize(bitmap2, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%lx\n", hr);
    ok(width == 3, "got %d, expected 3\n", width);
    ok(height == 3, "got %d, expected 3\n", height);

    /* CreateBitmapFromSourceRect */
    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 0, 0, 16, 32, &bitmap);
    ok(hr == S_OK, "Failed to create a bitmap, hr %#lx.\n", hr);
    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "Failed to get bitmap size, hr %#lx.\n", hr);
    ok(width == 3, "Unexpected width %u.\n", width);
    ok(height == 3, "Unexpected height %u.\n", height);
    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 0, 0, 1, 1, &bitmap);
    ok(hr == S_OK, "Failed to create a bitmap, hr %#lx.\n", hr);
    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "Failed to get bitmap size, hr %#lx.\n", hr);
    ok(width == 1, "Unexpected width %u.\n", width);
    ok(height == 1, "Unexpected height %u.\n", height);
    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 2, 1, 16, 32, &bitmap);
    ok(hr == S_OK, "Failed to create a bitmap, hr %#lx.\n", hr);
    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "Failed to get bitmap size, hr %#lx.\n", hr);
    ok(width == 1, "Unexpected width %u.\n", width);
    ok(height == 2, "Unexpected height %u.\n", height);
    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 0, 0, 0, 2, &bitmap);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 0, 0, 2, 0, &bitmap);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 1, 3, 16, 32, &bitmap);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromSourceRect(factory, (IWICBitmapSource *)bitmap2, 3, 1, 16, 32, &bitmap);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IWICBitmap_Release(bitmap2);
}

static void test_CreateBitmapFromMemory(void)
{
    BYTE orig_data3x3[27] = {
        128,128,255, 128,128,128, 128,255,128,
        128,128,128, 128,128,128, 255,255,255,
        255,128,128, 255,255,255, 255,255,255 };
    BYTE data3x3[27];
    BYTE data3x2[27] = {
        128,128,255, 128,128,128, 128,255,128,
        0,0,0, 0,128,128, 255,255,255,
        255,128,128, 255,0,0, 0,0,0 };
    BYTE data[27];
    HRESULT hr;
    IWICBitmap *bitmap;
    UINT width, height, i;

    memcpy(data3x3, orig_data3x3, sizeof(data3x3));

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   0, 0, NULL, &bitmap);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   0, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   6, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   12, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "expected WINCODEC_ERR_INSUFFICIENTBUFFER, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   9, sizeof(data3x3) - 1, data3x3, &bitmap);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "expected WINCODEC_ERR_INSUFFICIENTBUFFER, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   9, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromMemory error %#lx\n", hr);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    data3x3[2] = 192;

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 9, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#lx\n", hr);
    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == orig_data3x3[i], "%u: expected %u, got %u\n", i, data[i], data3x3[i]);

    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 2, &GUID_WICPixelFormat24bppBGR,
                                                   13, sizeof(orig_data3x3), orig_data3x3, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromMemory error %#lx\n", hr);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 2, "expected 2, got %u\n", height);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 13, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#lx\n", hr);
    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == data3x2[i], "%u: expected %u, got %u\n", i, data3x2[i], data[i]);

    IWICBitmap_Release(bitmap);
}

static void test_CreateBitmapFromHICON(void)
{
    static const char bits[4096];
    HICON icon;
    ICONINFO info;
    HRESULT hr;
    IWICBitmap *bitmap;
    UINT width, height;
    WICPixelFormatGUID format;

    /* 1 bpp mask */
    info.fIcon = 1;
    info.xHotspot = 0;
    info.yHotspot = 0;
    info.hbmColor = 0;
    info.hbmMask = CreateBitmap(16, 32, 1, 1, bits);
    ok(info.hbmMask != 0, "CreateBitmap failed\n");
    icon = CreateIconIndirect(&info);
    ok(icon != 0, "CreateIconIndirect failed\n");
    DeleteObject(info.hbmMask);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, 0, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, 0, &bitmap);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_CURSOR_HANDLE), "expected ERROR_INVALID_CURSOR_HANDLE, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, icon, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, icon, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHICON error %#lx\n", hr);
    DestroyIcon(icon);
    if (hr != S_OK) return;

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 16, "expected 16, got %u\n", width);
    ok(height == 16, "expected 16, got %u\n", height);

    IWICBitmap_Release(bitmap);

    /* 24 bpp color, 1 bpp mask */
    info.fIcon = 1;
    info.xHotspot = 0;
    info.yHotspot = 0;
    info.hbmColor = CreateBitmap(16, 16, 1, 24, bits);
    ok(info.hbmColor != 0, "CreateBitmap failed\n");
    info.hbmMask = CreateBitmap(16, 16, 1, 1, bits);
    ok(info.hbmMask != 0, "CreateBitmap failed\n");
    icon = CreateIconIndirect(&info);
    ok(icon != 0, "CreateIconIndirect failed\n");
    DeleteObject(info.hbmColor);
    DeleteObject(info.hbmMask);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, icon, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHICON error %#lx\n", hr);
    DestroyIcon(icon);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 16, "expected 16, got %u\n", width);
    ok(height == 16, "expected 16, got %u\n", height);

    IWICBitmap_Release(bitmap);
}

static void test_CreateBitmapFromHBITMAP(void)
{
    /* 8 bpp data must be aligned to a DWORD boundary for a DIB */
    static const BYTE data_8bpp_pal_dib[12] = { 0,1,2,0, 1,2,0,0, 2,1,0,0 };
    static const BYTE data_8bpp_rgb_dib[12] = { 0xf0,0x0f,0xff,0, 0x0f,0xff,0xf0,0, 0xf0,0x0f,0xff,0 };
    static const BYTE data_8bpp_pal_wic[12] = { 0xd,0xe,0x10,0, 0xe,0x10,0xd,0, 0x10,0xe,0xd,0 };
    static const PALETTEENTRY pal_data[3] = { {0xff,0,0,0}, {0,0xff,0,0}, {0,0,0xff,0} };
    char pal_buf[sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * 255];
    LOGPALETTE *pal = (LOGPALETTE *)pal_buf;
    HBITMAP hbmp;
    HPALETTE hpal;
    BYTE data[12];
    HRESULT hr;
    IWICBitmap *bitmap;
    UINT width, height, i, count;
    WICPixelFormatGUID format;
    IWICPalette *palette;
    WICBitmapPaletteType type;

    /* 8 bpp without palette */
    hbmp = create_dib(3, 3, 8, NULL, data_8bpp_rgb_dib);
    ok(hbmp != 0, "failed to create bitmap\n");

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, 0, 0, WICBitmapIgnoreAlpha, &bitmap);
    ok(hr == WINCODEC_ERR_WIN32ERROR || hr == 0x88980003 /*XP*/, "expected WINCODEC_ERR_WIN32ERROR, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, 0, WICBitmapIgnoreAlpha, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, 0, WICBitmapIgnoreAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 4, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#lx\n", hr);
    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == data_8bpp_rgb_dib[i], "%u: expected %#x, got %#x\n", i, data_8bpp_rgb_dib[i], data[i]);

    IWICBitmap_Release(bitmap);
    DeleteObject(hbmp);

    /* 8 bpp with a 3 entries palette */
    memset(pal_buf, 0, sizeof(pal_buf));
    pal->palVersion = 0x300;
    pal->palNumEntries = 3;
    memcpy(pal->palPalEntry, pal_data, sizeof(pal_data));
    hpal = CreatePalette(pal);
    ok(hpal != 0, "CreatePalette failed\n");

    hbmp = create_dib(3, 3, 8, pal, data_8bpp_pal_dib);
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, hpal, WICBitmapIgnoreAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    todo_wine
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat4bppIndexed),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);

    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "%u: GetType error %#lx\n", i, hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    todo_wine
    ok(count == 16, "expected 16, got %u\n", count);

    IWICPalette_Release(palette);

    IWICBitmap_Release(bitmap);
    DeleteObject(hbmp);
    DeleteObject(hpal);

    /* 8 bpp with a 256 entries palette */
    memset(pal_buf, 0, sizeof(pal_buf));
    pal->palVersion = 0x300;
    pal->palNumEntries = 256;
    memcpy(pal->palPalEntry, pal_data, sizeof(pal_data));
    hpal = CreatePalette(pal);
    ok(hpal != 0, "CreatePalette failed\n");

    hbmp = create_dib(3, 3, 8, pal, data_8bpp_pal_dib);
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, hpal, WICBitmapIgnoreAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
            "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#lx\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);

    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "%u: GetType error %#lx\n", i, hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 256, "expected 256, got %u\n", count);

    IWICPalette_Release(palette);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 4, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#lx\n", hr);
    for (i = 0; i < sizeof(data); i++)
        todo_wine_if (data[i] != data_8bpp_pal_wic[i])
            ok(data[i] == data_8bpp_pal_wic[i], "%u: expected %#x, got %#x\n", i, data_8bpp_pal_wic[i], data[i]);

    IWICBitmap_Release(bitmap);
    DeleteObject(hbmp);
    DeleteObject(hpal);

    /* 32bpp alpha */
    hbmp = create_dib(2, 2, 32, NULL, NULL);
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, NULL, WICBitmapUseAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    hr = IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    IWICBitmap_Release(bitmap);

    /* 32bpp pre-multiplied alpha */
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, NULL, WICBitmapUsePremultipliedAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    hr = IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppPBGRA),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    IWICBitmap_Release(bitmap);

    /* 32bpp no alpha */
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, NULL, WICBitmapIgnoreAlpha, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#lx\n", hr);

    hr = IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGR),
       "unexpected pixel format %s\n", wine_dbgstr_guid(&format));

    IWICBitmap_Release(bitmap);
    DeleteObject(hbmp);
}

static void test_clipper(void)
{
    IWICBitmapClipper *clipper;
    UINT height, width;
    IWICBitmap *bitmap;
    BYTE buffer[500];
    WICRect rect;
    HRESULT hr;

    hr = IWICImagingFactory_CreateBitmap(factory, 10, 10, &GUID_WICPixelFormat24bppBGR,
        WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IWICImagingFactory_CreateBitmapClipper(factory, &clipper);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    rect.X = rect.Y = 0;
    rect.Width = rect.Height = 11;
    hr = IWICBitmapClipper_Initialize(clipper, (IWICBitmapSource*)bitmap, &rect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    rect.X = rect.Y = 5;
    rect.Width = rect.Height = 6;
    hr = IWICBitmapClipper_Initialize(clipper, (IWICBitmapSource*)bitmap, &rect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    rect.X = rect.Y = 5;
    rect.Width = rect.Height = 5;
    hr = IWICBitmapClipper_Initialize(clipper, (IWICBitmapSource*)bitmap, &rect);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    width = height = 0;
    hr = IWICBitmapClipper_GetSize(clipper, &width, &height);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(width == 5, "got %d\n", width);
    ok(height == 5, "got %d\n", height);

    IWICBitmapClipper_Release(clipper);
    IWICBitmap_Release(bitmap);

    /* CopyPixels */
    hr = IWICImagingFactory_CreateBitmapClipper(factory, &clipper);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    rect.X = rect.Y = 5;
    rect.Width = rect.Height = 5;
    hr = IWICBitmapClipper_Initialize(clipper, &bitmapsource, &rect);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    rect.X = rect.Y = 0;
    rect.Width = rect.Height = 2;

    /* passed rectangle is relative to clipper rectangle, underlying source gets intersected
       rectangle */
    memset(&g_rect, 0, sizeof(g_rect));
    called_CopyPixels = FALSE;
    hr = IWICBitmapClipper_CopyPixels(clipper, &rect, 0, sizeof(buffer), buffer);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(called_CopyPixels, "CopyPixels not called\n");
    ok(g_rect.X == 5 && g_rect.Y == 5 && g_rect.Width == 2 && g_rect.Height == 2,
        "got wrong rectangle (%d,%d)-(%d,%d)\n", g_rect.X, g_rect.Y, g_rect.Width, g_rect.Height);

    /* whole clipping rectangle */
    memset(&g_rect, 0, sizeof(g_rect));
    called_CopyPixels = FALSE;

    rect.X = rect.Y = 0;
    rect.Width = rect.Height = 5;

    hr = IWICBitmapClipper_CopyPixels(clipper, &rect, 0, sizeof(buffer), buffer);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(called_CopyPixels, "CopyPixels not called\n");
    ok(g_rect.X == 5 && g_rect.Y == 5 && g_rect.Width == 5 && g_rect.Height == 5,
        "got wrong rectangle (%d,%d)-(%d,%d)\n", g_rect.X, g_rect.Y, g_rect.Width, g_rect.Height);

    /* larger than clipping rectangle */
    memset(&g_rect, 0, sizeof(g_rect));
    called_CopyPixels = FALSE;

    rect.X = rect.Y = 0;
    rect.Width = rect.Height = 20;

    hr = IWICBitmapClipper_CopyPixels(clipper, &rect, 0, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(!called_CopyPixels, "CopyPixels called\n");

    rect.X = rect.Y = 5;
    rect.Width = rect.Height = 5;

    hr = IWICBitmapClipper_CopyPixels(clipper, &rect, 0, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(!called_CopyPixels, "CopyPixels called\n");

    /* null rectangle */
    memset(&g_rect, 0, sizeof(g_rect));
    called_CopyPixels = FALSE;

    hr = IWICBitmapClipper_CopyPixels(clipper, NULL, 0, sizeof(buffer), buffer);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(called_CopyPixels, "CopyPixels not called\n");
    ok(g_rect.X == 5 && g_rect.Y == 5 && g_rect.Width == 5 && g_rect.Height == 5,
        "got wrong rectangle (%d,%d)-(%d,%d)\n", g_rect.X, g_rect.Y, g_rect.Width, g_rect.Height);

    IWICBitmapClipper_Release(clipper);
}

static HRESULT (WINAPI *pWICCreateBitmapFromSectionEx)
    (UINT, UINT, REFWICPixelFormatGUID, HANDLE, UINT, UINT, WICSectionAccessLevel, IWICBitmap **);

static void test_WICCreateBitmapFromSectionEx(void)
{
    SYSTEM_INFO sysinfo;
    HANDLE hsection;
    BITMAPINFO info;
    void *bits;
    HBITMAP hdib;
    IWICBitmap *bitmap;
    HRESULT hr;
    pWICCreateBitmapFromSectionEx =
        (void *)GetProcAddress(LoadLibraryA("windowscodecs"), "WICCreateBitmapFromSectionEx");

    if (!pWICCreateBitmapFromSectionEx)
    {
        win_skip("WICCreateBitmapFromSectionEx not available\n");
        return;
    }

    GetSystemInfo(&sysinfo);
    hsection = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                  sysinfo.dwAllocationGranularity * 2, NULL);
    ok(hsection != NULL, "CreateFileMapping failed %lu\n", GetLastError());

    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize        = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth       = 3;
    info.bmiHeader.biHeight      = -3;
    info.bmiHeader.biBitCount    = 24;
    info.bmiHeader.biPlanes      = 1;
    info.bmiHeader.biCompression = BI_RGB;

    hdib = CreateDIBSection(0, &info, DIB_RGB_COLORS, &bits, hsection, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");

    hr = pWICCreateBitmapFromSectionEx(3, 3, &GUID_WICPixelFormat24bppBGR, hsection, 0, 0,
                                       WICSectionAccessLevelReadWrite, &bitmap);
    ok(hr == S_OK, "WICCreateBitmapFromSectionEx returned %#lx\n", hr);
    IWICBitmap_Release(bitmap);

    /* non-zero offset, smaller than allocation granularity */
    hr = pWICCreateBitmapFromSectionEx(3, 3, &GUID_WICPixelFormat24bppBGR, hsection, 0, 0x100,
                                       WICSectionAccessLevelReadWrite, &bitmap);
    ok(hr == S_OK, "WICCreateBitmapFromSectionEx returned %#lx\n", hr);
    IWICBitmap_Release(bitmap);

    /* offset larger than allocation granularity */
    hr = pWICCreateBitmapFromSectionEx(3, 3, &GUID_WICPixelFormat24bppBGR, hsection, 0,
                                       sysinfo.dwAllocationGranularity + 1,
                                       WICSectionAccessLevelReadWrite, &bitmap);
    ok(hr == S_OK, "WICCreateBitmapFromSectionEx returned %#lx\n", hr);
    IWICBitmap_Release(bitmap);
    DeleteObject(hdib);
    CloseHandle(hsection);
}

static void test_bitmap_scaler(void)
{
    WICPixelFormatGUID pixel_format;
    IWICBitmapScaler *scaler;
    IWICPalette *palette;
    double res_x, res_y;
    IWICBitmap *bitmap;
    UINT width, height;
    BYTE buf[93];  /* capable of holding a 7*4px, 24bpp image with stride 24 -> buffer size = 3*24+21 */
    HRESULT hr;

    hr = IWICImagingFactory_CreateBitmap(factory, 4, 2, &GUID_WICPixelFormat24bppBGR, WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "Failed to create a bitmap, hr %#lx.\n", hr);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "Failed to get bitmap size, hr %#lx.\n", hr);
    ok(width == 4, "Unexpected width %u.\n", width);
    ok(height == 2, "Unexpected height %u.\n", height);

    hr = IWICBitmap_GetResolution(bitmap, &res_x, &res_y);
    ok(hr == S_OK, "Failed to get bitmap resolution, hr %#lx.\n", hr);
    ok(res_x == 0.0 && res_y == 0.0, "Unexpected resolution %f x %f.\n", res_x, res_y);

    hr = IWICImagingFactory_CreateBitmapScaler(factory, &scaler);
    ok(hr == S_OK, "Failed to create bitmap scaler, hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, NULL, 0, 0,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 0, 0,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, NULL, &height);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, &width, NULL);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetResolution(scaler, NULL, NULL);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    res_x = 0.1;
    hr = IWICBitmapScaler_GetResolution(scaler, &res_x, NULL);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);
    ok(res_x == 0.1, "Unexpected resolution %f.\n", res_x);

    hr = IWICBitmapScaler_GetResolution(scaler, NULL, &res_y);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetResolution(scaler, &res_x, &res_y);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetPixelFormat(scaler, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&pixel_format, 0, sizeof(pixel_format));
    hr = IWICBitmapScaler_GetPixelFormat(scaler, &pixel_format);
    ok(hr == S_OK, "Failed to get pixel format, hr %#lx.\n", hr);
    ok(IsEqualGUID(&pixel_format, &GUID_WICPixelFormatDontCare), "Unexpected pixel format %s.\n",
        wine_dbgstr_guid(&pixel_format));

    width = 123;
    height = 321;
    hr = IWICBitmapScaler_GetSize(scaler, &width, &height);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);
    ok(width == 123, "Unexpected width %u.\n", width);
    ok(height == 321, "Unexpected height %u.\n", height);

    hr = IWICBitmapScaler_CopyPalette(scaler, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "Failed to create a palette, hr %#lx.\n", hr);
    hr = IWICBitmapScaler_CopyPalette(scaler, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Unexpected hr %#lx.\n", hr);
    IWICPalette_Release(palette);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 4, 0,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, &width, &height);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_CopyPixels(scaler, NULL, 1, sizeof(buf), buf);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 0, 2,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, &width, &height);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, NULL, 8, 4,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Failed to initialize bitmap scaler, hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 7, 4,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == S_OK, "Failed to initialize bitmap scaler, hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 0, 4,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 7, 0,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, NULL, 8, 4, WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_Initialize(scaler, (IWICBitmapSource *)bitmap, 7, 4,
        WICBitmapInterpolationModeNearestNeighbor);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, &width, &height);
    ok(hr == S_OK, "Failed to get scaler size, hr %#lx.\n", hr);
    ok(width == 7, "Unexpected width %u.\n", width);
    ok(height == 4, "Unexpected height %u.\n", height);

    hr = IWICBitmapScaler_GetSize(scaler, NULL, &height);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, &width, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetSize(scaler, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICBitmapScaler_GetPixelFormat(scaler, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&pixel_format, 0, sizeof(pixel_format));
    hr = IWICBitmapScaler_GetPixelFormat(scaler, &pixel_format);
    ok(hr == S_OK, "Failed to get pixel format, hr %#lx.\n", hr);
    ok(IsEqualGUID(&pixel_format, &GUID_WICPixelFormat24bppBGR), "Unexpected pixel format %s.\n",
        wine_dbgstr_guid(&pixel_format));

    hr = IWICBitmapScaler_GetResolution(scaler, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    res_x = 0.1;
    hr = IWICBitmapScaler_GetResolution(scaler, &res_x, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(res_x == 0.1, "Unexpected resolution %f.\n", res_x);

    hr = IWICBitmapScaler_GetResolution(scaler, NULL, &res_y);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    res_x = res_y = 1.0;
    hr = IWICBitmapScaler_GetResolution(scaler, &res_x, &res_y);
    ok(hr == S_OK, "Failed to get scaler resolution, hr %#lx.\n", hr);
    ok(res_x == 0.0 && res_y == 0.0, "Unexpected resolution %f x %f.\n", res_x, res_y);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "Failed to create a palette, hr %#lx.\n", hr);
    hr = IWICBitmapScaler_CopyPalette(scaler, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Unexpected hr %#lx.\n", hr);
    IWICPalette_Release(palette);

    hr = IWICBitmapScaler_CopyPixels(scaler, NULL, /*cbStride=*/24, sizeof(buf), buf);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IWICBitmapScaler_Release(scaler);

    IWICBitmap_Release(bitmap);
}

static LONG obj_refcount(void *obj)
{
    IUnknown_AddRef((IUnknown *)obj);
    return IUnknown_Release((IUnknown *)obj);
}

static void test_IMILBitmap(void)
{
    HRESULT hr;
    IWICBitmap *bitmap;
    IWICBitmapScaler *scaler;
    IMILBitmap *mil_bitmap;
    IMILBitmapSource *mil_source;
    IMILBitmapScaler *mil_scaler;
    IUnknown *wic_unknown, *mil_unknown;
    WICPixelFormatGUID format;
    int MIL_format;
    UINT width, height;
    double dpix, dpiy;
    BYTE buf[256];

    /* Bitmap */
    hr = IWICImagingFactory_CreateBitmap(factory, 1, 1, &GUID_WICPixelFormat24bppBGR,
                                         WICBitmapCacheOnDemand, &bitmap);
    ok(hr == S_OK, "CreateBitmap error %#lx\n", hr);

    ok(obj_refcount(bitmap) == 1, "ref count %ld\n", obj_refcount(bitmap));

    hr = IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat24bppBGR), "wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpix == 0.0, "got %f, expected 0.0\n", dpix);
    ok(dpiy == 0.0, "got %f, expected 0.0\n", dpiy);

    hr = IWICBitmap_SetResolution(bitmap, 12.0, 34.0);
    ok(hr == S_OK, "SetResolution error %#lx\n", hr);

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 1, "got %u, expected 1\n", width);
    ok(height == 1, "got %u, expected 1\n", height);

    hr = IWICBitmap_QueryInterface(bitmap, &IID_IMILBitmap, (void **)&mil_bitmap);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    ok(obj_refcount(bitmap) == 2, "ref count %ld\n", obj_refcount(bitmap));
    ok(obj_refcount(mil_bitmap) == 2, "ref count %ld\n", obj_refcount(mil_bitmap));

    hr = IWICBitmap_QueryInterface(bitmap, &IID_IUnknown, (void **)&wic_unknown);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = mil_bitmap->lpVtbl->QueryInterface(mil_bitmap, &IID_IUnknown, (void **)&mil_unknown);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);
    ok((void *)wic_unknown->lpVtbl == (void *)mil_unknown->lpVtbl, "wrong lpVtbl ptrs %p != %p\n", wic_unknown->lpVtbl, mil_unknown->lpVtbl);

    IUnknown_Release(wic_unknown);
    IUnknown_Release(mil_unknown);

    hr = IWICBitmap_QueryInterface(bitmap, &IID_IMILBitmapSource, (void **)&mil_source);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);
    ok((void *)mil_source->lpVtbl == (void *)mil_bitmap->lpVtbl, "IMILBitmap->lpVtbl should be equal to IMILBitmapSource->lpVtbl\n");

    ok(obj_refcount(bitmap) == 3, "ref count %ld\n", obj_refcount(bitmap));
    ok(obj_refcount(mil_bitmap) == 3, "ref count %ld\n", obj_refcount(mil_bitmap));
    ok(obj_refcount(mil_source) == 3, "ref count %ld\n", obj_refcount(mil_source));

    hr = mil_source->lpVtbl->GetPixelFormat(mil_source, &MIL_format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(MIL_format == 0x0c, "wrong format %d\n", MIL_format);

    hr = mil_source->lpVtbl->GetResolution(mil_source, &dpix, &dpiy);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = mil_source->lpVtbl->GetSize(mil_source, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 1, "got %u, expected 1\n", width);
    ok(height == 1, "got %u, expected 1\n", height);

    /* Scaler */
    hr = IWICImagingFactory_CreateBitmapScaler(factory, &scaler);
    ok(hr == S_OK, "CreateBitmapScaler error %#lx\n", hr);

    ok(obj_refcount(scaler) == 1, "ref count %ld\n", obj_refcount(scaler));

    hr = IWICBitmapScaler_QueryInterface(scaler, &IID_IMILBitmapScaler, (void **)&mil_scaler);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    ok(obj_refcount(scaler) == 2, "ref count %ld\n", obj_refcount(scaler));
    ok(obj_refcount(mil_scaler) == 2, "ref count %ld\n", obj_refcount(mil_scaler));

    hr = IWICBitmapScaler_QueryInterface(scaler, &IID_IUnknown, (void **)&wic_unknown);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = mil_scaler->lpVtbl->QueryInterface(mil_scaler, &IID_IUnknown, (void **)&mil_unknown);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);
    ok((void *)wic_unknown->lpVtbl == (void *)mil_unknown->lpVtbl, "wrong lpVtbl ptrs %p != %p\n", wic_unknown->lpVtbl, mil_unknown->lpVtbl);

    IUnknown_Release(wic_unknown);
    IUnknown_Release(mil_unknown);

    hr = mil_scaler->lpVtbl->GetPixelFormat(mil_scaler, &MIL_format);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "GetPixelFormat error %#lx\n", hr);

    hr = mil_scaler->lpVtbl->GetResolution(mil_scaler, &dpix, &dpiy);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "GetResolution error %#lx\n", hr);

    hr = mil_scaler->lpVtbl->GetSize(mil_scaler, &width, &height);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "GetSize error %#lx\n", hr);

    memset(buf, 0xde, sizeof(buf));
    hr = mil_scaler->lpVtbl->CopyPixels(mil_scaler, NULL, 3, sizeof(buf), buf);
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "CopyPixels error %#lx\n", hr);

    hr = mil_scaler->lpVtbl->Initialize(mil_scaler, mil_source, 1, 1, 1);
    ok(hr == S_OK, "Initialize error %#lx\n", hr);

    hr = mil_scaler->lpVtbl->GetPixelFormat(mil_scaler, &MIL_format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(MIL_format == 0x0c, "wrong format %d\n", MIL_format);

    hr = mil_scaler->lpVtbl->GetResolution(mil_scaler, &dpix, &dpiy);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = mil_scaler->lpVtbl->GetSize(mil_scaler, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 1, "got %u, expected 1\n", width);
    ok(height == 1, "got %u, expected 1\n", height);

    memset(buf, 0xde, sizeof(buf));
    hr = mil_scaler->lpVtbl->CopyPixels(mil_scaler, NULL, 3, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);
    ok(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0xde,"wrong data: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

    mil_scaler->lpVtbl->Release(mil_scaler);
    IWICBitmapScaler_Release(scaler);
    mil_source->lpVtbl->Release(mil_source);
    mil_bitmap->lpVtbl->Release(mil_bitmap);

    mil_unknown = (void *)0xdeadbeef;
    hr = IWICBitmap_QueryInterface(bitmap, &IID_CMetaBitmapRenderTarget, (void **)&mil_unknown);
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);
    ok(!mil_unknown, "got %p\n", mil_unknown);

    IWICBitmap_Release(bitmap);
}

START_TEST(bitmap)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);

    test_IMILBitmap();
    test_createbitmap();
    test_createbitmapfromsource();
    test_CreateBitmapFromMemory();
    test_CreateBitmapFromHICON();
    test_CreateBitmapFromHBITMAP();
    test_clipper();
    test_bitmap_scaler();

    IWICImagingFactory_Release(factory);

    CoUninitialize();

    test_WICCreateBitmapFromSectionEx();
}
