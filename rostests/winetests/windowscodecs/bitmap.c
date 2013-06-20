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

//#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
//#include <math.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <objbase.h>
#include <wincodec.h>
#include <wine/test.h>

static IWICImagingFactory *factory;

static const char *debugstr_guid(const GUID *guid)
{
    static char buf[50];

    if (!guid) return "(null)";
    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return buf;
}

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

    GetObject(hdib, sizeof(bm), &bm);
    ok(bm.bmWidth == width, "expected %d, got %d\n", width, bm.bmWidth);
    ok(bm.bmHeight == height, "expected %d, got %d\n", height, bm.bmHeight);
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
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%x\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%x\n", hr);

    /* Palette is unavailable until explicitly set */
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "IWICBitmap_CopyPalette failed hr=%x\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%x\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%x\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray4, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%x\n", hr);

    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_CopyPalette failed hr=%x\n", hr);

    hr = IWICPalette_GetType(palette, &palettetype);
    ok(hr == S_OK, "IWICPalette_GetType failed hr=%x\n", hr);
    ok(palettetype == WICBitmapPaletteTypeFixedGray256,
        "expected WICBitmapPaletteTypeFixedGray256, got %x\n", palettetype);

    IWICPalette_Release(palette);

    /* pixel data is initially zeroed */
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 9, 27, returned_data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%x\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == 0, "returned_data[%i] == %i\n", i, returned_data[i]);

    /* Invalid lock rects */
    rc.X = rc.Y = 0;
    rc.Width = 4;
    rc.Height = 3;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.Width = 3;
    rc.Height = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.Height = 3;
    rc.X = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    rc.X = 0;
    rc.Y = 4;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == E_INVALIDARG, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr)) IWICBitmapLock_Release(lock);

    /* NULL lock rect */
    hr = IWICBitmap_Lock(bitmap, NULL, WICBitmapLockRead, &lock);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* winxp */, "IWICBitmap_Lock failed hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        /* entire bitmap is locked */
        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%x\n", hr);
        ok(width == 3, "got %d, expected 3\n", width);
        ok(height == 3, "got %d, expected 3\n", height);

        IWICBitmapLock_Release(lock);
    }
    else
        can_lock_null = 0;

    /* lock with a valid rect */
    rc.Y = 0;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%x\n", hr);
        /* stride is divisible by 4 */
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%x\n", hr);
        /* buffer size does not include padding from the last row */
        ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
        ok(lock_buffer != NULL, "got NULL data pointer\n");
        base_lock_buffer = lock_buffer;

        hr = IWICBitmapLock_GetPixelFormat(lock, &pixelformat);
        ok(hr == S_OK, "IWICBitmapLock_GetPixelFormat failed hr=%x\n", hr);
        ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%x\n", hr);
        ok(width == 3, "got %d, expected 3\n", width);
        ok(height == 3, "got %d, expected 3\n", height);

        /* We can have multiple simultaneous read locks */
        hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock2);
        ok(hr == S_OK, "IWICBitmap_Lock failed hr=%x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapLock_GetDataPointer(lock2, &lock_buffer_size, &lock_buffer);
            ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%x\n", hr);
            ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
            ok(lock_buffer == base_lock_buffer, "got %p, expected %p\n", lock_buffer, base_lock_buffer);

            IWICBitmapLock_Release(lock2);
        }

        if (can_lock_null) /* this hangs on xp/vista */
        {
            /* But not a read and a write lock */
            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%x\n", hr);
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
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%x\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == bitmap_data[i], "returned_data[%i] == %i\n", i, returned_data[i]);

    /* try a valid partial rect, and write mode */
    rc.X = 2;
    rc.Y = 0;
    rc.Width = 1;
    rc.Height = 2;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        if (can_lock_null) /* this hangs on xp/vista */
        {
            /* Can't lock again while locked for writing */
            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%x\n", hr);

            hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockRead, &lock2);
            ok(hr == WINCODEC_ERR_ALREADYLOCKED, "IWICBitmap_Lock failed hr=%x\n", hr);
        }

        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%x\n", hr);
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%x\n", hr);
        ok(lock_buffer_size == 15, "got %i, expected 15\n", lock_buffer_size);
        ok(lock_buffer == base_lock_buffer+6, "got %p, expected %p+6\n", lock_buffer, base_lock_buffer);

        hr = IWICBitmapLock_GetPixelFormat(lock, &pixelformat);
        ok(hr == S_OK, "IWICBitmapLock_GetPixelFormat failed hr=%x\n", hr);
        ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

        hr = IWICBitmapLock_GetSize(lock, &width, &height);
        ok(hr == S_OK, "IWICBitmapLock_GetSize failed hr=%x\n", hr);
        ok(width == 1, "got %d, expected 1\n", width);
        ok(height == 2, "got %d, expected 2\n", height);

        IWICBitmapLock_Release(lock);
    }

    hr = IWICBitmap_GetPixelFormat(bitmap, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%x\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%x\n", hr);
    ok(dpix == 0.0, "got %f, expected 0.0\n", dpix);
    ok(dpiy == 0.0, "got %f, expected 0.0\n", dpiy);

    hr = IWICBitmap_SetResolution(bitmap, 12.0, 34.0);
    ok(hr == S_OK, "IWICBitmap_SetResolution failed hr=%x\n", hr);

    hr = IWICBitmap_GetResolution(bitmap, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%x\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%x\n", hr);
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
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%x\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%x\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%x\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%x\n", hr);

    IWICPalette_Release(palette);

    rc.X = rc.Y = 0;
    rc.Width = 3;
    rc.Height = 3;
    hr = IWICBitmap_Lock(bitmap, &rc, WICBitmapLockWrite, &lock);
    ok(hr == S_OK, "IWICBitmap_Lock failed hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapLock_GetStride(lock, &lock_buffer_stride);
        ok(hr == S_OK, "IWICBitmapLock_GetStride failed hr=%x\n", hr);
        ok(lock_buffer_stride == 12, "got %i, expected 12\n", lock_buffer_stride);

        hr = IWICBitmapLock_GetDataPointer(lock, &lock_buffer_size, &lock_buffer);
        ok(hr == S_OK, "IWICBitmapLock_GetDataPointer failed hr=%x\n", hr);
        ok(lock_buffer_size == 33, "got %i, expected 33\n", lock_buffer_size);
        ok(lock_buffer != NULL, "got NULL data pointer\n");

        for (i=0; i<3; i++)
            memcpy(lock_buffer + lock_buffer_stride*i, bitmap_data + i*9, 9);

        IWICBitmapLock_Release(lock);
    }

    hr = IWICBitmap_SetResolution(bitmap, 12.0, 34.0);
    ok(hr == S_OK, "IWICBitmap_SetResolution failed hr=%x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromSource(factory, (IWICBitmapSource*)bitmap,
        WICBitmapCacheOnLoad, &bitmap2);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromSource failed hr=%x\n", hr);

    IWICBitmap_Release(bitmap);

    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%x\n", hr);

    /* palette isn't copied for non-indexed formats? */
    hr = IWICBitmap_CopyPalette(bitmap2, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "IWICBitmap_CopyPalette failed hr=%x\n", hr);

    IWICPalette_Release(palette);

    hr = IWICBitmap_CopyPixels(bitmap2, NULL, 9, 27, returned_data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels failed hr=%x\n", hr);

    for (i=0; i<27; i++)
        ok(returned_data[i] == bitmap_data[i], "returned_data[%i] == %i\n", i, returned_data[i]);

    hr = IWICBitmap_GetPixelFormat(bitmap2, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%x\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

    hr = IWICBitmap_GetResolution(bitmap2, &dpix, &dpiy);
    ok(hr == S_OK, "IWICBitmap_GetResolution failed hr=%x\n", hr);
    ok(dpix == 12.0, "got %f, expected 12.0\n", dpix);
    ok(dpiy == 34.0, "got %f, expected 34.0\n", dpiy);

    hr = IWICBitmap_GetSize(bitmap2, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%x\n", hr);
    ok(width == 3, "got %d, expected 3\n", width);
    ok(height == 3, "got %d, expected 3\n", height);

    IWICBitmap_Release(bitmap2);

    /* Ensure palette is copied for indexed formats */
    hr = IWICImagingFactory_CreateBitmap(factory, 3, 3, &GUID_WICPixelFormat4bppIndexed,
        WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmap failed hr=%x\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%x\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedGray256, FALSE);
    ok(hr == S_OK, "IWICPalette_InitializePredefined failed hr=%x\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "IWICBitmap_SetPalette failed hr=%x\n", hr);

    IWICPalette_Release(palette);

    hr = IWICImagingFactory_CreateBitmapFromSource(factory, (IWICBitmapSource*)bitmap,
        WICBitmapCacheOnLoad, &bitmap2);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromSource failed hr=%x\n", hr);

    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "IWICImagingFactory_CreatePalette failed hr=%x\n", hr);

    hr = IWICBitmap_CopyPalette(bitmap2, palette);
    ok(hr == S_OK, "IWICBitmap_CopyPalette failed hr=%x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "IWICPalette_GetColorCount failed hr=%x\n", hr);
    ok(count == 256, "unexpected count %d\n", count);

    hr = IWICPalette_GetType(palette, &palette_type);
    ok(hr == S_OK, "IWICPalette_GetType failed hr=%x\n", hr);
    ok(palette_type == WICBitmapPaletteTypeFixedGray256, "unexpected palette type %d\n", palette_type);

    IWICPalette_Release(palette);

    hr = IWICBitmap_GetPixelFormat(bitmap2, &pixelformat);
    ok(hr == S_OK, "IWICBitmap_GetPixelFormat failed hr=%x\n", hr);
    ok(IsEqualGUID(&pixelformat, &GUID_WICPixelFormat4bppIndexed), "unexpected pixel format\n");

    hr = IWICBitmap_GetSize(bitmap2, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize failed hr=%x\n", hr);
    ok(width == 3, "got %d, expected 3\n", width);
    ok(height == 3, "got %d, expected 3\n", height);

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
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   0, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   6, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   12, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "expected WINCODEC_ERR_INSUFFICIENTBUFFER, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   9, sizeof(data3x3) - 1, data3x3, &bitmap);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "expected WINCODEC_ERR_INSUFFICIENTBUFFER, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 3, &GUID_WICPixelFormat24bppBGR,
                                                   9, sizeof(data3x3), data3x3, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromMemory error %#x\n", hr);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    data3x3[2] = 192;

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 9, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#x\n", hr);
    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == orig_data3x3[i], "%u: expected %u, got %u\n", i, data[i], data3x3[i]);

    IWICBitmap_Release(bitmap);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, 3, 2, &GUID_WICPixelFormat24bppBGR,
                                                   13, sizeof(orig_data3x3), orig_data3x3, &bitmap);
    ok(hr == S_OK, "IWICImagingFactory_CreateBitmapFromMemory error %#x\n", hr);

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 2, "expected 2, got %u\n", height);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 13, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#x\n", hr);
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
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, 0, &bitmap);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_CURSOR_HANDLE), "expected ERROR_INVALID_CURSOR_HANDLE, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, icon, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHICON(factory, icon, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHICON error %#x\n", hr);
    DestroyIcon(icon);
    if (hr != S_OK) return;

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "unexpected pixel format %s\n", debugstr_guid(&format));

    IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
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
    ok(hr == S_OK, "CreateBitmapFromHICON error %#x\n", hr);
    DestroyIcon(icon);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "unexpected pixel format %s\n", debugstr_guid(&format));

    IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
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

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, 0, 0, WICBitmapCacheOnLoad, &bitmap);
todo_wine
    ok(hr == WINCODEC_ERR_WIN32ERROR || hr == 0x88980003 /*XP*/, "expected WINCODEC_ERR_WIN32ERROR, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, 0, WICBitmapCacheOnLoad, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, 0, WICBitmapCacheOnLoad, &bitmap);
todo_wine
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#x\n", hr);
    if (hr != S_OK) return;

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "unexpected pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 4, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#x\n", hr);
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
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, hpal, WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#x\n", hr);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat4bppIndexed),
       "unexpected pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "%u: GetType error %#x\n", i, hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
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
    hr = IWICImagingFactory_CreateBitmapFromHBITMAP(factory, hbmp, hpal, WICBitmapCacheOnLoad, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromHBITMAP error %#x\n", hr);

    IWICBitmap_GetPixelFormat(bitmap, &format);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "unexpected pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmap_GetSize(bitmap, &width, &height);
    ok(hr == S_OK, "IWICBitmap_GetSize error %#x\n", hr);
    ok(width == 3, "expected 3, got %u\n", width);
    ok(height == 3, "expected 3, got %u\n", height);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);
    hr = IWICBitmap_CopyPalette(bitmap, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "%u: GetType error %#x\n", i, hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 256, "expected 256, got %u\n", count);

    IWICPalette_Release(palette);

    memset(data, 0, sizeof(data));
    hr = IWICBitmap_CopyPixels(bitmap, NULL, 4, sizeof(data), data);
    ok(hr == S_OK, "IWICBitmap_CopyPixels error %#x\n", hr);
    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == data_8bpp_pal_wic[i], "%u: expected %#x, got %#x\n", i, data_8bpp_pal_wic[i], data[i]);

    IWICBitmap_Release(bitmap);
    DeleteObject(hbmp);
    DeleteObject(hpal);
}

START_TEST(bitmap)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);

    test_createbitmap();
    test_createbitmapfromsource();
    test_CreateBitmapFromMemory();
    test_CreateBitmapFromHICON();
    test_CreateBitmapFromHBITMAP();

    IWICImagingFactory_Release(factory);

    CoUninitialize();
}
