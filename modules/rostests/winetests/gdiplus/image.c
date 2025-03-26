/*
 * Unit test suite for images
 *
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2012, 2016 Dmitry Timoshkov
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

#define COBJMACROS

#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "initguid.h"
#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#ifdef __REACTOS__
#include <ole2.h>
#endif

/* FIXME: They belong to gdipluseffects.h */
DEFINE_GUID(BlurEffectGuid, 0x633c80a4, 0x1843, 0x482b, 0x9e, 0xf2, 0xbe, 0x28, 0x34, 0xc5, 0xfd, 0xd4);
DEFINE_GUID(SharpenEffectGuid, 0x63cbf3ee, 0xc526, 0x402c, 0x8f, 0x71, 0x62, 0xc5, 0x40, 0xbf, 0x51, 0x42);
DEFINE_GUID(ColorMatrixEffectGuid, 0x718f2615, 0x7933, 0x40e3, 0xa5, 0x11, 0x5f, 0x68, 0xfe, 0x14, 0xdd, 0x74);
DEFINE_GUID(ColorLUTEffectGuid, 0xa7ce72a9, 0x0f7f, 0x40d7, 0xb3, 0xcc, 0xd0, 0xc0, 0x2d, 0x5c, 0x32, 0x12);
DEFINE_GUID(BrightnessContrastEffectGuid, 0xd3a1dbe1, 0x8ec4, 0x4c17, 0x9f, 0x4c, 0xea, 0x97, 0xad, 0x1c, 0x34, 0x3d);
DEFINE_GUID(HueSaturationLightnessEffectGuid, 0x8b2dd6c3, 0xeb07, 0x4d87, 0xa5, 0xf0, 0x71, 0x08, 0xe2, 0x6a, 0x9c, 0x5f);
DEFINE_GUID(LevelsEffectGuid, 0x99c354ec, 0x2a31, 0x4f3a, 0x8c, 0x34, 0x17, 0xa8, 0x03, 0xb3, 0x3a, 0x25);
DEFINE_GUID(TintEffectGuid, 0x1077af00, 0x2848, 0x4441, 0x94, 0x89, 0x44, 0xad, 0x4c, 0x2d, 0x7a, 0x2c);
DEFINE_GUID(ColorBalanceEffectGuid, 0x537e597d, 0x251e, 0x48da, 0x96, 0x64, 0x29, 0xca, 0x49, 0x6b, 0x70, 0xf8);
DEFINE_GUID(RedEyeCorrectionEffectGuid, 0x74d29d05, 0x69a4, 0x4266, 0x95, 0x49, 0x3c, 0xc5, 0x28, 0x36, 0xb6, 0x32);
DEFINE_GUID(ColorCurveEffectGuid, 0xdd6a0022, 0x58e4, 0x4a67, 0x9d, 0x9b, 0xd4, 0x8e, 0xb8, 0x81, 0xa5, 0x3d);

static GpStatus (WINAPI *pGdipBitmapGetHistogramSize)(HistogramFormat,UINT*);
static GpStatus (WINAPI *pGdipBitmapGetHistogram)(GpBitmap*,HistogramFormat,UINT,UINT*,UINT*,UINT*,UINT*);
static GpStatus (WINAPI *pGdipImageSetAbort)(GpImage*,GdiplusAbort*);

static GpStatus (WINGDIPAPI *pGdipInitializePalette)(ColorPalette*,PaletteType,INT,BOOL,GpBitmap*);

#define expect(expected, got) ok((got) == (expected), "Expected %d, got %d\n", (UINT)(expected), (UINT)(got))
#define expectf(expected, got) ok(fabs((expected) - (got)) < 0.0001, "Expected %f, got %f\n", (expected), (got))

static BOOL color_match(ARGB c1, ARGB c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    return TRUE;
}

static void expect_guid(REFGUID expected, REFGUID got, int line, BOOL todo)
{
    WCHAR bufferW[39];
    char buffer[39];
    char buffer2[39];

    StringFromGUID2(got, bufferW, ARRAY_SIZE(bufferW));
    WideCharToMultiByte(CP_ACP, 0, bufferW, ARRAY_SIZE(bufferW), buffer, sizeof(buffer), NULL, NULL);
    StringFromGUID2(expected, bufferW, ARRAY_SIZE(bufferW));
    WideCharToMultiByte(CP_ACP, 0, bufferW, ARRAY_SIZE(bufferW), buffer2, sizeof(buffer2), NULL, NULL);
    todo_wine_if (todo)
        ok_(__FILE__, line)(IsEqualGUID(expected, got), "Expected %s, got %s\n", buffer2, buffer);
}

static void expect_rawformat(REFGUID expected, GpImage *img, int line, BOOL todo)
{
    GUID raw;
    GpStatus stat;

    stat = GdipGetImageRawFormat(img, &raw);
    ok_(__FILE__, line)(stat == Ok, "GdipGetImageRawFormat failed with %d\n", stat);
    if(stat != Ok) return;
    expect_guid(expected, &raw, line, todo);
}

static void test_bufferrawformat(void* buff, int size, REFGUID expected, int line, BOOL todo)
{
    LPSTREAM stream;
    HGLOBAL  hglob;
    LPBYTE   data;
    HRESULT  hres;
    GpStatus stat;
    GpImage *img;

    hglob = GlobalAlloc (0, size);
    data = GlobalLock (hglob);
    memcpy(data, buff, size);
    GlobalUnlock(hglob); data = NULL;

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok_(__FILE__, line)(hres == S_OK, "Failed to create a stream\n");
    if(hres != S_OK) return;

    stat = GdipLoadImageFromStream(stream, &img);
    ok_(__FILE__, line)(stat == Ok, "Failed to create a Bitmap\n");
    if(stat != Ok){
        IStream_Release(stream);
        return;
    }

    expect_rawformat(expected, img, line, todo);

    GdipDisposeImage(img);
    IStream_Release(stream);
}

static void test_Scan0(void)
{
    GpBitmap *bm;
    GpStatus stat;
    BYTE buff[360];

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, -10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(-10, 10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 0, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, 12, PixelFormat24bppRGB, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*) 0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, 0, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    ok( bm == (GpBitmap*)0xdeadbeef, "expected deadbeef bitmap\n" );

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, -8, PixelFormat24bppRGB, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, -10, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );
}

static void test_FromGdiDib(void)
{
    GpBitmap *bm;
    GpStatus stat;
    BYTE buff[400];
    BYTE rbmi[sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD)];
    BITMAPINFO *bmi = (BITMAPINFO*)rbmi;
    PixelFormat format;

    bm = NULL;

    memset(rbmi, 0, sizeof(rbmi));

    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = 10;
    bmi->bmiHeader.biHeight = 10;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biCompression = BI_RGB;

    stat = GdipCreateBitmapFromGdiDib(NULL, buff, &bm);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromGdiDib(bmi, NULL, &bm);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromGdiDib(bmi, buff, NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat32bppRGB, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 24;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat24bppRGB, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 16;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat16bppRGB555, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 8;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat8bppIndexed, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 4;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat4bppIndexed, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 1;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
    {
        stat = GdipGetImagePixelFormat((GpImage*)bm, &format);
        expect(Ok, stat);
        expect(PixelFormat1bppIndexed, format);

        GdipDisposeImage((GpImage*)bm);
    }

    bmi->bmiHeader.biBitCount = 0;
    stat = GdipCreateBitmapFromGdiDib(bmi, buff, &bm);
    expect(InvalidParameter, stat);
}

static void test_GetImageDimension(void)
{
    GpBitmap *bm;
    GpStatus stat;
    const REAL WIDTH = 10.0, HEIGHT = 20.0;
    REAL w,h;

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB,NULL, &bm);
    expect(Ok,stat);
    ok((GpBitmap*)0xdeadbeef != bm, "Expected bitmap to not be 0xdeadbeef\n");
    ok(NULL != bm, "Expected bitmap to not be NULL\n");

    stat = GdipGetImageDimension(NULL,&w,&h);
    expect(InvalidParameter, stat);

    stat = GdipGetImageDimension((GpImage*)bm,NULL,&h);
    expect(InvalidParameter, stat);

    stat = GdipGetImageDimension((GpImage*)bm,&w,NULL);
    expect(InvalidParameter, stat);

    w = -1;
    h = -1;
    stat = GdipGetImageDimension((GpImage*)bm,&w,&h);
    expect(Ok, stat);
    expectf(WIDTH,  w);
    expectf(HEIGHT, h);
    GdipDisposeImage((GpImage*)bm);
}

static void test_GdipImageGetFrameDimensionsCount(void)
{
    GpBitmap *bm;
    GpStatus stat;
    const REAL WIDTH = 10.0, HEIGHT = 20.0;
    UINT w;
    GUID dimension = {0};
    UINT count;
    ARGB color;

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB,NULL, &bm);
    expect(Ok,stat);
    ok((GpBitmap*)0xdeadbeef != bm, "Expected bitmap to not be 0xdeadbeef\n");
    ok(NULL != bm, "Expected bitmap to not be NULL\n");

    stat = GdipImageGetFrameDimensionsCount(NULL,&w);
    expect(InvalidParameter, stat);

    stat = GdipImageGetFrameDimensionsCount((GpImage*)bm,NULL);
    expect(InvalidParameter, stat);

    w = -1;
    stat = GdipImageGetFrameDimensionsCount((GpImage*)bm,&w);
    expect(Ok, stat);
    expect(1, w);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bm, &dimension, 1);
    expect(Ok, stat);
    expect_guid(&FrameDimensionPage, &dimension, __LINE__, FALSE);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bm, &dimension, 2);
    expect(InvalidParameter, stat);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bm, &dimension, 0);
    expect(InvalidParameter, stat);

    stat = GdipImageGetFrameCount(NULL, &dimension, &count);
    expect(InvalidParameter, stat);

    /* WinXP crashes on this test */
    if(0)
    {
        stat = GdipImageGetFrameCount((GpImage*)bm, &dimension, NULL);
        expect(InvalidParameter, stat);
    }

    stat = GdipImageGetFrameCount((GpImage*)bm, NULL, &count);
    expect(Ok, stat);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bm, &dimension, &count);
    expect(Ok, stat);
    expect(1, count);

    GdipBitmapSetPixel(bm, 0, 0, 0xffffffff);

    stat = GdipImageSelectActiveFrame((GpImage*)bm, &dimension, 0);
    expect(Ok, stat);

    /* SelectActiveFrame has no effect on image data of memory bitmaps */
    color = 0xdeadbeef;
    stat = GdipBitmapGetPixel(bm, 0, 0, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

    GdipDisposeImage((GpImage*)bm);
}

static void test_LoadingImages(void)
{
    GpStatus stat;
    GpBitmap *bm;
    GpImage *img;
    static const WCHAR nonexistentW[] = {'n','o','n','e','x','i','s','t','e','n','t',0};

    stat = GdipCreateBitmapFromFile(0, 0);
    expect(InvalidParameter, stat);

    bm = (GpBitmap *)0xdeadbeef;
    stat = GdipCreateBitmapFromFile(0, &bm);
    expect(InvalidParameter, stat);
    ok(bm == (GpBitmap *)0xdeadbeef, "returned %p\n", bm);

    bm = (GpBitmap *)0xdeadbeef;
    stat = GdipCreateBitmapFromFile(nonexistentW, &bm);
    todo_wine expect(InvalidParameter, stat);
    ok(!bm, "returned %p\n", bm);

    stat = GdipLoadImageFromFile(0, 0);
    expect(InvalidParameter, stat);

    img = (GpImage *)0xdeadbeef;
    stat = GdipLoadImageFromFile(0, &img);
    expect(InvalidParameter, stat);
    ok(img == (GpImage *)0xdeadbeef, "returned %p\n", img);

    img = (GpImage *)0xdeadbeef;
    stat = GdipLoadImageFromFile(nonexistentW, &img);
    todo_wine expect(OutOfMemory, stat);
    ok(!img, "returned %p\n", img);

    stat = GdipLoadImageFromFileICM(0, 0);
    expect(InvalidParameter, stat);

    img = (GpImage *)0xdeadbeef;
    stat = GdipLoadImageFromFileICM(0, &img);
    expect(InvalidParameter, stat);
    ok(img == (GpImage *)0xdeadbeef, "returned %p\n", img);

    img = (GpImage *)0xdeadbeef;
    stat = GdipLoadImageFromFileICM(nonexistentW, &img);
    todo_wine expect(OutOfMemory, stat);
    ok(!img, "returned %p\n", img);
}

static void test_SavingImages(void)
{
    GpStatus stat;
    GpBitmap *bm;
    UINT n;
    UINT s;
    const REAL WIDTH = 10.0, HEIGHT = 20.0;
    REAL w, h;
    ImageCodecInfo *codecs;
    static const CHAR filenameA[] = "a.bmp";
    static const WCHAR filename[] = { 'a','.','b','m','p',0 };

    codecs = NULL;

    stat = GdipSaveImageToFile(0, 0, 0, 0);
    expect(InvalidParameter, stat);

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    if (!bm)
        return;

    /* invalid params */
    stat = GdipSaveImageToFile((GpImage*)bm, 0, 0, 0);
    expect(InvalidParameter, stat);

    stat = GdipSaveImageToFile((GpImage*)bm, filename, 0, 0);
    expect(InvalidParameter, stat);

    /* encoder tests should succeed -- already tested */
    stat = GdipGetImageEncodersSize(&n, &s);
    if (stat != Ok || n == 0) goto cleanup;

    codecs = GdipAlloc(s);
    if (!codecs) goto cleanup;

    stat = GdipGetImageEncoders(n, s, codecs);
    if (stat != Ok) goto cleanup;

    stat = GdipSaveImageToFile((GpImage*)bm, filename, &codecs[0].Clsid, 0);
    expect(Ok, stat);

    GdipDisposeImage((GpImage*)bm);
    bm = 0;

    /* re-load and check image stats */
    stat = GdipLoadImageFromFile(filename, (GpImage**)&bm);
    expect(Ok, stat);
    if (stat != Ok) goto cleanup;

    stat = GdipGetImageDimension((GpImage*)bm, &w, &h);
    if (stat != Ok) goto cleanup;

    expectf(WIDTH, w);
    expectf(HEIGHT, h);

 cleanup:
    GdipFree(codecs);
    if (bm)
        GdipDisposeImage((GpImage*)bm);
    ok(DeleteFileA(filenameA), "Delete failed.\n");
}

static void test_encoders(void)
{
    GpStatus stat;
    UINT n;
    UINT s;
    ImageCodecInfo *codecs;
    int i;
    int bmp_found;

    static const CHAR bmp_format[] = "BMP";

    stat = GdipGetImageEncodersSize(&n, &s);
    expect(stat, Ok);

    codecs = GdipAlloc(s);
    if (!codecs)
        return;

    stat = GdipGetImageEncoders(n, s, NULL);
    expect(GenericError, stat);

    stat = GdipGetImageEncoders(0, s, codecs);
    expect(GenericError, stat);

    stat = GdipGetImageEncoders(n, s-1, codecs);
    expect(GenericError, stat);

    stat = GdipGetImageEncoders(n, s+1, codecs);
    expect(GenericError, stat);

    stat = GdipGetImageEncoders(n, s, codecs);
    expect(stat, Ok);

    bmp_found = FALSE;
    for (i = 0; i < n; i++)
        {
            CHAR desc[32];

            WideCharToMultiByte(CP_ACP, 0, codecs[i].FormatDescription, -1,
                                desc, 32, 0, 0);

            if (CompareStringA(LOCALE_SYSTEM_DEFAULT, 0,
                               desc, -1,
                               bmp_format, -1) == CSTR_EQUAL) {
                bmp_found = TRUE;
                break;
            }
        }
    if (!bmp_found)
        ok(FALSE, "No BMP codec found.\n");

    GdipFree(codecs);
}

static void test_LockBits(void)
{
    GpStatus stat;
    GpBitmap *bm;
    GpRect rect;
    BitmapData bd;
    const INT WIDTH = 10, HEIGHT = 20;
    ARGB color;
    int y;

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    rect.X = 2;
    rect.Y = 3;
    rect.Width = 4;
    rect.Height = 5;

    stat = GdipBitmapSetPixel(bm, 2, 3, 0xffc30000);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bm, 2, 8, 0xff480000);
    expect(Ok, stat);

    /* read-only */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        expect(0xc3, ((BYTE*)bd.Scan0)[2]);
        expect(0x48, ((BYTE*)bd.Scan0)[2 + bd.Stride * 5]);

        ((char*)bd.Scan0)[2] = 0xff;

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapSetPixel(bm, 2, 3, 0xffc30000);
    expect(Ok, stat);

    /* read-only, with NULL rect -> whole bitmap lock */
    stat = GdipBitmapLockBits(bm, NULL, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);
    expect(bd.Width,  WIDTH);
    expect(bd.Height, HEIGHT);

    if (stat == Ok) {
        ((char*)bd.Scan0)[2 + 2*3 + 3*bd.Stride] = 0xff;

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    /* read-only, consecutive */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    /* read x2 */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(WrongState, stat);

    stat = GdipBitmapUnlockBits(bm, &bd);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bm, 2, 3, 0xffff0000);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bm, 2, 8, 0xffc30000);
    expect(Ok, stat);

    /* write, no conversion */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        /* all bits are readable, inside the rect or not */
        expect(0xff, ((BYTE*)bd.Scan0)[2]);
        expect(0xc3, ((BYTE*)bd.Scan0)[2 + bd.Stride * 5]);

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    /* read, conversion */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        expect(0xff, ((BYTE*)bd.Scan0)[2]);
        if (0)
            /* Areas outside the rectangle appear to be uninitialized */
            ok(0xc3 != ((BYTE*)bd.Scan0)[2 + bd.Stride * 5], "original image bits are readable\n");

        ((BYTE*)bd.Scan0)[2] = 0xc3;

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    /* writes do not work in read mode if there was a conversion */
    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    /* read/write, conversion */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead|ImageLockModeWrite, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        expect(0xff, ((BYTE*)bd.Scan0)[2]);
        ((BYTE*)bd.Scan0)[1] = 0x88;
        if (0)
            /* Areas outside the rectangle appear to be uninitialized */
            ok(0xc3 != ((BYTE*)bd.Scan0)[2 + bd.Stride * 5], "original image bits are readable\n");

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xffff8800, color);

    /* write, conversion */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        if (0)
        {
            /* This is completely uninitialized. */
            ok(0xff != ((BYTE*)bd.Scan0)[2], "original image bits are readable\n");
            ok(0xc3 != ((BYTE*)bd.Scan0)[2 + bd.Stride * 5], "original image bits are readable\n");
        }

        /* Initialize the buffer so the unlock doesn't access undefined memory */
        for (y=0; y<5; y++)
            memset(((BYTE*)bd.Scan0) + bd.Stride * y, 0, 12);

        ((BYTE*)bd.Scan0)[0] = 0x12;
        ((BYTE*)bd.Scan0)[1] = 0x34;
        ((BYTE*)bd.Scan0)[2] = 0x56;

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xff563412, color);

    stat = GdipBitmapGetPixel(bm, 2, 8, &color);
    expect(Ok, stat);
    expect(0xffc30000, color);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    /* write, no modification */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    /* write, consecutive */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    /* write, modify */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        if (bd.Scan0)
            ((char*)bd.Scan0)[2] = 0xff;

        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);

    /* dispose locked */
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);
    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
}

static void test_LockBits_UserBuf(void)
{
    GpStatus stat;
    GpBitmap *bm;
    GpRect rect;
    BitmapData bd;
    const INT WIDTH = 10, HEIGHT = 20;
    DWORD bits[200];
    ARGB color;

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat32bppARGB, NULL, &bm);
    expect(Ok, stat);

    memset(bits, 0xaa, sizeof(bits));

    rect.X = 2;
    rect.Y = 3;
    rect.Width = 4;
    rect.Height = 5;

    bd.Width = 4;
    bd.Height = 6;
    bd.Stride = WIDTH * 4;
    bd.PixelFormat = PixelFormat32bppARGB;
    bd.Scan0 = &bits[2+3*WIDTH];
    bd.Reserved = 0xaaaaaaaa;

    /* read-only */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead|ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    expect(0xaaaaaaaa, bits[0]);
    expect(0, bits[2+3*WIDTH]);

    bits[2+3*WIDTH] = 0xdeadbeef;

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0, color);

    /* write-only */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeWrite|ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    expect(0xdeadbeef, bits[2+3*WIDTH]);
    bits[2+3*WIDTH] = 0x12345678;

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0x12345678, color);

    bits[2+3*WIDTH] = 0;

    /* read/write */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead|ImageLockModeWrite|ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd);
    expect(Ok, stat);

    expect(0x12345678, bits[2+3*WIDTH]);
    bits[2+3*WIDTH] = 0xdeadbeef;

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    stat = GdipBitmapGetPixel(bm, 2, 3, &color);
    expect(Ok, stat);
    expect(0xdeadbeef, color);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
}

struct BITMAPINFOWITHBITFIELDS
{
    BITMAPINFOHEADER bmiHeader;
    DWORD masks[3];
};

union BITMAPINFOUNION
{
    BITMAPINFO bi;
    struct BITMAPINFOWITHBITFIELDS bf;
};

static void test_GdipCreateBitmapFromHBITMAP(void)
{
    GpBitmap* gpbm = NULL;
    HBITMAP hbm = NULL;
    HPALETTE hpal = NULL;
    GpStatus stat;
    BYTE buff[1000];
    LOGPALETTE* LogPal = NULL;
    REAL width, height;
    const REAL WIDTH1 = 5;
    const REAL HEIGHT1 = 15;
    const REAL WIDTH2 = 10;
    const REAL HEIGHT2 = 20;
    HDC hdc;
    union BITMAPINFOUNION bmi;
    BYTE *bits;
    PixelFormat format;

    stat = GdipCreateBitmapFromHBITMAP(NULL, NULL, NULL);
    expect(InvalidParameter, stat);

    hbm = CreateBitmap(WIDTH1, HEIGHT1, 1, 1, NULL);
    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    expectf(WIDTH1,  width);
    expectf(HEIGHT1, height);
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);
    DeleteObject(hbm);

    memset(buff, 0, sizeof(buff));
    hbm = CreateBitmap(WIDTH2, HEIGHT2, 1, 1, &buff);
    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    /* raw format */
    expect_rawformat(&ImageFormatMemoryBMP, (GpImage*)gpbm, __LINE__, FALSE);

    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    expectf(WIDTH2,  width);
    expectf(HEIGHT2, height);
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);
    DeleteObject(hbm);

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");
    bmi.bi.bmiHeader.biSize = sizeof(bmi.bi.bmiHeader);
    bmi.bi.bmiHeader.biHeight = HEIGHT1;
    bmi.bi.bmiHeader.biWidth = WIDTH1;
    bmi.bi.bmiHeader.biBitCount = 24;
    bmi.bi.bmiHeader.biPlanes = 1;
    bmi.bi.bmiHeader.biCompression = BI_RGB;
    bmi.bi.bmiHeader.biClrUsed = 0;

    hbm = CreateDIBSection(hdc, &bmi.bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hbm != NULL, "CreateDIBSection failed\n");

    bits[0] = 0;

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    expectf(WIDTH1,  width);
    expectf(HEIGHT1, height);
    if (stat == Ok)
    {
        /* test whether writing to the bitmap affects the original */
        stat = GdipBitmapSetPixel(gpbm, 0, 0, 0xffffffff);
        expect(Ok, stat);

        expect(0, bits[0]);

        GdipDisposeImage((GpImage*)gpbm);
    }

    LogPal = GdipAlloc(sizeof(LOGPALETTE));
    ok(LogPal != NULL, "unable to allocate LOGPALETTE\n");
    LogPal->palVersion = 0x300;
    LogPal->palNumEntries = 1;
    hpal = CreatePalette(LogPal);
    ok(hpal != NULL, "CreatePalette failed\n");
    GdipFree(LogPal);

    stat = GdipCreateBitmapFromHBITMAP(hbm, hpal, &gpbm);
    expect(Ok, stat);

    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);

    DeleteObject(hpal);
    DeleteObject(hbm);

    /* 16-bit 555 dib, rgb */
    bmi.bi.bmiHeader.biBitCount = 16;
    bmi.bi.bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, &bmi.bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hbm != NULL, "CreateDIBSection failed\n");

    bits[0] = 0;

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageDimension((GpImage*) gpbm, &width, &height);
        expect(Ok, stat);
        expectf(WIDTH1,  width);
        expectf(HEIGHT1, height);

        stat = GdipGetImagePixelFormat((GpImage*) gpbm, &format);
        expect(Ok, stat);
        expect(PixelFormat16bppRGB555, format);

        GdipDisposeImage((GpImage*)gpbm);
    }
    DeleteObject(hbm);

    /* 16-bit 555 dib, with bitfields */
    bmi.bi.bmiHeader.biSize = sizeof(bmi);
    bmi.bi.bmiHeader.biCompression = BI_BITFIELDS;
    bmi.bf.masks[0] = 0x7c00;
    bmi.bf.masks[1] = 0x3e0;
    bmi.bf.masks[2] = 0x1f;

    hbm = CreateDIBSection(hdc, &bmi.bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hbm != NULL, "CreateDIBSection failed\n");

    bits[0] = 0;

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageDimension((GpImage*) gpbm, &width, &height);
        expect(Ok, stat);
        expectf(WIDTH1,  width);
        expectf(HEIGHT1, height);

        stat = GdipGetImagePixelFormat((GpImage*) gpbm, &format);
        expect(Ok, stat);
        expect(PixelFormat16bppRGB555, format);

        GdipDisposeImage((GpImage*)gpbm);
    }
    DeleteObject(hbm);

    /* 16-bit 565 dib, with bitfields */
    bmi.bf.masks[0] = 0xf800;
    bmi.bf.masks[1] = 0x7e0;
    bmi.bf.masks[2] = 0x1f;

    hbm = CreateDIBSection(hdc, &bmi.bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hbm != NULL, "CreateDIBSection failed\n");

    bits[0] = 0;

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageDimension((GpImage*) gpbm, &width, &height);
        expect(Ok, stat);
        expectf(WIDTH1,  width);
        expectf(HEIGHT1, height);

        stat = GdipGetImagePixelFormat((GpImage*) gpbm, &format);
        expect(Ok, stat);
        expect(PixelFormat16bppRGB565, format);

        GdipDisposeImage((GpImage*)gpbm);
    }
    DeleteObject(hbm);

    DeleteDC(hdc);
}

static void test_GdipGetImageFlags(void)
{
    GpImage *img;
    GpStatus stat;
    UINT flags;

    img = (GpImage*)0xdeadbeef;

    stat = GdipGetImageFlags(NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipGetImageFlags(NULL, &flags);
    expect(InvalidParameter, stat);

    stat = GdipGetImageFlags(img, NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat1bppIndexed, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat4bppIndexed, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat8bppIndexed, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat16bppGrayScale, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsNone, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat16bppRGB555, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsNone, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat16bppRGB565, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsNone, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat16bppARGB1555, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsNone, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat32bppRGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsNone, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat32bppARGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat32bppPARGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    stat = GdipGetImageFlags(img, &flags);
    expect(Ok, stat);
    expect(ImageFlagsHasAlpha, flags);
    GdipDisposeImage(img);

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat48bppRGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    if (stat == Ok)
    {
        stat = GdipGetImageFlags(img, &flags);
        expect(Ok, stat);
        expect(ImageFlagsNone, flags);
        GdipDisposeImage(img);
    }

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat64bppARGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    if (stat == Ok)
    {
        expect(Ok, stat);
        stat = GdipGetImageFlags(img, &flags);
        expect(Ok, stat);
        expect(ImageFlagsHasAlpha, flags);
        GdipDisposeImage(img);
    }

    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat64bppPARGB, NULL, (GpBitmap**)&img);
    expect(Ok, stat);
    if (stat == Ok)
    {
        expect(Ok, stat);
        stat = GdipGetImageFlags(img, &flags);
        expect(Ok, stat);
        expect(ImageFlagsHasAlpha, flags);
        GdipDisposeImage(img);
    }
}

static void test_GdipCloneImage(void)
{
    GpStatus stat;
    GpRectF rectF;
    GpUnit unit;
    GpBitmap *bm;
    GpImage *image_src, *image_dest = NULL;
    const INT WIDTH = 10, HEIGHT = 20;

    /* Create an image, clone it, delete the original, make sure the copy works */
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    expect_rawformat(&ImageFormatMemoryBMP, (GpImage*)bm, __LINE__, FALSE);

    image_src = ((GpImage*)bm);
    stat = GdipCloneImage(image_src, &image_dest);
    expect(Ok, stat);
    expect_rawformat(&ImageFormatMemoryBMP, image_dest, __LINE__, FALSE);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipGetImageBounds(image_dest, &rectF, &unit);
    expect(Ok, stat);

    /* Treat FP values carefully */
    expectf((REAL)WIDTH, rectF.Width);
    expectf((REAL)HEIGHT, rectF.Height);

    stat = GdipDisposeImage(image_dest);
    expect(Ok, stat);
}

static void test_testcontrol(void)
{
    GpStatus stat;
    DWORD param;

    param = 0;
    stat = GdipTestControl(TestControlGetBuildNumber, &param);
    expect(Ok, stat);
    ok(param != 0, "Build number expected, got %u\n", param);
}

static void test_fromhicon(void)
{
    static const BYTE bmp_bits[1024];
    HBITMAP hbmMask, hbmColor;
    ICONINFO info;
    HICON hIcon;
    GpStatus stat;
    GpBitmap *bitmap = NULL;
    UINT dim;
    ImageType type;
    PixelFormat format;

    /* NULL */
    stat = GdipCreateBitmapFromHICON(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreateBitmapFromHICON(NULL, &bitmap);
    expect(InvalidParameter, stat);

    /* color icon 1 bit */
    hbmMask = CreateBitmap(16, 16, 1, 1, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    hbmColor = CreateBitmap(16, 16, 1, 1, bmp_bits);
    ok(hbmColor != 0, "CreateBitmap failed\n");
    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = hbmColor;
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    DeleteObject(hbmMask);
    DeleteObject(hbmColor);

    stat = GdipCreateBitmapFromHICON(hIcon, &bitmap);
    ok(stat == Ok ||
       broken(stat == InvalidParameter), /* Win98 */
       "Expected Ok, got %.8x\n", stat);
    if(stat == Ok){
       /* check attributes */
       stat = GdipGetImageHeight((GpImage*)bitmap, &dim);
       expect(Ok, stat);
       expect(16, dim);
       stat = GdipGetImageWidth((GpImage*)bitmap, &dim);
       expect(Ok, stat);
       expect(16, dim);
       stat = GdipGetImageType((GpImage*)bitmap, &type);
       expect(Ok, stat);
       expect(ImageTypeBitmap, type);
       stat = GdipGetImagePixelFormat((GpImage*)bitmap, &format);
       expect(Ok, stat);
       expect(PixelFormat32bppARGB, format);
       /* raw format */
       expect_rawformat(&ImageFormatMemoryBMP, (GpImage*)bitmap, __LINE__, FALSE);
       GdipDisposeImage((GpImage*)bitmap);
    }
    DestroyIcon(hIcon);

    /* color icon 8 bpp */
    hbmMask = CreateBitmap(16, 16, 1, 8, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    hbmColor = CreateBitmap(16, 16, 1, 8, bmp_bits);
    ok(hbmColor != 0, "CreateBitmap failed\n");
    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = hbmColor;
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    DeleteObject(hbmMask);
    DeleteObject(hbmColor);

    stat = GdipCreateBitmapFromHICON(hIcon, &bitmap);
    expect(Ok, stat);
    if(stat == Ok){
        /* check attributes */
        stat = GdipGetImageHeight((GpImage*)bitmap, &dim);
        expect(Ok, stat);
        expect(16, dim);
        stat = GdipGetImageWidth((GpImage*)bitmap, &dim);
        expect(Ok, stat);
        expect(16, dim);
        stat = GdipGetImageType((GpImage*)bitmap, &type);
        expect(Ok, stat);
        expect(ImageTypeBitmap, type);
        stat = GdipGetImagePixelFormat((GpImage*)bitmap, &format);
	expect(Ok, stat);
        expect(PixelFormat32bppARGB, format);
        /* raw format */
        expect_rawformat(&ImageFormatMemoryBMP, (GpImage*)bitmap, __LINE__, FALSE);
        GdipDisposeImage((GpImage*)bitmap);
    }
    DestroyIcon(hIcon);
}

/* 1x1 pixel png */
static const unsigned char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};
/* 1x1 pixel gif */
static const unsigned char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};
/* 1x1 pixel transparent gif */
static const unsigned char transparentgif[] = {
0x47,0x49,0x46,0x38,0x39,0x61,0x01,0x00,0x01,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x21,0xf9,0x04,0x01,0x00,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,
0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,0x01,0x00,0x3b
};
/* 1x1 pixel bmp */
static const unsigned char bmpimage[66] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
0x00,0x00
};
/* 1x1 pixel jpg */
static const unsigned char jpgimage[285] = {
0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x01,0x01,0x01,0x2c,
0x01,0x2c,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x05,0x03,0x04,0x04,0x04,0x03,0x05,
0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x07,0x0c,0x08,0x07,0x07,0x07,0x07,0x0f,0x0b,
0x0b,0x09,0x0c,0x11,0x0f,0x12,0x12,0x11,0x0f,0x11,0x11,0x13,0x16,0x1c,0x17,0x13,
0x14,0x1a,0x15,0x11,0x11,0x18,0x21,0x18,0x1a,0x1d,0x1d,0x1f,0x1f,0x1f,0x13,0x17,
0x22,0x24,0x22,0x1e,0x24,0x1c,0x1e,0x1f,0x1e,0xff,0xdb,0x00,0x43,0x01,0x05,0x05,
0x05,0x07,0x06,0x07,0x0e,0x08,0x08,0x0e,0x1e,0x14,0x11,0x14,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0xff,0xc0,
0x00,0x11,0x08,0x00,0x01,0x00,0x01,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
0x01,0xff,0xc4,0x00,0x15,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0xc4,0x00,0x14,0x10,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc4,
0x00,0x14,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xff,0xc4,0x00,0x14,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,
0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,0xb2,0xc0,0x07,0xff,0xd9
};
/* 1x1 pixel tiff */
static const unsigned char tiffimage[] = {
0x49,0x49,0x2a,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0xfe,0x00,
0x04,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x01,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0x02,0x01,0x03,0x00,0x03,0x00,0x00,0x00,0xd2,0x00,0x00,0x00,0x03,0x01,
0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x06,0x01,0x03,0x00,0x01,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0x0d,0x01,0x02,0x00,0x1b,0x00,0x00,0x00,0xd8,0x00,
0x00,0x00,0x11,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x12,0x01,
0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x15,0x01,0x03,0x00,0x01,0x00,
0x00,0x00,0x03,0x00,0x00,0x00,0x16,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x40,0x00,
0x00,0x00,0x17,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x1a,0x01,
0x05,0x00,0x01,0x00,0x00,0x00,0xf4,0x00,0x00,0x00,0x1b,0x01,0x05,0x00,0x01,0x00,
0x00,0x00,0xfc,0x00,0x00,0x00,0x1c,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0x28,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x2f,0x68,0x6f,0x6d,0x65,0x2f,0x6d,0x65,
0x68,0x2f,0x44,0x65,0x73,0x6b,0x74,0x6f,0x70,0x2f,0x74,0x65,0x73,0x74,0x2e,0x74,
0x69,0x66,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x48,
0x00,0x00,0x00,0x01
};
/* 320x320 twip wmf */
static const unsigned char wmfimage[180] = {
0xd7,0xcd,0xc6,0x9a,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x01,0x40,0x01,0xa0,0x05,
0x00,0x00,0x00,0x00,0xb1,0x52,0x01,0x00,0x09,0x00,0x00,0x03,0x4f,0x00,0x00,0x00,
0x0f,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x0b,0x02,0x00,0x00,
0x00,0x00,0x05,0x00,0x00,0x00,0x0c,0x02,0x40,0x01,0x40,0x01,0x04,0x00,0x00,0x00,
0x02,0x01,0x01,0x00,0x04,0x00,0x00,0x00,0x04,0x01,0x0d,0x00,0x08,0x00,0x00,0x00,
0xfa,0x02,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
0x2d,0x01,0x00,0x00,0x07,0x00,0x00,0x00,0xfc,0x02,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x2d,0x01,0x01,0x00,0x07,0x00,0x00,0x00,0xfc,0x02,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x2d,0x01,0x02,0x00,
0x07,0x00,0x00,0x00,0x1b,0x04,0x40,0x01,0x40,0x01,0x00,0x00,0x00,0x00,0x04,0x00,
0x00,0x00,0xf0,0x01,0x00,0x00,0x04,0x00,0x00,0x00,0xf0,0x01,0x01,0x00,0x03,0x00,
0x00,0x00,0x00,0x00
};
static void test_getrawformat(void)
{
    test_bufferrawformat((void*)pngimage, sizeof(pngimage), &ImageFormatPNG,  __LINE__, FALSE);
    test_bufferrawformat((void*)gifimage, sizeof(gifimage), &ImageFormatGIF,  __LINE__, FALSE);
    test_bufferrawformat((void*)bmpimage, sizeof(bmpimage), &ImageFormatBMP,  __LINE__, FALSE);
    test_bufferrawformat((void*)jpgimage, sizeof(jpgimage), &ImageFormatJPEG, __LINE__, FALSE);
    test_bufferrawformat((void*)tiffimage, sizeof(tiffimage), &ImageFormatTIFF, __LINE__, FALSE);
    test_bufferrawformat((void*)wmfimage, sizeof(wmfimage), &ImageFormatWMF, __LINE__, FALSE);
}

static void test_loadwmf(void)
{
    LPSTREAM stream;
    HGLOBAL  hglob;
    LPBYTE   data;
    HRESULT  hres;
    GpStatus stat;
    GpImage *img;
    GpRectF bounds;
    GpUnit unit;
    REAL res = 12345.0;
    MetafileHeader header;

    hglob = GlobalAlloc (0, sizeof(wmfimage));
    data = GlobalLock (hglob);
    memcpy(data, wmfimage, sizeof(wmfimage));
    GlobalUnlock(hglob); data = NULL;

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream\n");
    if(hres != S_OK) return;

    stat = GdipLoadImageFromStream(stream, &img);
    ok(stat == Ok, "Failed to create a Bitmap\n");
    if(stat != Ok){
        IStream_Release(stream);
        return;
    }

    IStream_Release(stream);

    stat = GdipGetImageBounds(img, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf(320.0, bounds.Width);
    expectf(320.0, bounds.Height);

    stat = GdipGetImageHorizontalResolution(img, &res);
    expect(Ok, stat);
    expectf(1440.0, res);

    stat = GdipGetImageVerticalResolution(img, &res);
    expect(Ok, stat);
    expectf(1440.0, res);

    memset(&header, 0, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile((GpMetafile*)img, &header);
    expect(Ok, stat);
    if (stat == Ok)
    {
        expect(MetafileTypeWmfPlaceable, header.Type);
        todo_wine expect(sizeof(wmfimage)-sizeof(WmfPlaceableFileHeader), header.Size);
        todo_wine expect(0x300, header.Version);
        expect(0, header.EmfPlusFlags);
        expectf(1440.0, header.DpiX);
        expectf(1440.0, header.DpiY);
        expect(0, header.X);
        expect(0, header.Y);
        expect(320, header.Width);
        expect(320, header.Height);
        expect(1, U(header).WmfHeader.mtType);
        expect(0, header.EmfPlusHeaderSize);
        expect(0, header.LogicalDpiX);
        expect(0, header.LogicalDpiY);
    }

    GdipDisposeImage(img);
}

static void test_createfromwmf(void)
{
    HMETAFILE hwmf;
    GpImage *img;
    GpStatus stat;
    GpRectF bounds;
    GpUnit unit;
    REAL res = 12345.0;
    MetafileHeader header;

    hwmf = SetMetaFileBitsEx(sizeof(wmfimage)-sizeof(WmfPlaceableFileHeader),
        wmfimage+sizeof(WmfPlaceableFileHeader));
    ok(hwmf != 0, "SetMetaFileBitsEx failed\n");

    stat = GdipCreateMetafileFromWmf(hwmf, TRUE,
        (WmfPlaceableFileHeader*)wmfimage, (GpMetafile**)&img);
    expect(Ok, stat);

    stat = GdipGetImageBounds(img, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf(320.0, bounds.Width);
    expectf(320.0, bounds.Height);

    stat = GdipGetImageHorizontalResolution(img, &res);
    expect(Ok, stat);
    expectf(1440.0, res);

    stat = GdipGetImageVerticalResolution(img, &res);
    expect(Ok, stat);
    expectf(1440.0, res);

    memset(&header, 0, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile((GpMetafile*)img, &header);
    expect(Ok, stat);
    if (stat == Ok)
    {
        expect(MetafileTypeWmfPlaceable, header.Type);
        todo_wine expect(sizeof(wmfimage)-sizeof(WmfPlaceableFileHeader), header.Size);
        todo_wine expect(0x300, header.Version);
        expect(0, header.EmfPlusFlags);
        expectf(1440.0, header.DpiX);
        expectf(1440.0, header.DpiY);
        expect(0, header.X);
        expect(0, header.Y);
        expect(320, header.Width);
        expect(320, header.Height);
        expect(1, U(header).WmfHeader.mtType);
        expect(0, header.EmfPlusHeaderSize);
        expect(0, header.LogicalDpiX);
        expect(0, header.LogicalDpiY);
    }

    GdipDisposeImage(img);
}

static void test_createfromwmf_noplaceable(void)
{
    HMETAFILE hwmf;
    GpImage *img;
    GpStatus stat;

    hwmf = SetMetaFileBitsEx(sizeof(wmfimage)-sizeof(WmfPlaceableFileHeader),
        wmfimage+sizeof(WmfPlaceableFileHeader));
    ok(hwmf != 0, "SetMetaFileBitsEx failed\n");

    stat = GdipCreateMetafileFromWmf(hwmf, TRUE, NULL, (GpMetafile**)&img);
    expect(Ok, stat);

    GdipDisposeImage(img);
}

static void test_resolution(void)
{
    GpStatus stat;
    GpBitmap *bitmap;
    GpGraphics *graphics;
    REAL res=-1.0;
    HDC screendc;
    int screenxres, screenyres;

    /* create Bitmap */
    stat = GdipCreateBitmapFromScan0(1, 1, 32, PixelFormat24bppRGB, NULL, &bitmap);
    expect(Ok, stat);

    /* test invalid values */
    stat = GdipGetImageHorizontalResolution(NULL, &res);
    expect(InvalidParameter, stat);

    stat = GdipGetImageHorizontalResolution((GpImage*)bitmap, NULL);
    expect(InvalidParameter, stat);

    stat = GdipGetImageVerticalResolution(NULL, &res);
    expect(InvalidParameter, stat);

    stat = GdipGetImageVerticalResolution((GpImage*)bitmap, NULL);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetResolution(NULL, 96.0, 96.0);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetResolution(bitmap, 0.0, 0.0);
    expect(InvalidParameter, stat);

    /* defaults to screen resolution */
    screendc = GetDC(0);

    screenxres = GetDeviceCaps(screendc, LOGPIXELSX);
    screenyres = GetDeviceCaps(screendc, LOGPIXELSY);

    ReleaseDC(0, screendc);

    stat = GdipGetImageHorizontalResolution((GpImage*)bitmap, &res);
    expect(Ok, stat);
    expectf((REAL)screenxres, res);

    stat = GdipGetImageVerticalResolution((GpImage*)bitmap, &res);
    expect(Ok, stat);
    expectf((REAL)screenyres, res);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);
    stat = GdipGetDpiX(graphics, &res);
    expect(Ok, stat);
    expectf((REAL)screenxres, res);
    stat = GdipGetDpiY(graphics, &res);
    expect(Ok, stat);
    expectf((REAL)screenyres, res);

    /* test changing the resolution */
    stat = GdipBitmapSetResolution(bitmap, screenxres*2.0, screenyres*3.0);
    expect(Ok, stat);

    stat = GdipGetImageHorizontalResolution((GpImage*)bitmap, &res);
    expect(Ok, stat);
    expectf(screenxres*2.0, res);

    stat = GdipGetImageVerticalResolution((GpImage*)bitmap, &res);
    expect(Ok, stat);
    expectf(screenyres*3.0, res);

    stat = GdipGetDpiX(graphics, &res);
    expect(Ok, stat);
    expectf((REAL)screenxres, res);
    stat = GdipGetDpiY(graphics, &res);
    expect(Ok, stat);
    expectf((REAL)screenyres, res);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);
    stat = GdipGetDpiX(graphics, &res);
    expect(Ok, stat);
    expectf(screenxres*2.0, res);
    stat = GdipGetDpiY(graphics, &res);
    expect(Ok, stat);
    expectf(screenyres*3.0, res);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);
}

static void test_createhbitmap(void)
{
    GpStatus stat;
    GpBitmap *bitmap;
    HBITMAP hbitmap, oldhbitmap;
    BITMAP bm;
    int ret;
    HDC hdc;
    COLORREF pixel;
    BYTE bits[640];
    BitmapData lockeddata;

    memset(bits, 0x68, 640);

    /* create Bitmap */
    stat = GdipCreateBitmapFromScan0(10, 20, 32, PixelFormat24bppRGB, bits, &bitmap);
    expect(Ok, stat);

    /* test NULL values */
    stat = GdipCreateHBITMAPFromBitmap(NULL, &hbitmap, 0);
    expect(InvalidParameter, stat);

    stat = GdipCreateHBITMAPFromBitmap(bitmap, NULL, 0);
    expect(InvalidParameter, stat);

    /* create HBITMAP */
    stat = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0);
    expect(Ok, stat);

    if (stat == Ok)
    {
        ret = GetObjectA(hbitmap, sizeof(BITMAP), &bm);
        expect(sizeof(BITMAP), ret);

        expect(0, bm.bmType);
        expect(10, bm.bmWidth);
        expect(20, bm.bmHeight);
        expect(40, bm.bmWidthBytes);
        expect(1, bm.bmPlanes);
        expect(32, bm.bmBitsPixel);
        ok(bm.bmBits != NULL, "got DDB, expected DIB\n");

        if (bm.bmBits)
        {
            DWORD val = *(DWORD*)bm.bmBits;
            ok(val == 0xff686868, "got %x, expected 0xff686868\n", val);
        }

        hdc = CreateCompatibleDC(NULL);

        oldhbitmap = SelectObject(hdc, hbitmap);
        pixel = GetPixel(hdc, 5, 5);
        SelectObject(hdc, oldhbitmap);

        DeleteDC(hdc);

        expect(0x686868, pixel);

        DeleteObject(hbitmap);
    }

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    /* make (1,0) have no alpha and (2,0) a different blue value. */
    bits[7] = 0x00;
    bits[8] = 0x40;

    /* create alpha Bitmap */
    stat = GdipCreateBitmapFromScan0(8, 20, 32, PixelFormat32bppARGB, bits, &bitmap);
    expect(Ok, stat);

    /* create HBITMAP */
    stat = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0);
    expect(Ok, stat);

    if (stat == Ok)
    {
        ret = GetObjectA(hbitmap, sizeof(BITMAP), &bm);
        expect(sizeof(BITMAP), ret);

        expect(0, bm.bmType);
        expect(8, bm.bmWidth);
        expect(20, bm.bmHeight);
        expect(32, bm.bmWidthBytes);
        expect(1, bm.bmPlanes);
        expect(32, bm.bmBitsPixel);
        ok(bm.bmBits != NULL, "got DDB, expected DIB\n");

        if (bm.bmBits)
        {
            DWORD val = *(DWORD*)bm.bmBits;
            ok(val == 0x682a2a2a, "got %x, expected 0x682a2a2a\n", val);
            val = *((DWORD*)bm.bmBits + (bm.bmHeight-1) * bm.bmWidthBytes/4 + 1);
            ok(val == 0x0, "got %x, expected 0x682a2a2a\n", val);
        }

        hdc = CreateCompatibleDC(NULL);

        oldhbitmap = SelectObject(hdc, hbitmap);
        pixel = GetPixel(hdc, 5, 5);
        expect(0x2a2a2a, pixel);
        pixel = GetPixel(hdc, 1, 0);
        expect(0x0, pixel);

        SelectObject(hdc, oldhbitmap);

        DeleteDC(hdc);


        DeleteObject(hbitmap);
    }

    /* create HBITMAP with bkgnd colour */
    /* gdiplus.dll 5.1 is broken and only applies the blue value */
    stat = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0xff00ff);
    expect(Ok, stat);

    if (stat == Ok)
    {
        ret = GetObjectA(hbitmap, sizeof(BITMAP), &bm);
        expect(sizeof(BITMAP), ret);

        expect(0, bm.bmType);
        expect(8, bm.bmWidth);
        expect(20, bm.bmHeight);
        expect(32, bm.bmWidthBytes);
        expect(1, bm.bmPlanes);
        expect(32, bm.bmBitsPixel);
        ok(bm.bmBits != NULL, "got DDB, expected DIB\n");

        if (bm.bmBits)
        {
            DWORD val = *(DWORD*)bm.bmBits;
            ok(val == 0x68c12ac1 || broken(val == 0x682a2ac1), "got %x, expected 0x68c12ac1\n", val);
            val = *((DWORD*)bm.bmBits + (bm.bmHeight-1) * bm.bmWidthBytes/4 + 1);
            ok(val == 0xff00ff || broken(val == 0xff), "got %x, expected 0xff00ff\n", val);
        }

        hdc = CreateCompatibleDC(NULL);

        oldhbitmap = SelectObject(hdc, hbitmap);
        pixel = GetPixel(hdc, 5, 5);
        ok(pixel == 0xc12ac1 || broken(pixel == 0xc12a2a), "got %x, expected 0xc12ac1\n", pixel);
        pixel = GetPixel(hdc, 1, 0);
        ok(pixel == 0xff00ff || broken(pixel == 0xff0000), "got %x, expected 0xff00ff\n", pixel);
        pixel = GetPixel(hdc, 2, 0);
        ok(pixel == 0xb12ac1 || broken(pixel == 0xb12a2a), "got %x, expected 0xb12ac1\n", pixel);

        SelectObject(hdc, oldhbitmap);
        DeleteDC(hdc);
        DeleteObject(hbitmap);
    }

    /* create HBITMAP with bkgnd colour with alpha and show it behaves with no alpha. */
    stat = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0x80ff00ff);
    expect(Ok, stat);

    if (stat == Ok)
    {
        ret = GetObjectA(hbitmap, sizeof(BITMAP), &bm);
        expect(sizeof(BITMAP), ret);

        expect(0, bm.bmType);
        expect(8, bm.bmWidth);
        expect(20, bm.bmHeight);
        expect(32, bm.bmWidthBytes);
        expect(1, bm.bmPlanes);
        expect(32, bm.bmBitsPixel);
        ok(bm.bmBits != NULL, "got DDB, expected DIB\n");

        if (bm.bmBits)
        {
            DWORD val = *(DWORD*)bm.bmBits;
            ok(val == 0x68c12ac1 || broken(val == 0x682a2ac1), "got %x, expected 0x68c12ac1\n", val);
            val = *((DWORD*)bm.bmBits + (bm.bmHeight-1) * bm.bmWidthBytes/4 + 1);
            ok(val == 0xff00ff || broken(val == 0xff), "got %x, expected 0xff00ff\n", val);
        }

        hdc = CreateCompatibleDC(NULL);

        oldhbitmap = SelectObject(hdc, hbitmap);
        pixel = GetPixel(hdc, 5, 5);
        ok(pixel == 0xc12ac1 || broken(pixel == 0xc12a2a), "got %x, expected 0xc12ac1\n", pixel);
        pixel = GetPixel(hdc, 1, 0);
        ok(pixel == 0xff00ff || broken(pixel == 0xff0000), "got %x, expected 0xff00ff\n", pixel);
        pixel = GetPixel(hdc, 2, 0);
        ok(pixel == 0xb12ac1 || broken(pixel == 0xb12a2a), "got %x, expected 0xb12ac1\n", pixel);

        SelectObject(hdc, oldhbitmap);
        DeleteDC(hdc);
        DeleteObject(hbitmap);
    }

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    /* create HBITMAP from locked data */
    memset(bits, 0x68, 640);
    stat = GdipCreateBitmapFromScan0(10, 20, 32, PixelFormat24bppRGB, bits, &bitmap);
    expect(Ok, stat);

    memset(&lockeddata, 0, sizeof(lockeddata));
    stat = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead | ImageLockModeWrite,
            PixelFormat32bppRGB, &lockeddata);
    expect(Ok, stat);
    ((DWORD*)lockeddata.Scan0)[0] = 0xff242424;
    stat = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0);
    expect(Ok, stat);
    stat = GdipBitmapUnlockBits(bitmap, &lockeddata);
    expect(Ok, stat);
    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    hdc = CreateCompatibleDC(NULL);
    oldhbitmap = SelectObject(hdc, hbitmap);
    pixel = GetPixel(hdc, 0, 0);
    expect(0x686868, pixel);
    SelectObject(hdc, oldhbitmap);
    DeleteDC(hdc);
}

static void test_getthumbnail(void)
{
    GpStatus stat;
    GpImage *bitmap1, *bitmap2;
    UINT width, height;

    stat = GdipGetImageThumbnail(NULL, 0, 0, &bitmap2, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromScan0(128, 128, 0, PixelFormat32bppRGB, NULL, (GpBitmap**)&bitmap1);
    expect(Ok, stat);

    stat = GdipGetImageThumbnail(bitmap1, 0, 0, NULL, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipGetImageThumbnail(bitmap1, 0, 0, &bitmap2, NULL, NULL);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageWidth(bitmap2, &width);
        expect(Ok, stat);
        expect(120, width);

        stat = GdipGetImageHeight(bitmap2, &height);
        expect(Ok, stat);
        expect(120, height);

        GdipDisposeImage(bitmap2);
    }

    GdipDisposeImage(bitmap1);


    stat = GdipCreateBitmapFromScan0(64, 128, 0, PixelFormat32bppRGB, NULL, (GpBitmap**)&bitmap1);
    expect(Ok, stat);

    stat = GdipGetImageThumbnail(bitmap1, 32, 32, &bitmap2, NULL, NULL);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageWidth(bitmap2, &width);
        expect(Ok, stat);
        expect(32, width);

        stat = GdipGetImageHeight(bitmap2, &height);
        expect(Ok, stat);
        expect(32, height);

        GdipDisposeImage(bitmap2);
    }

    stat = GdipGetImageThumbnail(bitmap1, 0, 0, &bitmap2, NULL, NULL);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipGetImageWidth(bitmap2, &width);
        expect(Ok, stat);
        expect(120, width);

        stat = GdipGetImageHeight(bitmap2, &height);
        expect(Ok, stat);
        expect(120, height);

        GdipDisposeImage(bitmap2);
    }

    GdipDisposeImage(bitmap1);
}

static void test_getsetpixel(void)
{
    GpStatus stat;
    GpBitmap *bitmap;
    ARGB color;
    BYTE bits[16] = {0x00,0x00,0x00,0x00, 0x00,0xff,0xff,0x00,
                     0xff,0x00,0x00,0x00, 0xff,0xff,0xff,0x00};

    stat = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat32bppRGB, bits, &bitmap);
    expect(Ok, stat);

    /* null parameters */
    stat = GdipBitmapGetPixel(NULL, 1, 1, &color);
    expect(InvalidParameter, stat);

    stat = GdipBitmapGetPixel(bitmap, 1, 1, NULL);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetPixel(NULL, 1, 1, 0);
    expect(InvalidParameter, stat);

    /* out of bounds */
    stat = GdipBitmapGetPixel(bitmap, -1, 1, &color);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetPixel(bitmap, -1, 1, 0);
    expect(InvalidParameter, stat);

    stat = GdipBitmapGetPixel(bitmap, 1, -1, &color);
    ok(stat == InvalidParameter ||
       broken(stat == Ok), /* Older gdiplus */
       "Expected InvalidParameter, got %.8x\n", stat);

if (0) /* crashes some gdiplus implementations */
{
    stat = GdipBitmapSetPixel(bitmap, 1, -1, 0);
    ok(stat == InvalidParameter ||
       broken(stat == Ok), /* Older gdiplus */
       "Expected InvalidParameter, got %.8x\n", stat);
}

    stat = GdipBitmapGetPixel(bitmap, 2, 1, &color);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetPixel(bitmap, 2, 1, 0);
    expect(InvalidParameter, stat);

    stat = GdipBitmapGetPixel(bitmap, 1, 2, &color);
    expect(InvalidParameter, stat);

    stat = GdipBitmapSetPixel(bitmap, 1, 2, 0);
    expect(InvalidParameter, stat);

    /* valid use */
    stat = GdipBitmapGetPixel(bitmap, 1, 1, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

    stat = GdipBitmapGetPixel(bitmap, 0, 1, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapSetPixel(bitmap, 1, 1, 0xff676869);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap, 0, 0, 0xff474849);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap, 1, 1, &color);
    expect(Ok, stat);
    expect(0xff676869, color);

    stat = GdipBitmapGetPixel(bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff474849, color);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);
}

static void check_halftone_palette(ColorPalette *palette)
{
    static const BYTE halftone_values[6]={0x00,0x33,0x66,0x99,0xcc,0xff};
    UINT i;

    for (i=0; i<palette->Count; i++)
    {
        ARGB expected=0xff000000;
        if (i<8)
        {
            if (i&1) expected |= 0x800000;
            if (i&2) expected |= 0x8000;
            if (i&4) expected |= 0x80;
        }
        else if (i == 8)
        {
            expected = 0xffc0c0c0;
        }
        else if (i < 16)
        {
            if (i&1) expected |= 0xff0000;
            if (i&2) expected |= 0xff00;
            if (i&4) expected |= 0xff;
        }
        else if (i < 40)
        {
            expected = 0x00000000;
        }
        else
        {
            expected |= halftone_values[(i-40)%6];
            expected |= halftone_values[((i-40)/6)%6] << 8;
            expected |= halftone_values[((i-40)/36)%6] << 16;
        }
        ok(expected == palette->Entries[i], "Expected %.8x, got %.8x, i=%u/%u\n",
            expected, palette->Entries[i], i, palette->Count);
    }
}

static void test_palette(void)
{
    GpStatus stat;
    GpBitmap *bitmap;
    INT size;
    BYTE buffer[1040];
    ColorPalette *palette=(ColorPalette*)buffer;
    ARGB *entries = palette->Entries;
    ARGB color=0;

    /* test initial palette from non-indexed bitmap */
    stat = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat32bppRGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB), size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(0, palette->Count);

    /* test setting palette on not-indexed bitmap */
    palette->Count = 3;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    expect(Ok, stat);

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*3, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(3, palette->Count);

    GdipDisposeImage((GpImage*)bitmap);

    /* test initial palette on 1-bit bitmap */
    stat = GdipCreateBitmapFromScan0(2, 2, 4, PixelFormat1bppIndexed, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*2, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(PaletteFlagsGrayScale, palette->Flags);
    expect(2, palette->Count);

    expect(0xff000000, entries[0]);
    expect(0xffffffff, entries[1]);

    /* test getting/setting pixels */
    stat = GdipBitmapGetPixel(bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapSetPixel(bitmap, 0, 1, 0xffffffff);
    ok((stat == Ok) ||
       broken(stat == InvalidParameter) /* pre-win7 */, "stat=%.8x\n", stat);

    if (stat == Ok)
    {
        stat = GdipBitmapGetPixel(bitmap, 0, 1, &color);
        expect(Ok, stat);
        expect(0xffffffff, color);
    }

    GdipDisposeImage((GpImage*)bitmap);

    /* test initial palette on 4-bit bitmap */
    stat = GdipCreateBitmapFromScan0(2, 2, 4, PixelFormat4bppIndexed, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*16, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(0, palette->Flags);
    expect(16, palette->Count);

    check_halftone_palette(palette);

    /* test getting/setting pixels */
    stat = GdipBitmapGetPixel(bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapSetPixel(bitmap, 0, 1, 0xffff00ff);
    ok((stat == Ok) ||
       broken(stat == InvalidParameter) /* pre-win7 */, "stat=%.8x\n", stat);

    if (stat == Ok)
    {
        stat = GdipBitmapGetPixel(bitmap, 0, 1, &color);
        expect(Ok, stat);
        expect(0xffff00ff, color);
    }

    GdipDisposeImage((GpImage*)bitmap);

    /* test initial palette on 8-bit bitmap */
    stat = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat8bppIndexed, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*256, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(PaletteFlagsHalftone, palette->Flags);
    expect(256, palette->Count);

    check_halftone_palette(palette);

    /* test getting/setting pixels */
    stat = GdipBitmapGetPixel(bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapSetPixel(bitmap, 0, 1, 0xffcccccc);
    ok((stat == Ok) ||
       broken(stat == InvalidParameter) /* pre-win7 */, "stat=%.8x\n", stat);

    if (stat == Ok)
    {
        stat = GdipBitmapGetPixel(bitmap, 0, 1, &color);
        expect(Ok, stat);
        expect(0xffcccccc, color);
    }

    /* test setting/getting a different palette */
    entries[1] = 0xffcccccc;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    expect(Ok, stat);

    entries[1] = 0;

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*256, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(PaletteFlagsHalftone, palette->Flags);
    expect(256, palette->Count);
    expect(0xffcccccc, entries[1]);

    /* test count < 256 */
    palette->Flags = 12345;
    palette->Count = 3;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    expect(Ok, stat);

    entries[1] = 0;
    entries[3] = 0xdeadbeef;

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*3, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(12345, palette->Flags);
    expect(3, palette->Count);
    expect(0xffcccccc, entries[1]);
    expect(0xdeadbeef, entries[3]);

    /* test count > 256 */
    palette->Count = 257;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    ok(stat == InvalidParameter ||
       broken(stat == Ok), /* Old gdiplus behavior */
       "Expected %.8x, got %.8x\n", InvalidParameter, stat);

    GdipDisposeImage((GpImage*)bitmap);
}

static void test_colormatrix(void)
{
    GpStatus stat;
    ColorMatrix colormatrix, graymatrix;
    GpImageAttributes *imageattr;
    const ColorMatrix identity = {{
        {1.0,0.0,0.0,0.0,0.0},
        {0.0,1.0,0.0,0.0,0.0},
        {0.0,0.0,1.0,0.0,0.0},
        {0.0,0.0,0.0,1.0,0.0},
        {0.0,0.0,0.0,0.0,1.0}}};
    const ColorMatrix double_red = {{
        {2.0,0.0,0.0,0.0,0.0},
        {0.0,1.0,0.0,0.0,0.0},
        {0.0,0.0,1.0,0.0,0.0},
        {0.0,0.0,0.0,1.0,0.0},
        {0.0,0.0,0.0,0.0,1.0}}};
    const ColorMatrix asymmetric = {{
        {0.0,1.0,0.0,0.0,0.0},
        {0.0,0.0,1.0,0.0,0.0},
        {0.0,0.0,0.0,1.0,0.0},
        {1.0,0.0,0.0,0.0,0.0},
        {0.0,0.0,0.0,0.0,1.0}}};
    GpBitmap *bitmap1, *bitmap2;
    GpGraphics *graphics;
    ARGB color;

    colormatrix = identity;
    graymatrix = identity;

    stat = GdipSetImageAttributesColorMatrix(NULL, ColorAdjustTypeDefault,
        TRUE, &colormatrix, &graymatrix, ColorMatrixFlagsDefault);
    expect(InvalidParameter, stat);

    stat = GdipCreateImageAttributes(&imageattr);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, NULL, NULL, ColorMatrixFlagsDefault);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, &graymatrix, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsSkipGrays);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsAltGray);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, &graymatrix, ColorMatrixFlagsAltGray);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, &graymatrix, 3);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeCount,
        TRUE, &colormatrix, &graymatrix, ColorMatrixFlagsDefault);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeAny,
        TRUE, &colormatrix, &graymatrix, ColorMatrixFlagsDefault);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        FALSE, NULL, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    /* Drawing a bitmap transforms the colors */
    colormatrix = double_red;
    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppARGB, NULL, &bitmap1);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppARGB, NULL, &bitmap2);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 0, 0, 0xff40ccee);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap2, &graphics);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff80ccee, color);

    colormatrix = asymmetric;
    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap2, 0, 0, 0);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xeeff40cc, color, 3), "expected 0xeeff40cc, got 0x%08x\n", color);

    /* Toggle NoOp */
    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, FALSE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xfefe40cc, color, 3), "expected 0xfefe40cc, got 0x%08x\n", color);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, TRUE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, FALSE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 1), "Expected ff40ccee, got %.8x\n", color);

    /* Disable adjustment, toggle NoOp */
    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        FALSE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, FALSE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, TRUE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    /* Reset with NoOp on, enable adjustment. */
    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xfff24ace, color, 3), "expected 0xfff24ace, got 0x%08x\n", color);

    /* Now inhibit specific category. */
    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeBitmap,
        TRUE, &colormatrix, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xfffe41cc, color, 3), "expected 0xfffe41cc, got 0x%08x\n", color);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeBitmap, TRUE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeBitmap, FALSE);
    expect(Ok, stat);

    stat = GdipSetImageAttributesNoOp(imageattr, ColorAdjustTypeDefault, TRUE);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xfff24ace, color, 3), "expected 0xfff24ace, got 0x%08x\n", color);

    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeBitmap);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage *)bitmap1, 0, 0, 1, 1, 0, 0, 1, 1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff40ccee, color, 3), "expected 0xff40ccee, got 0x%08x\n", color);

    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap1);
    GdipDisposeImage((GpImage*)bitmap2);
    GdipDisposeImageAttributes(imageattr);
}

static void test_gamma(void)
{
    GpStatus stat;
    GpImageAttributes *imageattr;
    GpBitmap *bitmap1, *bitmap2;
    GpGraphics *graphics;
    ARGB color;

    stat = GdipSetImageAttributesGamma(NULL, ColorAdjustTypeDefault, TRUE, 1.0);
    expect(InvalidParameter, stat);

    stat = GdipCreateImageAttributes(&imageattr);
    expect(Ok, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, TRUE, 1.0);
    expect(Ok, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeAny, TRUE, 1.0);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, TRUE, -1.0);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, TRUE, 0.0);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, TRUE, 0.5);
    expect(Ok, stat);

    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, FALSE, 0.0);
    expect(Ok, stat);

    /* Drawing a bitmap transforms the colors */
    stat = GdipSetImageAttributesGamma(imageattr, ColorAdjustTypeDefault, TRUE, 3.0);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppRGB, NULL, &bitmap1);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppRGB, NULL, &bitmap2);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 0, 0, 0xff80ffff);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap2, &graphics);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff20ffff, color, 1), "Expected ff20ffff, got %.8x\n", color);

    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff80ffff, color, 1), "Expected ff80ffff, got %.8x\n", color);

    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap1);
    GdipDisposeImage((GpImage*)bitmap2);
    GdipDisposeImageAttributes(imageattr);
}

/* 1x1 pixel gif, 2 frames; first frame is white, second is black */
static const unsigned char gifanimation[72] = {
0x47,0x49,0x46,0x38,0x39,0x61,0x01,0x00,0x01,0x00,0xa1,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0xf9,0x04,0x00,0x0a,0x00,0xff,
0x00,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x4c,0x01,0x00,
0x21,0xf9,0x04,0x01,0x0a,0x00,0x01,0x00,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,
0x00,0x00,0x02,0x02,0x44,0x01,0x00,0x3b
};

/* Generated with ImageMagick:
 * convert -transparent black -delay 100 -size 8x2 xc:black \
 *     -dispose none -page +0+0 -size 2x2 xc:red \
 *     -dispose background -page +2+0 -size 2x2 xc:blue \
 *     -dispose previous -page +4+0 -size 2x2 xc:green \
 *     -dispose undefined -page +6+0 -size 2x2 xc:gray \
 *     test.gif
 */
static const unsigned char gifanimation2[] = {
    0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x08, 0x00,
    0x02, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x21, 0xf9, 0x04, 0x01, 0x64,
    0x00, 0x00, 0x00, 0x21, 0xff, 0x0b, 0x4e, 0x45,
    0x54, 0x53, 0x43, 0x41, 0x50, 0x45, 0x32, 0x2e,
    0x30, 0x03, 0x01, 0x00, 0x00, 0x00, 0x2c, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00,
    0x02, 0x04, 0x84, 0x8f, 0x09, 0x05, 0x00, 0x21,
    0xf9, 0x04, 0x04, 0x64, 0x00, 0x00, 0x00, 0x2c,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x81, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff,
    0x00, 0x00, 0xff, 0x00, 0x00, 0x02, 0x03, 0x44,
    0x34, 0x05, 0x00, 0x21, 0xf9, 0x04, 0x08, 0x64,
    0x00, 0x00, 0x00, 0x2c, 0x02, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x81, 0x00, 0x00, 0xff,
    0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00,
    0xff, 0x02, 0x03, 0x44, 0x34, 0x05, 0x00, 0x21,
    0xf9, 0x04, 0x0c, 0x64, 0x00, 0x00, 0x00, 0x2c,
    0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x81, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x00, 0x02, 0x03, 0x44,
    0x34, 0x05, 0x00, 0x21, 0xf9, 0x04, 0x00, 0x64,
    0x00, 0x00, 0x00, 0x2c, 0x06, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x80, 0x7e, 0x7e, 0x7e,
    0x00, 0x00, 0x00, 0x02, 0x02, 0x84, 0x51, 0x00,
    0x3b
};

static ARGB gifanimation2_pixels[5][4] = {
    {0, 0, 0, 0},
    {0xffff0000, 0, 0, 0},
    {0xffff0000, 0xff0000ff, 0, 0},
    {0xffff0000, 0, 0xff008000, 0},
    {0xffff0000, 0, 0, 0xff7e7e7e}
};

static void test_multiframegif(void)
{
    LPSTREAM stream;
    HGLOBAL hglob;
    LPBYTE data;
    HRESULT hres;
    GpStatus stat;
    GpBitmap *bmp;
    ARGB color;
    UINT count;
    GUID dimension;
    PixelFormat pixel_format;
    INT palette_size, i, j;
    char palette_buf[256];
    ColorPalette *palette;
    ARGB *palette_entries;

    /* Test frame functions with an animated GIF */
    hglob = GlobalAlloc (0, sizeof(gifanimation));
    data = GlobalLock (hglob);
    memcpy(data, gifanimation, sizeof(gifanimation));
    GlobalUnlock(hglob);

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream\n");
    if(hres != S_OK) return;

    stat = GdipCreateBitmapFromStream(stream, &bmp);
    ok(stat == Ok, "Failed to create a Bitmap\n");
    if(stat != Ok){
        IStream_Release(stream);
        return;
    }

    stat = GdipGetImagePixelFormat((GpImage*)bmp, &pixel_format);
    expect(Ok, stat);
    expect(PixelFormat32bppARGB, pixel_format);

    stat = GdipGetImagePaletteSize((GpImage*)bmp, &palette_size);
    expect(Ok, stat);
    ok(palette_size == sizeof(ColorPalette) ||
            broken(palette_size == sizeof(ColorPalette)+sizeof(ARGB[3])),
            "palette_size = %d\n", palette_size);

    /* Bitmap starts at frame 0 */
    color = 0xdeadbeef;
    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

    /* Check that we get correct metadata */
    stat = GdipImageGetFrameDimensionsCount((GpImage*)bmp,&count);
    expect(Ok, stat);
    expect(1, count);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bmp, &dimension, 1);
    expect(Ok, stat);
    expect_guid(&FrameDimensionTime, &dimension, __LINE__, FALSE);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bmp, &dimension, &count);
    expect(Ok, stat);
    expect(2, count);

    /* SelectActiveFrame overwrites our current data */
    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 1);
    expect(Ok, stat);

    color = 0xdeadbeef;
    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 0);
    expect(Ok, stat);

    color = 0xdeadbeef;
    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

    /* Write over the image data */
    stat = GdipBitmapSetPixel(bmp, 0, 0, 0xff000000);
    expect(Ok, stat);

    /* Switching to the same frame does not overwrite our changes */
    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 0);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    /* But switching to another frame and back does */
    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 1);
    expect(Ok, stat);

    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 0);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

    /* rotate/flip discards the information about other frames */
    stat = GdipImageRotateFlip((GpImage*)bmp, Rotate90FlipNone);
    expect(Ok, stat);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bmp, &dimension, &count);
    expect(Ok, stat);
    expect(1, count);

    expect_rawformat(&ImageFormatMemoryBMP, (GpImage*)bmp, __LINE__, FALSE);

    GdipDisposeImage((GpImage*)bmp);
    IStream_Release(stream);

    /* Test with a non-animated gif */
    hglob = GlobalAlloc (0, sizeof(gifimage));
    data = GlobalLock (hglob);
    memcpy(data, gifimage, sizeof(gifimage));
    GlobalUnlock(hglob);

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream\n");
    if(hres != S_OK) return;

    stat = GdipCreateBitmapFromStream(stream, &bmp);
    ok(stat == Ok, "Failed to create a Bitmap\n");
    if(stat != Ok){
        IStream_Release(stream);
        return;
    }

    stat = GdipGetImagePixelFormat((GpImage*)bmp, &pixel_format);
    expect(Ok, stat);
    expect(PixelFormat8bppIndexed, pixel_format);

    /* Check metadata */
    stat = GdipImageGetFrameDimensionsCount((GpImage*)bmp,&count);
    expect(Ok, stat);
    expect(1, count);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bmp, &dimension, 1);
    expect(Ok, stat);
    expect_guid(&FrameDimensionTime, &dimension, __LINE__, FALSE);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bmp, &dimension, &count);
    expect(Ok, stat);
    expect(1, count);

    GdipDisposeImage((GpImage*)bmp);
    IStream_Release(stream);

    /* Test with a non-animated transparent gif */
    hglob = GlobalAlloc (0, sizeof(transparentgif));
    data = GlobalLock (hglob);
    memcpy(data, transparentgif, sizeof(transparentgif));
    GlobalUnlock(hglob);

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream\n");

    stat = GdipCreateBitmapFromStream(stream, &bmp);
    IStream_Release(stream);
    ok(stat == Ok, "Failed to create a Bitmap\n");

    stat = GdipGetImagePixelFormat((GpImage*)bmp, &pixel_format);
    expect(Ok, stat);
    expect(PixelFormat8bppIndexed, pixel_format);

    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipGetImagePaletteSize((GpImage*)bmp, &palette_size);
    expect(Ok, stat);
    ok(palette_size == sizeof(ColorPalette)+sizeof(ARGB),
            "palette_size = %d\n", palette_size);

    memset(palette_buf, 0xfe, sizeof(palette_buf));
    palette = (ColorPalette*)palette_buf;
    stat = GdipGetImagePalette((GpImage*)bmp, palette,
            sizeof(ColorPalette)+sizeof(ARGB));
    palette_entries = palette->Entries;
    expect(Ok, stat);
    expect(PaletteFlagsHasAlpha, palette->Flags);
    expect(2, palette->Count);
    expect(0, palette_entries[0]);
    expect(0xff000000, palette_entries[1]);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bmp, &dimension, &count);
    expect(Ok, stat);
    expect(1, count);

    GdipDisposeImage((GpImage*)bmp);

    /* Test frame dispose methods */
    hglob = GlobalAlloc (0, sizeof(gifanimation2));
    data = GlobalLock (hglob);
    memcpy(data, gifanimation2, sizeof(gifanimation2));
    GlobalUnlock(hglob);

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream\n");

    stat = GdipCreateBitmapFromStream(stream, &bmp);
    ok(stat == Ok, "Failed to create a Bitmap\n");
    IStream_Release(stream);

    stat = GdipImageGetFrameDimensionsList((GpImage*)bmp, &dimension, 1);
    expect(Ok, stat);
    expect_guid(&FrameDimensionTime, &dimension, __LINE__, FALSE);

    stat = GdipImageGetFrameCount((GpImage*)bmp, &dimension, &count);
    expect(Ok, stat);
    expect(5, count);

    stat = GdipBitmapGetPixel(bmp, 0, 0, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, 3);
    expect(Ok, stat);
    stat = GdipBitmapGetPixel(bmp, 2, 0, &color);
    expect(Ok, stat);
    ok(color==0 || broken(color==0xff0000ff), "color = %x\n", color);
    if(color != 0) {
        win_skip("broken animated gif support\n");
        GdipDisposeImage((GpImage*)bmp);
        return;
    }

    for(i=0; i<6; i++) {
        stat = GdipImageSelectActiveFrame((GpImage*)bmp, &dimension, i%5);
        expect(Ok, stat);

        for(j=0; j<4; j++) {
            stat = GdipBitmapGetPixel(bmp, j*2, 0, &color);
            expect(Ok, stat);
            ok(gifanimation2_pixels[i%5][j] == color, "at %d,%d got %x, expected %x\n", i, j, color, gifanimation2_pixels[i%5][j]);
        }
    }

    GdipDisposeImage((GpImage*)bmp);
}

static void test_rotateflip(void)
{
    GpImage *bitmap;
    GpStatus stat;
    BYTE bits[24];
    static const BYTE orig_bits[24] = {
        0,0,0xff,    0,0xff,0,    0xff,0,0,    23,23,23,
        0xff,0xff,0, 0xff,0,0xff, 0,0xff,0xff, 23,23,23};
    UINT width, height;
    ARGB color;

    memcpy(bits, orig_bits, sizeof(bits));
    stat = GdipCreateBitmapFromScan0(3, 2, 12, PixelFormat24bppRGB, bits, (GpBitmap**)&bitmap);
    expect(Ok, stat);

    stat = GdipImageRotateFlip(bitmap, Rotate90FlipNone);
    expect(Ok, stat);

    stat = GdipGetImageWidth(bitmap, &width);
    expect(Ok, stat);
    stat = GdipGetImageHeight(bitmap, &height);
    expect(Ok, stat);
    expect(2, width);
    expect(3, height);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 1, 0, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 2, &color);
    expect(Ok, stat);
    expect(0xffffff00, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 1, 2, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    expect(0, bits[0]);
    expect(0, bits[1]);
    expect(0xff, bits[2]);

    GdipDisposeImage(bitmap);

    memcpy(bits, orig_bits, sizeof(bits));
    stat = GdipCreateBitmapFromScan0(3, 2, 12, PixelFormat24bppRGB, bits, (GpBitmap**)&bitmap);
    expect(Ok, stat);

    stat = GdipImageRotateFlip(bitmap, RotateNoneFlipX);
    expect(Ok, stat);

    stat = GdipGetImageWidth(bitmap, &width);
    expect(Ok, stat);
    stat = GdipGetImageHeight(bitmap, &height);
    expect(Ok, stat);
    expect(3, width);
    expect(2, height);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 2, 0, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 1, &color);
    expect(Ok, stat);
    expect(0xffffff00, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 2, 1, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    expect(0, bits[0]);
    expect(0, bits[1]);
    expect(0xff, bits[2]);

    GdipDisposeImage(bitmap);

    memcpy(bits, orig_bits, sizeof(bits));
    stat = GdipCreateBitmapFromScan0(3, 2, 12, PixelFormat24bppRGB, bits, (GpBitmap**)&bitmap);
    expect(Ok, stat);

    stat = GdipImageRotateFlip(bitmap, RotateNoneFlipY);
    expect(Ok, stat);

    stat = GdipGetImageWidth(bitmap, &width);
    expect(Ok, stat);
    stat = GdipGetImageHeight(bitmap, &height);
    expect(Ok, stat);
    expect(3, width);
    expect(2, height);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 0, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 2, 0, &color);
    expect(Ok, stat);
    expect(0xffffff00, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 0, 1, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel((GpBitmap*)bitmap, 2, 1, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    expect(0, bits[0]);
    expect(0, bits[1]);
    expect(0xff, bits[2]);

    GdipDisposeImage(bitmap);
}

static void test_remaptable(void)
{
    GpStatus stat;
    GpImageAttributes *imageattr;
    GpBitmap *bitmap1, *bitmap2;
    GpGraphics *graphics;
    ARGB color;
    ColorMap *map;

    map = GdipAlloc(sizeof(ColorMap));

    map->oldColor.Argb = 0xff00ff00;
    map->newColor.Argb = 0xffff00ff;

    stat = GdipSetImageAttributesRemapTable(NULL, ColorAdjustTypeDefault, TRUE, 1, map);
    expect(InvalidParameter, stat);

    stat = GdipCreateImageAttributes(&imageattr);
    expect(Ok, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeDefault, TRUE, 1, NULL);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeCount, TRUE, 1, map);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeAny, TRUE, 1, map);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeDefault, TRUE, 0, map);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeDefault, FALSE, 0, NULL);
    expect(Ok, stat);

    stat = GdipSetImageAttributesRemapTable(imageattr, ColorAdjustTypeDefault, TRUE, 1, map);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppRGB, NULL, &bitmap1);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat32bppRGB, NULL, &bitmap2);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 0, 0, 0xff00ff00);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap2, &graphics);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
	UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xffff00ff, color, 1), "Expected ffff00ff, got %.8x\n", color);

    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,1,1, 0,0,1,1,
        UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0xff00ff00, color, 1), "Expected ff00ff00, got %.8x\n", color);

    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap1);
    GdipDisposeImage((GpImage*)bitmap2);
    GdipDisposeImageAttributes(imageattr);
    GdipFree(map);
}

static void test_colorkey(void)
{
    GpStatus stat;
    GpImageAttributes *imageattr;
    GpBitmap *bitmap1, *bitmap2;
    GpGraphics *graphics;
    ARGB color;

    stat = GdipSetImageAttributesColorKeys(NULL, ColorAdjustTypeDefault, TRUE, 0xff405060, 0xff708090);
    expect(InvalidParameter, stat);

    stat = GdipCreateImageAttributes(&imageattr);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorKeys(imageattr, ColorAdjustTypeCount, TRUE, 0xff405060, 0xff708090);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorKeys(imageattr, ColorAdjustTypeAny, TRUE, 0xff405060, 0xff708090);
    expect(InvalidParameter, stat);

    stat = GdipSetImageAttributesColorKeys(imageattr, ColorAdjustTypeDefault, TRUE, 0xff405060, 0xff708090);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(2, 2, 0, PixelFormat32bppARGB, NULL, &bitmap1);
    expect(Ok, stat);

    stat = GdipCreateBitmapFromScan0(2, 2, 0, PixelFormat32bppARGB, NULL, &bitmap2);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 0, 0, 0x20405060);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 0, 1, 0x40506070);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 1, 0, 0x60708090);
    expect(Ok, stat);

    stat = GdipBitmapSetPixel(bitmap1, 1, 1, 0xffffffff);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap2, &graphics);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,2,2, 0,0,2,2,
	UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0x00000000, color, 1), "Expected 00000000, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 0, 1, &color);
    expect(Ok, stat);
    ok(color_match(0x00000000, color, 1), "Expected 00000000, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 1, 0, &color);
    expect(Ok, stat);
    ok(color_match(0x00000000, color, 1), "Expected 00000000, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 1, 1, &color);
    expect(Ok, stat);
    ok(color_match(0xffffffff, color, 1), "Expected ffffffff, got %.8x\n", color);

    stat = GdipResetImageAttributes(imageattr, ColorAdjustTypeDefault);
    expect(Ok, stat);

    stat = GdipDrawImageRectRectI(graphics, (GpImage*)bitmap1, 0,0,2,2, 0,0,2,2,
	UnitPixel, imageattr, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap2, 0, 0, &color);
    expect(Ok, stat);
    ok(color_match(0x20405060, color, 1), "Expected 20405060, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 0, 1, &color);
    expect(Ok, stat);
    ok(color_match(0x40506070, color, 1), "Expected 40506070, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 1, 0, &color);
    expect(Ok, stat);
    ok(color_match(0x60708090, color, 1), "Expected 60708090, got %.8x\n", color);

    stat = GdipBitmapGetPixel(bitmap2, 1, 1, &color);
    expect(Ok, stat);
    ok(color_match(0xffffffff, color, 1), "Expected ffffffff, got %.8x\n", color);


    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap1);
    GdipDisposeImage((GpImage*)bitmap2);
    GdipDisposeImageAttributes(imageattr);
}

static void test_dispose(void)
{
    GpStatus stat;
    GpImage *image;
    char invalid_image[256];

    stat = GdipDisposeImage(NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromScan0(2, 2, 0, PixelFormat32bppARGB, NULL, (GpBitmap**)&image);
    expect(Ok, stat);

    stat = GdipDisposeImage(image);
    expect(Ok, stat);

    stat = GdipDisposeImage(image);
    expect(ObjectBusy, stat);

    memset(invalid_image, 0, 256);
    stat = GdipDisposeImage((GpImage*)invalid_image);
    expect(ObjectBusy, stat);
}

static LONG obj_refcount(void *obj)
{
    IUnknown_AddRef((IUnknown *)obj);
    return IUnknown_Release((IUnknown *)obj);
}

static GpImage *load_image(const BYTE *image_data, UINT image_size, BOOL valid_data, BOOL todo_load)
{
    IStream *stream;
    HGLOBAL hmem;
    BYTE *data;
    HRESULT hr;
    GpStatus status;
    GpImage *image = NULL, *clone;
    ImageType image_type;
    LONG refcount, old_refcount;

    hmem = GlobalAlloc(0, image_size);
    data = GlobalLock(hmem);
    memcpy(data, image_data, image_size);
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#x\n", hr);
    if (hr != S_OK) return NULL;

    refcount = obj_refcount(stream);
    ok(refcount == 1, "expected stream refcount 1, got %d\n", refcount);

    status = GdipLoadImageFromStream(stream, &image);
    todo_wine_if(todo_load)
    if (valid_data)
        ok(status == Ok || broken(status == InvalidParameter), /* XP */
           "GdipLoadImageFromStream error %d\n", status);
    else
        ok(status != Ok, "GdipLoadImageFromStream should fail\n");
    if (status != Ok)
    {
        IStream_Release(stream);
        return NULL;
    }

    status = GdipGetImageType(image, &image_type);
    ok(status == Ok, "GdipGetImageType error %d\n", status);

    refcount = obj_refcount(stream);
    if (image_type == ImageTypeBitmap)
        ok(refcount > 1, "expected stream refcount > 1, got %d\n", refcount);
    else
        ok(refcount == 1, "expected stream refcount 1, got %d\n", refcount);
    old_refcount = refcount;

    status = GdipCloneImage(image, &clone);
    ok(status == Ok, "GdipCloneImage error %d\n", status);
    refcount = obj_refcount(stream);
    ok(refcount == old_refcount, "expected stream refcount %d, got %d\n", old_refcount, refcount);
    status = GdipDisposeImage(clone);
    ok(status == Ok, "GdipDisposeImage error %d\n", status);
    refcount = obj_refcount(stream);
    ok(refcount == old_refcount, "expected stream refcount %d, got %d\n", old_refcount, refcount);

    refcount = IStream_Release(stream);
    if (image_type == ImageTypeBitmap)
        ok(refcount >= 1, "expected stream refcount != 0\n");
    else
        ok(refcount == 0, "expected stream refcount 0, got %d\n", refcount);

    return image;
}

static void test_image_properties(void)
{
    static const struct test_data
    {
        const BYTE *image_data;
        UINT image_size;
        ImageType image_type;
        UINT prop_count;
        UINT prop_count2; /* if win7 behaves differently */
        /* 1st property attributes */
        UINT prop_size;
        UINT prop_size2; /* if win7 behaves differently */
        UINT prop_id;
        UINT prop_id2; /* if win7 behaves differently */
    }
    td[] =
    {
        { pngimage, sizeof(pngimage), ImageTypeBitmap, 4, ~0, 1, 20, 0x5110, 0x132 },
        { jpgimage, sizeof(jpgimage), ImageTypeBitmap, 2, ~0, 128, 0, 0x5090, 0x5091 },
        { tiffimage, sizeof(tiffimage), ImageTypeBitmap, 16, 0, 4, 0, 0xfe, 0 },
        { bmpimage, sizeof(bmpimage), ImageTypeBitmap, 0, 0, 0, 0, 0, 0 },
        { wmfimage, sizeof(wmfimage), ImageTypeMetafile, 0, 0, 0, 0, 0, 0 }
    };
    GpStatus status;
    GpImage *image;
    UINT prop_count, prop_size, i;
    PROPID prop_id[16] = { 0 };
    ImageType image_type;
    union
    {
        PropertyItem data;
        char buf[256];
    } item;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        image = load_image(td[i].image_data, td[i].image_size, TRUE, FALSE);
        if (!image)
        {
            trace("%u: failed to load image data\n", i);
            continue;
        }

        status = GdipGetImageType(image, &image_type);
        ok(status == Ok, "%u: GdipGetImageType error %d\n", i, status);
        ok(td[i].image_type == image_type, "%u: expected image_type %d, got %d\n",
           i, td[i].image_type, image_type);

        status = GdipGetPropertyCount(image, &prop_count);
        ok(status == Ok, "%u: GdipGetPropertyCount error %d\n", i, status);
        todo_wine_if(td[i].image_data == pngimage || td[i].image_data == jpgimage)
        ok(td[i].prop_count == prop_count || td[i].prop_count2 == prop_count,
           " %u: expected property count %u or %u, got %u\n",
           i, td[i].prop_count, td[i].prop_count2, prop_count);

        status = GdipGetPropertyItemSize(NULL, 0, &prop_size);
        expect(InvalidParameter, status);
        status = GdipGetPropertyItemSize(image, 0, NULL);
        expect(InvalidParameter, status);
        status = GdipGetPropertyItemSize(image, 0, &prop_size);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else
            expect(PropertyNotFound, status);

        status = GdipGetPropertyItem(NULL, 0, 0, &item.data);
        expect(InvalidParameter, status);
        status = GdipGetPropertyItem(image, 0, 0, NULL);
        expect(InvalidParameter, status);
        status = GdipGetPropertyItem(image, 0, 0, &item.data);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else
            expect(PropertyNotFound, status);

        /* FIXME: remove once Wine is fixed */
        if (td[i].prop_count != prop_count)
        {
            GdipDisposeImage(image);
            continue;
        }

        status = GdipGetPropertyIdList(NULL, prop_count, prop_id);
        expect(InvalidParameter, status);
        status = GdipGetPropertyIdList(image, prop_count, NULL);
        expect(InvalidParameter, status);
        status = GdipGetPropertyIdList(image, 0, prop_id);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else if (prop_count == 0)
            expect(Ok, status);
        else
            expect(InvalidParameter, status);
        status = GdipGetPropertyIdList(image, prop_count - 1, prop_id);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else
            expect(InvalidParameter, status);
        status = GdipGetPropertyIdList(image, prop_count + 1, prop_id);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else
            expect(InvalidParameter, status);
        status = GdipGetPropertyIdList(image, prop_count, prop_id);
        if (image_type == ImageTypeMetafile)
            expect(NotImplemented, status);
        else
        {
            expect(Ok, status);
            if (prop_count != 0)
                ok(td[i].prop_id == prop_id[0] || td[i].prop_id2 == prop_id[0],
                   " %u: expected property id %#x or %#x, got %#x\n",
                   i, td[i].prop_id, td[i].prop_id2, prop_id[0]);
        }

        if (status == Ok)
        {
            status = GdipGetPropertyItemSize(image, prop_id[0], &prop_size);
            if (prop_count == 0)
                expect(PropertyNotFound, status);
            else
            {
                expect(Ok, status);

                assert(sizeof(item) >= prop_size);
                ok(prop_size > sizeof(PropertyItem), "%u: got too small prop_size %u\n",
                   i, prop_size);
                ok(td[i].prop_size + sizeof(PropertyItem) == prop_size ||
                   td[i].prop_size2 + sizeof(PropertyItem) == prop_size,
                   " %u: expected property size %u or %u, got %u\n",
                   i, td[i].prop_size, td[i].prop_size2, prop_size);

                status = GdipGetPropertyItem(image, prop_id[0], 0, &item.data);
                ok(status == InvalidParameter || status == GenericError /* Win7 */,
                   "%u: expected InvalidParameter, got %d\n", i, status);
                status = GdipGetPropertyItem(image, prop_id[0], prop_size - 1, &item.data);
                ok(status == InvalidParameter || status == GenericError /* Win7 */,
                   "%u: expected InvalidParameter, got %d\n", i, status);
                status = GdipGetPropertyItem(image, prop_id[0], prop_size + 1, &item.data);
                ok(status == InvalidParameter || status == GenericError /* Win7 */,
                   "%u: expected InvalidParameter, got %d\n", i, status);
                status = GdipGetPropertyItem(image, prop_id[0], prop_size, &item.data);
                expect(Ok, status);
                ok(prop_id[0] == item.data.id,
                   "%u: expected property id %#x, got %#x\n", i, prop_id[0], item.data.id);
            }
        }

        GdipDisposeImage(image);
    }
}

#define IFD_BYTE      1
#define IFD_ASCII     2
#define IFD_SHORT     3
#define IFD_LONG      4
#define IFD_RATIONAL  5
#define IFD_SBYTE     6
#define IFD_UNDEFINED 7
#define IFD_SSHORT    8
#define IFD_SLONG     9
#define IFD_SRATIONAL 10
#define IFD_FLOAT     11
#define IFD_DOUBLE    12

#ifndef PropertyTagTypeSByte
#define PropertyTagTypeSByte  6
#define PropertyTagTypeSShort 8
#define PropertyTagTypeFloat  11
#define PropertyTagTypeDouble 12
#endif

static UINT documented_type(UINT type)
{
    switch (type)
    {
    case PropertyTagTypeSByte: return PropertyTagTypeByte;
    case PropertyTagTypeSShort: return PropertyTagTypeShort;
    case PropertyTagTypeFloat: return PropertyTagTypeUndefined;
    case PropertyTagTypeDouble: return PropertyTagTypeUndefined;
    default: return type;
    }
}

#include "pshpack2.h"
struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG count;
    LONG  value;
};

struct IFD_rational
{
    LONG numerator;
    LONG denominator;
};

static const struct tiff_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[40];
    ULONG next_IFD;
    struct IFD_rational xres;
    DOUBLE double_val;
    struct IFD_rational srational_val;
    char string[14];
    SHORT short_val[4];
    LONG long_val[2];
    FLOAT float_val[2];
    struct IFD_rational rational[3];
    BYTE pixel_data[4];
} TIFF_data =
{
#ifdef WORDS_BIGENDIAN
    'M' | 'M' << 8,
#else
    'I' | 'I' << 8,
#endif
    42,
    FIELD_OFFSET(struct tiff_data, number_of_entries),
    31,
    {
        { 0xff,  IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 1 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 1 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 1 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 1 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_data, xres) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_data, xres) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0xf001, IFD_BYTE, 1, 0x11223344 },
        { 0xf002, IFD_BYTE, 4, 0x11223344 },
        { 0xf003, IFD_SBYTE, 1, 0x11223344 },
        { 0xf004, IFD_SSHORT, 1, 0x11223344 },
        { 0xf005, IFD_SSHORT, 2, 0x11223344 },
        { 0xf006, IFD_SLONG, 1, 0x11223344 },
        { 0xf007, IFD_FLOAT, 1, 0x11223344 },
        { 0xf008, IFD_DOUBLE, 1, FIELD_OFFSET(struct tiff_data, double_val) },
        { 0xf009, IFD_SRATIONAL, 1, FIELD_OFFSET(struct tiff_data, srational_val) },
        { 0xf00a, IFD_BYTE, 13, FIELD_OFFSET(struct tiff_data, string) },
        { 0xf00b, IFD_SSHORT, 4, FIELD_OFFSET(struct tiff_data, short_val) },
        { 0xf00c, IFD_SLONG, 2, FIELD_OFFSET(struct tiff_data, long_val) },
        { 0xf00e, IFD_ASCII, 13, FIELD_OFFSET(struct tiff_data, string) },
        { 0xf00f, IFD_ASCII, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        { 0xf010, IFD_UNDEFINED, 13, FIELD_OFFSET(struct tiff_data, string) },
        { 0xf011, IFD_UNDEFINED, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        /* Some gdiplus versions ignore these fields.
        { 0xf012, IFD_BYTE, 0, 0x11223344 },
        { 0xf013, IFD_SHORT, 0, 0x11223344 },
        { 0xf014, IFD_LONG, 0, 0x11223344 },
        { 0xf015, IFD_FLOAT, 0, 0x11223344 },*/
        { 0xf016, IFD_SRATIONAL, 3, FIELD_OFFSET(struct tiff_data, rational) },
        /* Win7 before SP1 doesn't recognize this field, everybody else does. */
        { 0xf017, IFD_FLOAT, 2, FIELD_OFFSET(struct tiff_data, float_val) },
    },
    0,
    { 900, 3 },
    1234567890.0987654321,
    { 0x1a2b3c4d, 0x5a6b7c8d },
    "Hello World!",
    { 0x0101, 0x0202, 0x0303, 0x0404 },
    { 0x11223344, 0x55667788 },
    { (FLOAT)1234.5678, (FLOAT)8765.4321 },
    { { 0x01020304, 0x05060708 }, { 0x10203040, 0x50607080 }, { 0x11223344, 0x55667788 } },
    { 0x11, 0x22, 0x33, 0 }
};
#include "poppack.h"

static void test_tiff_properties(void)
{
    static const struct test_data
    {
        ULONG type, id, length;
        const BYTE value[24];
    } td[31] =
    {
        { PropertyTagTypeShort, 0xff, 2, { 0 } },
        { PropertyTagTypeLong, 0x100, 4, { 1 } },
        { PropertyTagTypeLong, 0x101, 4, { 1 } },
        { PropertyTagTypeShort, 0x102, 2, { 1 } },
        { PropertyTagTypeShort, 0x103, 2, { 1 } },
        { PropertyTagTypeShort, 0x106, 2, { 1 } },
        { PropertyTagTypeLong, 0x111, 4, { 0x44,0x02 } },
        { PropertyTagTypeShort, 0x115, 2, { 1 } },
        { PropertyTagTypeLong, 0x116, 4, { 1 } },
        { PropertyTagTypeLong, 0x117, 4, { 1 } },
        { PropertyTagTypeRational, 0x11a, 8, { 0x84,0x03,0,0,0x03 } },
        { PropertyTagTypeRational, 0x11b, 8, { 0x84,0x03,0,0,0x03 } },
        { PropertyTagTypeShort, 0x128, 2, { 2 } },
        { PropertyTagTypeByte, 0xf001, 1, { 0x44 } },
        { PropertyTagTypeByte, 0xf002, 4, { 0x44,0x33,0x22,0x11 } },
        { PropertyTagTypeSByte, 0xf003, 1, { 0x44 } },
        { PropertyTagTypeSShort, 0xf004, 2, { 0x44,0x33 } },
        { PropertyTagTypeSShort, 0xf005, 4, { 0x44,0x33,0x22,0x11 } },
        { PropertyTagTypeSLONG, 0xf006, 4, { 0x44,0x33,0x22,0x11 } },
        { PropertyTagTypeFloat, 0xf007, 4, { 0x44,0x33,0x22,0x11 } },
        { PropertyTagTypeDouble, 0xf008, 8, { 0x2c,0x52,0x86,0xb4,0x80,0x65,0xd2,0x41 } },
        { PropertyTagTypeSRational, 0xf009, 8, { 0x4d, 0x3c, 0x2b, 0x1a, 0x8d, 0x7c, 0x6b, 0x5a } },
        { PropertyTagTypeByte, 0xf00a, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { PropertyTagTypeSShort, 0xf00b, 8, { 0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04 } },
        { PropertyTagTypeSLONG, 0xf00c, 8, { 0x44,0x33,0x22,0x11,0x88,0x77,0x66,0x55 } },
        { PropertyTagTypeASCII, 0xf00e, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { PropertyTagTypeASCII, 0xf00f, 5, { 'a','b','c','d' } },
        { PropertyTagTypeUndefined, 0xf010, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { PropertyTagTypeUndefined, 0xf011, 4, { 'a','b','c','d' } },
        { PropertyTagTypeSRational, 0xf016, 24,
          { 0x04,0x03,0x02,0x01,0x08,0x07,0x06,0x05,
            0x40,0x30,0x20,0x10,0x80,0x70,0x60,0x50,
            0x44,0x33,0x22,0x11,0x88,0x77,0x66,0x55 } },
        /* Win7 before SP1 doesn't recognize this field, everybody else does. */
        { PropertyTagTypeFloat, 0xf017, 8, { 0x2b,0x52,0x9a,0x44,0xba,0xf5,0x08,0x46 } },
    };
    GpStatus status;
    GpImage *image;
    GUID guid;
    UINT dim_count, frame_count, prop_count, prop_size, i;
    PROPID *prop_id;
    PropertyItem *prop_item;

    image = load_image((const BYTE *)&TIFF_data, sizeof(TIFF_data), TRUE, FALSE);
    if (!image)
    {
        win_skip("Failed to load TIFF image data. Might not be supported. Skipping.\n");
        return;
    }

    status = GdipImageGetFrameDimensionsCount(image, &dim_count);
    expect(Ok, status);
    expect(1, dim_count);

    status = GdipImageGetFrameDimensionsList(image, &guid, 1);
    expect(Ok, status);
    expect_guid(&FrameDimensionPage, &guid, __LINE__, FALSE);

    frame_count = 0xdeadbeef;
    status = GdipImageGetFrameCount(image, &guid, &frame_count);
    expect(Ok, status);
    expect(1, frame_count);

    prop_count = 0xdeadbeef;
    status = GdipGetPropertyCount(image, &prop_count);
    expect(Ok, status);
    ok(prop_count == ARRAY_SIZE(td) ||
       broken(prop_count == ARRAY_SIZE(td) - 1) /* Win7 SP0 */,
       "expected property count %u, got %u\n", (UINT) ARRAY_SIZE(td), prop_count);

    prop_id = HeapAlloc(GetProcessHeap(), 0, prop_count * sizeof(*prop_id));

    status = GdipGetPropertyIdList(image, prop_count, prop_id);
    expect(Ok, status);

    for (i = 0; i < prop_count; i++)
    {
        status = GdipGetPropertyItemSize(image, prop_id[i], &prop_size);
        expect(Ok, status);
        if (status != Ok) break;
        ok(prop_size > sizeof(*prop_item), "%u: too small item length %u\n", i, prop_size);

        prop_item = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, prop_size);
        status = GdipGetPropertyItem(image, prop_id[i], prop_size, prop_item);
        expect(Ok, status);
        ok(prop_item->value == prop_item + 1, "expected item->value %p, got %p\n", prop_item + 1, prop_item->value);
        ok(td[i].type == prop_item->type ||
           /* Win7 stopped using proper but not documented types, and it
              looks broken since TypeFloat and TypeDouble now reported as
              TypeUndefined, and signed types reported as unsigned. */
           broken(prop_item->type == documented_type(td[i].type)),
            "%u: expected type %u, got %u\n", i, td[i].type, prop_item->type);
        ok(td[i].id == prop_item->id, "%u: expected id %#x, got %#x\n", i, td[i].id, prop_item->id);
        prop_size -= sizeof(*prop_item);
        ok(prop_item->length == prop_size, "%u: expected length %u, got %u\n", i, prop_size, prop_item->length);
        ok(td[i].length == prop_item->length || broken(td[i].id == 0xf00f && td[i].length == prop_item->length+1) /* XP */,
           "%u: expected length %u, got %u\n", i, td[i].length, prop_item->length);
        ok(td[i].length == prop_size || broken(td[i].id == 0xf00f && td[i].length == prop_size+1) /* XP */,
           "%u: expected length %u, got %u\n", i, td[i].length, prop_size);
        if (td[i].length == prop_item->length)
        {
            int match = memcmp(td[i].value, prop_item->value, td[i].length) == 0;
            ok(match || broken(td[i].length <= 4 && !match), "%u: data mismatch\n", i);
            if (!match)
            {
                UINT j;
                BYTE *data = prop_item->value;
                trace("id %#x:", prop_item->id);
                for (j = 0; j < prop_item->length; j++)
                    trace(" %02x", data[j]);
                trace("\n");
            }
        }
        HeapFree(GetProcessHeap(), 0, prop_item);
    }

    HeapFree(GetProcessHeap(), 0, prop_id);

    GdipDisposeImage(image);
}

static void test_GdipGetAllPropertyItems(void)
{
    static const struct test_data
    {
        ULONG type, id, length;
        BYTE value[32];
    } td[16] =
    {
        { PropertyTagTypeLong, 0xfe, 4, { 0 } },
        { PropertyTagTypeShort, 0x100, 2, { 1 } },
        { PropertyTagTypeShort, 0x101, 2, { 1 } },
        { PropertyTagTypeShort, 0x102, 6, { 8,0,8,0,8,0 } },
        { PropertyTagTypeShort, 0x103, 2, { 1 } },
        { PropertyTagTypeShort, 0x106, 2, { 2,0 } },
        { PropertyTagTypeASCII, 0x10d, 27, "/home/meh/Desktop/test.tif" },
        { PropertyTagTypeLong, 0x111, 4, { 8,0,0,0 } },
        { PropertyTagTypeShort, 0x112, 2, { 1 } },
        { PropertyTagTypeShort, 0x115, 2, { 3,0 } },
        { PropertyTagTypeShort, 0x116, 2, { 0x40,0 } },
        { PropertyTagTypeLong, 0x117, 4, { 3,0,0,0 } },
        { PropertyTagTypeRational, 0x11a, 8, { 0,0,0,72,0,0,0,1 } },
        { PropertyTagTypeRational, 0x11b, 8, { 0,0,0,72,0,0,0,1 } },
        { PropertyTagTypeShort, 0x11c, 2, { 1 } },
        { PropertyTagTypeShort, 0x128, 2, { 2 } }
    };
    GpStatus status;
    GpImage *image;
    GUID guid;
    UINT dim_count, frame_count, prop_count, prop_size, i;
    UINT total_size, total_count;
    PROPID *prop_id;
    PropertyItem *prop_item;
    const char *item_data;

    image = load_image(tiffimage, sizeof(tiffimage), TRUE, FALSE);
    ok(image != 0, "Failed to load TIFF image data\n");
    if (!image) return;

    dim_count = 0xdeadbeef;
    status = GdipImageGetFrameDimensionsCount(image, &dim_count);
    expect(Ok, status);
    expect(1, dim_count);

    status = GdipImageGetFrameDimensionsList(image, &guid, 1);
    expect(Ok, status);
    expect_guid(&FrameDimensionPage, &guid, __LINE__, FALSE);

    frame_count = 0xdeadbeef;
    status = GdipImageGetFrameCount(image, &guid, &frame_count);
    expect(Ok, status);
    expect(1, frame_count);

    prop_count = 0xdeadbeef;
    status = GdipGetPropertyCount(image, &prop_count);
    expect(Ok, status);
    ok(prop_count == ARRAY_SIZE(td),
       "expected property count %u, got %u\n", (UINT) ARRAY_SIZE(td), prop_count);

    prop_id = HeapAlloc(GetProcessHeap(), 0, prop_count * sizeof(*prop_id));

    status = GdipGetPropertyIdList(image, prop_count, prop_id);
    expect(Ok, status);

    prop_size = 0;
    for (i = 0; i < prop_count; i++)
    {
        UINT size;
        status = GdipGetPropertyItemSize(image, prop_id[i], &size);
        expect(Ok, status);
        if (status != Ok) break;
        ok(size > sizeof(*prop_item), "%u: too small item length %u\n", i, size);

        prop_size += size;

        prop_item = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        status = GdipGetPropertyItem(image, prop_id[i], size, prop_item);
        expect(Ok, status);
        ok(prop_item->value == prop_item + 1, "expected item->value %p, got %p\n", prop_item + 1, prop_item->value);
        ok(td[i].type == prop_item->type,
            "%u: expected type %u, got %u\n", i, td[i].type, prop_item->type);
        ok(td[i].id == prop_item->id, "%u: expected id %#x, got %#x\n", i, td[i].id, prop_item->id);
        size -= sizeof(*prop_item);
        ok(prop_item->length == size, "%u: expected length %u, got %u\n", i, size, prop_item->length);
        ok(td[i].length == prop_item->length, "%u: expected length %u, got %u\n", i, td[i].length, prop_item->length);
        if (td[i].length == prop_item->length)
        {
            int match = memcmp(td[i].value, prop_item->value, td[i].length) == 0;
            ok(match, "%u: data mismatch\n", i);
            if (!match)
            {
                UINT j;
                BYTE *data = prop_item->value;
                trace("id %#x:", prop_item->id);
                for (j = 0; j < prop_item->length; j++)
                    trace(" %02x", data[j]);
                trace("\n");
            }
        }
        HeapFree(GetProcessHeap(), 0, prop_item);
    }

    HeapFree(GetProcessHeap(), 0, prop_id);

    status = GdipGetPropertySize(NULL, &total_size, &total_count);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, &total_size, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, NULL, &total_count);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, NULL, NULL);
    expect(InvalidParameter, status);
    total_size = 0xdeadbeef;
    total_count = 0xdeadbeef;
    status = GdipGetPropertySize(image, &total_size, &total_count);
    expect(Ok, status);
    ok(prop_count == total_count,
       "expected total property count %u, got %u\n", prop_count, total_count);
    ok(prop_size == total_size,
       "expected total property size %u, got %u\n", prop_size, total_size);

    prop_item = HeapAlloc(GetProcessHeap(), 0, prop_size);

    status = GdipGetAllPropertyItems(image, 0, prop_count, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, 1, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, 0, 0, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size + 1, prop_count, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, prop_item);
    expect(Ok, status);

    item_data = (const char *)(prop_item + prop_count);
    for (i = 0; i < prop_count; i++)
    {
        ok(prop_item[i].value == item_data, "%u: expected value %p, got %p\n",
           i, item_data, prop_item[i].value);
        ok(td[i].type == prop_item[i].type,
            "%u: expected type %u, got %u\n", i, td[i].type, prop_item[i].type);
        ok(td[i].id == prop_item[i].id, "%u: expected id %#x, got %#x\n", i, td[i].id, prop_item[i].id);
        ok(td[i].length == prop_item[i].length, "%u: expected length %u, got %u\n", i, td[i].length, prop_item[i].length);
        if (td[i].length == prop_item[i].length)
        {
            int match = memcmp(td[i].value, prop_item[i].value, td[i].length) == 0;
            ok(match, "%u: data mismatch\n", i);
            if (!match)
            {
                UINT j;
                BYTE *data = prop_item[i].value;
                trace("id %#x:", prop_item[i].id);
                for (j = 0; j < prop_item[i].length; j++)
                    trace(" %02x", data[j]);
                trace("\n");
            }
        }
        item_data += prop_item[i].length;
    }

    HeapFree(GetProcessHeap(), 0, prop_item);

    GdipDisposeImage(image);
}

static void test_tiff_palette(void)
{
    GpStatus status;
    GpImage *image;
    PixelFormat format;
    INT size;
    struct
    {
        ColorPalette pal;
        ARGB entry[256];
    } palette;
    ARGB *entries = palette.pal.Entries;

    /* 1bpp TIFF without palette */
    image = load_image((const BYTE *)&TIFF_data, sizeof(TIFF_data), TRUE, FALSE);
    if (!image)
    {
        win_skip("Failed to load TIFF image data. Might not be supported. Skipping.\n");
        return;
    }

    status = GdipGetImagePixelFormat(image, &format);
    expect(Ok, status);
    ok(format == PixelFormat1bppIndexed, "expected PixelFormat1bppIndexed, got %#x\n", format);

    status = GdipGetImagePaletteSize(image, &size);
    ok(status == Ok || broken(status == GenericError), /* XP */
       "GdipGetImagePaletteSize error %d\n", status);
    if (status == GenericError)
    {
        GdipDisposeImage(image);
        return;
    }
    expect(sizeof(ColorPalette) + sizeof(ARGB), size);

    status = GdipGetImagePalette(image, &palette.pal, size);
    expect(Ok, status);
    expect(0, palette.pal.Flags);
    expect(2, palette.pal.Count);
    if (palette.pal.Count == 2)
    {
        ok(entries[0] == 0xff000000, "expected 0xff000000, got %#x\n", entries[0]);
        ok(entries[1] == 0xffffffff, "expected 0xffffffff, got %#x\n", entries[1]);
    }

    GdipDisposeImage(image);
}

static void test_bitmapbits(void)
{
    /* 8 x 2 bitmap */
    static const BYTE pixels_24[48] =
    {
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0
    };
    static const BYTE pixels_00[48] =
    {
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0
    };
    static const BYTE pixels_24_77[64] =
    {
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0xff,0xff,0xff, 0,0,0, 0xff,0xff,0xff, 0,0,0,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77
    };
    static const BYTE pixels_77[64] =
    {
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77
    };
    static const BYTE pixels_8[16] =
    {
        0x01,0,0x01,0,0x01,0,0x01,0,
        0x01,0,0x01,0,0x01,0,0x01,0
    };
    static const BYTE pixels_8_77[64] =
    {
        0x01,0,0x01,0,0x01,0,0x01,0,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x01,0,0x01,0,0x01,0,0x01,0,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77
    };
    static const BYTE pixels_1_77[64] =
    {
        0xaa,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0xaa,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
        0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77
    };
    static const BYTE pixels_1[8] = {0xaa,0,0,0,0xaa,0,0,0};
    static const struct test_data
    {
        PixelFormat format;
        UINT bpp;
        ImageLockMode mode;
        UINT stride, size;
        const BYTE *pixels;
        const BYTE *pixels_unlocked;
    } td[] =
    {
        /* 0 */
        { PixelFormat24bppRGB, 24, 0xfff0, 24, 48, pixels_24, pixels_00 },

        { PixelFormat24bppRGB, 24, 0, 24, 48, pixels_24, pixels_00 },
        { PixelFormat24bppRGB, 24, ImageLockModeRead, 24, 48, pixels_24, pixels_00 },
        { PixelFormat24bppRGB, 24, ImageLockModeWrite, 24, 48, pixels_24, pixels_00 },
        { PixelFormat24bppRGB, 24, ImageLockModeRead|ImageLockModeWrite, 24, 48, pixels_24, pixels_00 },
        { PixelFormat24bppRGB, 24, ImageLockModeRead|ImageLockModeUserInputBuf, 32, 64, pixels_24_77, pixels_24 },
        { PixelFormat24bppRGB, 24, ImageLockModeWrite|ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_00 },
        { PixelFormat24bppRGB, 24, ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_24 },
        /* 8 */
        { PixelFormat8bppIndexed, 8, 0, 8, 16, pixels_8, pixels_24 },
        { PixelFormat8bppIndexed, 8, ImageLockModeRead, 8, 16, pixels_8, pixels_24 },
        { PixelFormat8bppIndexed, 8, ImageLockModeWrite, 8, 16, pixels_8, pixels_00 },
        { PixelFormat8bppIndexed, 8, ImageLockModeRead|ImageLockModeWrite, 8, 16, pixels_8, pixels_00 },
        { PixelFormat8bppIndexed, 8, ImageLockModeRead|ImageLockModeUserInputBuf, 32, 64, pixels_8_77, pixels_24 },
        { PixelFormat8bppIndexed, 8, ImageLockModeWrite|ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_00 },
        { PixelFormat8bppIndexed, 8, ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_24 },
        /* 15 */
        { PixelFormat1bppIndexed, 1, 0, 4, 8, pixels_1, pixels_24 },
        { PixelFormat1bppIndexed, 1, ImageLockModeRead, 4, 8, pixels_1, pixels_24 },
        { PixelFormat1bppIndexed, 1, ImageLockModeWrite, 4, 8, pixels_1, pixels_00 },
        { PixelFormat1bppIndexed, 1, ImageLockModeRead|ImageLockModeWrite, 4, 8, pixels_1, pixels_00 },
        { PixelFormat1bppIndexed, 1, ImageLockModeRead|ImageLockModeUserInputBuf, 32, 64, pixels_1_77, pixels_24 },
        { PixelFormat1bppIndexed, 1, ImageLockModeWrite|ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_00 },
        { PixelFormat1bppIndexed, 1, ImageLockModeUserInputBuf, 32, 64, pixels_77, pixels_24 },
    };
    BYTE buf[64];
    GpStatus status;
    GpBitmap *bitmap;
    UINT i;
    BitmapData data;
    struct
    {
        ColorPalette pal;
        ARGB entries[1];
    } palette;
    ARGB *entries = palette.pal.Entries;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        BYTE pixels[sizeof(pixels_24)];
        memcpy(pixels, pixels_24, sizeof(pixels_24));
        status = GdipCreateBitmapFromScan0(8, 2, 24, PixelFormat24bppRGB, pixels, &bitmap);
        expect(Ok, status);

        /* associate known palette with pixel data */
        palette.pal.Flags = PaletteFlagsGrayScale;
        palette.pal.Count = 2;
        entries[0] = 0xff000000;
        entries[1] = 0xffffffff;
        status = GdipSetImagePalette((GpImage *)bitmap, &palette.pal);
        expect(Ok, status);

        memset(&data, 0xfe, sizeof(data));
        if (td[i].mode & ImageLockModeUserInputBuf)
        {
            memset(buf, 0x77, sizeof(buf));
            data.Scan0 = buf;
            data.Stride = 32;
        }
        status = GdipBitmapLockBits(bitmap, NULL, td[i].mode, td[i].format, &data);
        ok(status == Ok || broken(status == InvalidParameter) /* XP */, "%u: GdipBitmapLockBits error %d\n", i, status);
        if (status != Ok)
        {
            GdipDisposeImage((GpImage *)bitmap);
            continue;
        }
        ok(data.Width == 8, "%u: expected 8, got %d\n", i, data.Width);
        ok(data.Height == 2, "%u: expected 2, got %d\n", i, data.Height);
        ok(td[i].stride == data.Stride, "%u: expected %d, got %d\n", i, td[i].stride, data.Stride);
        ok(td[i].format == data.PixelFormat, "%u: expected %d, got %d\n", i, td[i].format, data.PixelFormat);
        ok(td[i].size == data.Height * data.Stride, "%u: expected %d, got %d\n", i, td[i].size, data.Height * data.Stride);
        if (td[i].mode & ImageLockModeUserInputBuf)
            ok(data.Scan0 == buf, "%u: got wrong buffer\n", i);
        if (td[i].size == data.Height * data.Stride)
        {
            UINT j, match, width_bytes = (data.Width * td[i].bpp) / 8;

            match = 1;
            for (j = 0; j < data.Height; j++)
            {
                if (memcmp((const BYTE *)data.Scan0 + j * data.Stride, td[i].pixels + j * data.Stride, width_bytes) != 0)
                {
                    match = 0;
                    break;
                }
            }
            if ((td[i].mode & (ImageLockModeRead|ImageLockModeUserInputBuf)) || td[i].format == PixelFormat24bppRGB)
            {
                ok(match,
                   "%u: data should match\n", i);
                if (!match)
                {
                    BYTE *bits = data.Scan0;
                    trace("%u: data mismatch for format %#x:", i, td[i].format);
                    for (j = 0; j < td[i].size; j++)
                        trace(" %02x", bits[j]);
                    trace("\n");
                }
            }
            else
                ok(!match, "%u: data shouldn't match\n", i);

            memset(data.Scan0, 0, td[i].size);
        }

        status = GdipBitmapUnlockBits(bitmap, &data);
        ok(status == Ok, "%u: GdipBitmapUnlockBits error %d\n", i, status);

        memset(&data, 0xfe, sizeof(data));
        status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat24bppRGB, &data);
        ok(status == Ok, "%u: GdipBitmapLockBits error %d\n", i, status);
        ok(data.Width == 8, "%u: expected 8, got %d\n", i, data.Width);
        ok(data.Height == 2, "%u: expected 2, got %d\n", i, data.Height);
        ok(data.Stride == 24, "%u: expected 24, got %d\n", i, data.Stride);
        ok(data.PixelFormat == PixelFormat24bppRGB, "%u: got wrong pixel format %d\n", i, data.PixelFormat);
        ok(data.Height * data.Stride == 48, "%u: expected 48, got %d\n", i, data.Height * data.Stride);
        if (data.Height * data.Stride == 48)
        {
            int match = memcmp(data.Scan0, td[i].pixels_unlocked, 48) == 0;
            ok(match, "%u: data should match\n", i);
            if (!match)
            {
                UINT j;
                BYTE *bits = data.Scan0;
                trace("%u: data mismatch for format %#x:", i, td[i].format);
                for (j = 0; j < 48; j++)
                    trace(" %02x", bits[j]);
                trace("\n");
            }
        }

        status = GdipBitmapUnlockBits(bitmap, &data);
        ok(status == Ok, "%u: GdipBitmapUnlockBits error %d\n", i, status);

        status = GdipDisposeImage((GpImage *)bitmap);
        expect(Ok, status);
    }
}

static void test_DrawImage(void)
{
    BYTE black_1x1[4] = { 0,0,0,0 };
    BYTE white_2x2[16] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                           0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
    BYTE black_2x2[16] = { 0,0,0,0,0,0,0xff,0xff,
                           0,0,0,0,0,0,0xff,0xff };
    GpStatus status;
    union
    {
        GpBitmap *bitmap;
        GpImage *image;
    } u1, u2;
    GpGraphics *graphics;
    int match;

    status = GdipCreateBitmapFromScan0(1, 1, 4, PixelFormat24bppRGB, black_1x1, &u1.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u1.bitmap, 100.0, 100.0);
    expect(Ok, status);

    status = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat24bppRGB, white_2x2, &u2.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u2.bitmap, 300.0, 300.0);
    expect(Ok, status);
    status = GdipGetImageGraphicsContext(u2.image, &graphics);
    expect(Ok, status);
    status = GdipSetInterpolationMode(graphics, InterpolationModeNearestNeighbor);
    expect(Ok, status);

    status = GdipDrawImageI(graphics, u1.image, 0, 0);
    expect(Ok, status);

    match = memcmp(white_2x2, black_2x2, sizeof(black_2x2)) == 0;
    ok(match, "data should match\n");
    if (!match)
    {
        UINT i, size = sizeof(white_2x2);
        BYTE *bits = white_2x2;
        for (i = 0; i < size; i++)
            trace(" %02x", bits[i]);
        trace("\n");
    }

    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    status = GdipDisposeImage(u1.image);
    expect(Ok, status);
    status = GdipDisposeImage(u2.image);
    expect(Ok, status);
}

static void test_DrawImage_SourceCopy(void)
{
    DWORD dst_pixels[4] = { 0xffffffff, 0xffffffff,
                            0xffffffff, 0xffffffff };
    DWORD src_pixels[4] = { 0, 0xffff0000,
                            0, 0xff00ff };

    GpStatus status;
    union
    {
        GpBitmap *bitmap;
        GpImage *image;
    } u1, u2;
    GpGraphics *graphics;

    status = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat32bppARGB, (BYTE*)dst_pixels, &u1.bitmap);
    expect(Ok, status);

    status = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat32bppARGB, (BYTE*)src_pixels, &u2.bitmap);
    expect(Ok, status);
    status = GdipGetImageGraphicsContext(u1.image, &graphics);
    expect(Ok, status);
    status = GdipSetInterpolationMode(graphics, InterpolationModeNearestNeighbor);
    expect(Ok, status);

    status = GdipSetCompositingMode(graphics, CompositingModeSourceCopy);
    expect(Ok, status);

    status = GdipDrawImageI(graphics, u2.image, 0, 0);
    expect(Ok, status);

    todo_wine expect(0, dst_pixels[0]);
    expect(0xffff0000, dst_pixels[1]);
    todo_wine expect(0, dst_pixels[2]);
    todo_wine expect(0, dst_pixels[3]);

    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    status = GdipDisposeImage(u1.image);
    expect(Ok, status);
    status = GdipDisposeImage(u2.image);
    expect(Ok, status);
}

static void test_GdipDrawImagePointRect(void)
{
    BYTE black_1x1[4] = { 0,0,0,0 };
    BYTE white_2x2[16] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                           0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
    BYTE black_2x2[16] = { 0,0,0,0,0,0,0xff,0xff,
                           0,0,0,0,0,0,0xff,0xff };
    GpStatus status;
    union
    {
        GpBitmap *bitmap;
        GpImage *image;
    } u1, u2;
    GpGraphics *graphics;
    int match;

    status = GdipCreateBitmapFromScan0(1, 1, 4, PixelFormat24bppRGB, black_1x1, &u1.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u1.bitmap, 100.0, 100.0);
    expect(Ok, status);

    status = GdipCreateBitmapFromScan0(2, 2, 8, PixelFormat24bppRGB, white_2x2, &u2.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u2.bitmap, 300.0, 300.0);
    expect(Ok, status);
    status = GdipGetImageGraphicsContext(u2.image, &graphics);
    expect(Ok, status);
    status = GdipSetInterpolationMode(graphics, InterpolationModeNearestNeighbor);
    expect(Ok, status);

    status = GdipDrawImagePointRectI(graphics, u1.image, 0, 0, 0, 0, 1, 1, UnitPixel);
    expect(Ok, status);

    match = memcmp(white_2x2, black_2x2, sizeof(black_2x2)) == 0;
    ok(match, "data should match\n");
    if (!match)
    {
        UINT i, size = sizeof(white_2x2);
        BYTE *bits = white_2x2;
        for (i = 0; i < size; i++)
            trace(" %02x", bits[i]);
        trace("\n");
    }

    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    status = GdipDisposeImage(u1.image);
    expect(Ok, status);
    status = GdipDisposeImage(u2.image);
    expect(Ok, status);
}

static void test_image_format(void)
{
    static const PixelFormat fmt[] =
    {
        PixelFormat1bppIndexed, PixelFormat4bppIndexed, PixelFormat8bppIndexed,
        PixelFormat16bppGrayScale, PixelFormat16bppRGB555, PixelFormat16bppRGB565,
        PixelFormat16bppARGB1555, PixelFormat24bppRGB, PixelFormat32bppRGB,
        PixelFormat32bppARGB, PixelFormat32bppPARGB, PixelFormat48bppRGB,
        PixelFormat64bppARGB, PixelFormat64bppPARGB, PixelFormat32bppCMYK
    };
    GpStatus status;
    GpBitmap *bitmap;
    GpImage *thumb;
    HBITMAP hbitmap;
    BITMAP bm;
    PixelFormat format;
    BitmapData data;
    UINT i, ret;

    for (i = 0; i < ARRAY_SIZE(fmt); i++)
    {
        status = GdipCreateBitmapFromScan0(1, 1, 0, fmt[i], NULL, &bitmap);
        ok(status == Ok || broken(status == InvalidParameter) /* before win7 */,
           "GdipCreateBitmapFromScan0 error %d\n", status);
        if (status != Ok) continue;

        status = GdipGetImagePixelFormat((GpImage *)bitmap, &format);
        expect(Ok, status);
        expect(fmt[i], format);

        status = GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0);
        if (fmt[i] == PixelFormat16bppGrayScale || fmt[i] == PixelFormat32bppCMYK)
            todo_wine expect(InvalidParameter, status);
        else
        {
            expect(Ok, status);
            ret = GetObjectW(hbitmap, sizeof(bm), &bm);
            expect(sizeof(bm), ret);
            expect(0, bm.bmType);
            expect(1, bm.bmWidth);
            expect(1, bm.bmHeight);
            expect(4, bm.bmWidthBytes);
            expect(1, bm.bmPlanes);
            expect(32, bm.bmBitsPixel);
            DeleteObject(hbitmap);
        }

        status = GdipGetImageThumbnail((GpImage *)bitmap, 0, 0, &thumb, NULL, NULL);
        if (fmt[i] == PixelFormat16bppGrayScale || fmt[i] == PixelFormat32bppCMYK)
            todo_wine
            ok(status == OutOfMemory || broken(status == InvalidParameter) /* before win7 */,
               "expected OutOfMemory, got %d\n", status);
        else
            expect(Ok, status);
        if (status == Ok)
        {
            status = GdipGetImagePixelFormat(thumb, &format);
            expect(Ok, status);
            ok(format == PixelFormat32bppPARGB || broken(format != PixelFormat32bppPARGB) /* before win7 */,
               "expected PixelFormat32bppPARGB, got %#x\n", format);
            status = GdipDisposeImage(thumb);
            expect(Ok, status);
        }

        status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat32bppPARGB, &data);
        if (fmt[i] == PixelFormat16bppGrayScale || fmt[i] == PixelFormat32bppCMYK)
            todo_wine expect(InvalidParameter, status);
        else
        {
            expect(Ok, status);
            status = GdipBitmapUnlockBits(bitmap, &data);
            expect(Ok, status);
        }

        status = GdipDisposeImage((GpImage *)bitmap);
        expect(Ok, status);
    }
}

static void test_DrawImage_scale(void)
{
    static const BYTE back_8x1[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
                                       0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_080[24] = { 0x40,0x40,0x40,0x80,0x80,0x80,0x40,0x40,0x40,0x40,0x40,0x40,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_100[24] = { 0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,0x40,0x40,0x40,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_120[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x40,0x40,0x40,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_150[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_180[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x80,0x80,0x80,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_200[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x80,0x80,0x80,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_250[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,
                                        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x40,0x40,0x40 };
    static const BYTE image_120_half[24] = { 0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_150_half[24] = { 0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_200_half[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x80,0x80,0x80,0x80,0x80,0x80,0x40,0x40,0x40,0x40,0x40,0x40 };
    static const BYTE image_250_half[24] = { 0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
                                        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x40,0x40,0x40 };
    static const struct test_data
    {
        REAL scale_x;
        PixelOffsetMode pixel_offset_mode;
        const BYTE *image;
        BOOL todo;
    } td[] =
    {
        { 0.8, PixelOffsetModeNone, image_080 }, /* 0 */
        { 1.0, PixelOffsetModeNone, image_100 },
        { 1.2, PixelOffsetModeNone, image_120 },
        { 1.5, PixelOffsetModeNone, image_150 },
        { 1.8, PixelOffsetModeNone, image_180 },
        { 2.0, PixelOffsetModeNone, image_200 },
        { 2.5, PixelOffsetModeNone, image_250 },

        { 0.8, PixelOffsetModeHighSpeed, image_080 }, /* 7 */
        { 1.0, PixelOffsetModeHighSpeed, image_100 },
        { 1.2, PixelOffsetModeHighSpeed, image_120 },
        { 1.5, PixelOffsetModeHighSpeed, image_150 },
        { 1.8, PixelOffsetModeHighSpeed, image_180 },
        { 2.0, PixelOffsetModeHighSpeed, image_200 },
        { 2.5, PixelOffsetModeHighSpeed, image_250 },

        { 0.8, PixelOffsetModeHalf, image_080 }, /* 14 */
        { 1.0, PixelOffsetModeHalf, image_100 },
        { 1.2, PixelOffsetModeHalf, image_120_half, TRUE },
        { 1.5, PixelOffsetModeHalf, image_150_half, TRUE },
        { 1.8, PixelOffsetModeHalf, image_180 },
        { 2.0, PixelOffsetModeHalf, image_200_half, TRUE },
        { 2.5, PixelOffsetModeHalf, image_250_half, TRUE },

        { 0.8, PixelOffsetModeHighQuality, image_080 }, /* 21 */
        { 1.0, PixelOffsetModeHighQuality, image_100 },
        { 1.2, PixelOffsetModeHighQuality, image_120_half, TRUE },
        { 1.5, PixelOffsetModeHighQuality, image_150_half, TRUE },
        { 1.8, PixelOffsetModeHighQuality, image_180 },
        { 2.0, PixelOffsetModeHighQuality, image_200_half, TRUE },
        { 2.5, PixelOffsetModeHighQuality, image_250_half, TRUE },
    };
    BYTE src_2x1[6] = { 0x80,0x80,0x80,0x80,0x80,0x80 };
    BYTE dst_8x1[24];
    GpStatus status;
    union
    {
        GpBitmap *bitmap;
        GpImage *image;
    } u1, u2;
    GpGraphics *graphics;
    GpMatrix *matrix;
    int i, match;

    status = GdipCreateBitmapFromScan0(2, 1, 4, PixelFormat24bppRGB, src_2x1, &u1.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u1.bitmap, 100.0, 100.0);
    expect(Ok, status);

    status = GdipCreateBitmapFromScan0(8, 1, 24, PixelFormat24bppRGB, dst_8x1, &u2.bitmap);
    expect(Ok, status);
    status = GdipBitmapSetResolution(u2.bitmap, 100.0, 100.0);
    expect(Ok, status);
    status = GdipGetImageGraphicsContext(u2.image, &graphics);
    expect(Ok, status);
    status = GdipSetInterpolationMode(graphics, InterpolationModeNearestNeighbor);
    expect(Ok, status);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        status = GdipSetPixelOffsetMode(graphics, td[i].pixel_offset_mode);
        expect(Ok, status);

        status = GdipCreateMatrix2(td[i].scale_x, 0.0, 0.0, 1.0, 0.0, 0.0, &matrix);
        expect(Ok, status);
        status = GdipSetWorldTransform(graphics, matrix);
        expect(Ok, status);
        GdipDeleteMatrix(matrix);

        memcpy(dst_8x1, back_8x1, sizeof(dst_8x1));
        status = GdipDrawImageI(graphics, u1.image, 1, 0);
        expect(Ok, status);

        match = memcmp(dst_8x1, td[i].image, sizeof(dst_8x1)) == 0;
        todo_wine_if (!match && td[i].todo)
            ok(match, "%d: data should match\n", i);
        if (!match)
        {
            UINT i, size = sizeof(dst_8x1);
            const BYTE *bits = dst_8x1;
            for (i = 0; i < size; i++)
                trace(" %02x", bits[i]);
            trace("\n");
        }
    }

    status = GdipDeleteGraphics(graphics);
    expect(Ok, status);
    status = GdipDisposeImage(u1.image);
    expect(Ok, status);
    status = GdipDisposeImage(u2.image);
    expect(Ok, status);
}

static const BYTE animatedgif[] = {
'G','I','F','8','9','a',0x01,0x00,0x01,0x00,0xA1,0x02,0x00,
0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
/*0x21,0xFF,0x0B,'A','N','I','M','E','X','T','S','1','.','0',*/
0x21,0xFF,0x0B,'N','E','T','S','C','A','P','E','2','.','0',
0x03,0x01,0x05,0x00,0x00,
0x21,0xFE,0x0C,'H','e','l','l','o',' ','W','o','r','l','d','!',0x00,
0x21,0x01,0x0D,'a','n','i','m','a','t','i','o','n','.','g','i','f',0x00,
0x21,0xF9,0x04,0xff,0x0A,0x00,0x08,0x00,
0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,
0x02,0x02,0x4C,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','1',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','1',0x00,
0x21,0xF9,0x04,0x00,0x14,0x00,0x01,0x00,
0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,
0x02,0x02,0x44,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','2',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','2',0x00,0x3B
};

static void test_gif_properties(void)
{
    static const struct test_data
    {
        ULONG type, id, length;
        const BYTE value[13];
    } td[] =
    {
        { PropertyTagTypeLong, PropertyTagFrameDelay, 8, { 10,0,0,0,20,0,0,0 } },
        { PropertyTagTypeASCII, PropertyTagExifUserComment, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { PropertyTagTypeShort, PropertyTagLoopCount, 2, { 5,0 } },
        { PropertyTagTypeByte, PropertyTagGlobalPalette, 12, { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c } },
        { PropertyTagTypeByte, PropertyTagIndexBackground, 1, { 2 } },
        { PropertyTagTypeByte, PropertyTagIndexTransparent, 1, { 8 } }
    };
    GpStatus status;
    GpImage *image;
    GUID guid;
    UINT dim_count, frame_count, prop_count, prop_size, i;
    UINT total_size, total_count;
    PROPID *prop_id;
    PropertyItem *prop_item;
    const char *item_data;

    image = load_image(animatedgif, sizeof(animatedgif), TRUE, FALSE);
    if (!image) /* XP fails to load this GIF image */
    {
        trace("Failed to load GIF image data\n");
        return;
    }

    status = GdipImageGetFrameDimensionsCount(image, &dim_count);
    expect(Ok, status);
    expect(1, dim_count);

    status = GdipImageGetFrameDimensionsList(image, &guid, 1);
    expect(Ok, status);
    expect_guid(&FrameDimensionTime, &guid, __LINE__, FALSE);

    status = GdipImageGetFrameCount(image, &guid, &frame_count);
    expect(Ok, status);
    expect(2, frame_count);

    status = GdipImageSelectActiveFrame(image, &guid, 1);
    expect(Ok, status);

    status = GdipGetPropertyCount(image, &prop_count);
    expect(Ok, status);
    ok(prop_count == ARRAY_SIZE(td) || broken(prop_count == 1) /* before win7 */,
       "expected property count %u, got %u\n", (UINT) ARRAY_SIZE(td), prop_count);

    if (prop_count != ARRAY_SIZE(td))
    {
        GdipDisposeImage(image);
        return;
    }

    prop_id = HeapAlloc(GetProcessHeap(), 0, prop_count * sizeof(*prop_id));

    status = GdipGetPropertyIdList(image, prop_count, prop_id);
    expect(Ok, status);

    prop_size = 0;
    for (i = 0; i < prop_count; i++)
    {
        UINT size;
        status = GdipGetPropertyItemSize(image, prop_id[i], &size);
        expect(Ok, status);
        if (status != Ok) break;
        ok(size > sizeof(*prop_item), "%u: too small item length %u\n", i, size);

        prop_size += size;

        prop_item = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        status = GdipGetPropertyItem(image, prop_id[i], size, prop_item);
        expect(Ok, status);
        ok(prop_item->value == prop_item + 1, "expected item->value %p, got %p\n", prop_item + 1, prop_item->value);
        ok(td[i].type == prop_item->type,
            "%u: expected type %u, got %u\n", i, td[i].type, prop_item->type);
        ok(td[i].id == prop_item->id, "%u: expected id %#x, got %#x\n", i, td[i].id, prop_item->id);
        size -= sizeof(*prop_item);
        ok(prop_item->length == size, "%u: expected length %u, got %u\n", i, size, prop_item->length);
        ok(td[i].length == prop_item->length, "%u: expected length %u, got %u\n", i, td[i].length, prop_item->length);
        if (td[i].length == prop_item->length)
        {
            int match = memcmp(td[i].value, prop_item->value, td[i].length) == 0;
            ok(match, "%u: data mismatch\n", i);
            if (!match)
            {
                UINT j;
                BYTE *data = prop_item->value;
                trace("id %#x:", prop_item->id);
                for (j = 0; j < prop_item->length; j++)
                    trace(" %02x", data[j]);
                trace("\n");
            }
        }
        HeapFree(GetProcessHeap(), 0, prop_item);
    }

    HeapFree(GetProcessHeap(), 0, prop_id);

    status = GdipGetPropertySize(NULL, &total_size, &total_count);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, &total_size, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, NULL, &total_count);
    expect(InvalidParameter, status);
    status = GdipGetPropertySize(image, NULL, NULL);
    expect(InvalidParameter, status);
    total_size = 0xdeadbeef;
    total_count = 0xdeadbeef;
    status = GdipGetPropertySize(image, &total_size, &total_count);
    expect(Ok, status);
    ok(prop_count == total_count,
       "expected total property count %u, got %u\n", prop_count, total_count);
    ok(prop_size == total_size,
       "expected total property size %u, got %u\n", prop_size, total_size);

    prop_item = HeapAlloc(GetProcessHeap(), 0, prop_size);

    status = GdipGetAllPropertyItems(image, 0, prop_count, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, 1, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, 0, 0, NULL);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size + 1, prop_count, prop_item);
    expect(InvalidParameter, status);
    status = GdipGetAllPropertyItems(image, prop_size, prop_count, prop_item);
    expect(Ok, status);

    item_data = (const char *)(prop_item + prop_count);
    for (i = 0; i < prop_count; i++)
    {
        ok(prop_item[i].value == item_data, "%u: expected value %p, got %p\n",
           i, item_data, prop_item[i].value);
        ok(td[i].type == prop_item[i].type,
            "%u: expected type %u, got %u\n", i, td[i].type, prop_item[i].type);
        ok(td[i].id == prop_item[i].id, "%u: expected id %#x, got %#x\n", i, td[i].id, prop_item[i].id);
        ok(td[i].length == prop_item[i].length, "%u: expected length %u, got %u\n", i, td[i].length, prop_item[i].length);
        if (td[i].length == prop_item[i].length)
        {
            int match = memcmp(td[i].value, prop_item[i].value, td[i].length) == 0;
            ok(match, "%u: data mismatch\n", i);
            if (!match)
            {
                UINT j;
                BYTE *data = prop_item[i].value;
                trace("id %#x:", prop_item[i].id);
                for (j = 0; j < prop_item[i].length; j++)
                    trace(" %02x", data[j]);
                trace("\n");
            }
        }
        item_data += prop_item[i].length;
    }

    HeapFree(GetProcessHeap(), 0, prop_item);

    GdipDisposeImage(image);
}

static void test_ARGB_conversion(void)
{
    BYTE argb[8] = { 0x11,0x22,0x33,0x80, 0xff,0xff,0xff,0 };
    BYTE pargb[8] = { 0x09,0x11,0x1a,0x80, 0,0,0,0 };
    BYTE rgb32_xp[8] = { 0x11,0x22,0x33,0xff, 0xff,0xff,0xff,0xff };
    BYTE rgb24[6] = { 0x11,0x22,0x33, 0xff,0xff,0xff };
    BYTE *bits;
    GpBitmap *bitmap;
    BitmapData data;
    GpStatus status;
    int match;

    status = GdipCreateBitmapFromScan0(2, 1, 8, PixelFormat32bppARGB, argb, &bitmap);
    expect(Ok, status);

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat32bppPARGB, &data);
    expect(Ok, status);
    ok(data.Width == 2, "expected 2, got %d\n", data.Width);
    ok(data.Height == 1, "expected 1, got %d\n", data.Height);
    ok(data.Stride == 8, "expected 8, got %d\n", data.Stride);
    ok(data.PixelFormat == PixelFormat32bppPARGB, "expected PixelFormat32bppPARGB, got %d\n", data.PixelFormat);
    match = !memcmp(data.Scan0, pargb, sizeof(pargb));
    ok(match, "bits don't match\n");
    if (!match)
    {
        bits = data.Scan0;
        trace("format %#x, bits %02x,%02x,%02x,%02x %02x,%02x,%02x,%02x\n", PixelFormat32bppPARGB,
               bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7]);
    }
    status = GdipBitmapUnlockBits(bitmap, &data);
    expect(Ok, status);

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat32bppRGB, &data);
    expect(Ok, status);
    ok(data.Width == 2, "expected 2, got %d\n", data.Width);
    ok(data.Height == 1, "expected 1, got %d\n", data.Height);
    ok(data.Stride == 8, "expected 8, got %d\n", data.Stride);
    ok(data.PixelFormat == PixelFormat32bppRGB, "expected PixelFormat32bppRGB, got %d\n", data.PixelFormat);
    match = !memcmp(data.Scan0, argb, sizeof(argb)) ||
            !memcmp(data.Scan0, rgb32_xp, sizeof(rgb32_xp));
    ok(match, "bits don't match\n");
    if (!match)
    {
        bits = data.Scan0;
        trace("format %#x, bits %02x,%02x,%02x,%02x %02x,%02x,%02x,%02x\n", PixelFormat32bppRGB,
               bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7]);
    }
    status = GdipBitmapUnlockBits(bitmap, &data);
    expect(Ok, status);

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat24bppRGB, &data);
    expect(Ok, status);
    ok(data.Width == 2, "expected 2, got %d\n", data.Width);
    ok(data.Height == 1, "expected 1, got %d\n", data.Height);
    ok(data.Stride == 8, "expected 8, got %d\n", data.Stride);
    ok(data.PixelFormat == PixelFormat24bppRGB, "expected PixelFormat24bppRGB, got %d\n", data.PixelFormat);
    match = !memcmp(data.Scan0, rgb24, sizeof(rgb24));
    ok(match, "bits don't match\n");
    if (!match)
    {
        bits = data.Scan0;
        trace("format %#x, bits %02x,%02x,%02x,%02x %02x,%02x,%02x,%02x\n", PixelFormat24bppRGB,
               bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7]);
    }
    status = GdipBitmapUnlockBits(bitmap, &data);
    expect(Ok, status);

    GdipDisposeImage((GpImage *)bitmap);
}


static void test_CloneBitmapArea(void)
{
    GpStatus status;
    GpBitmap *bitmap, *copy;
    BitmapData data, data2;

    status = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat24bppRGB, NULL, &bitmap);
    expect(Ok, status);

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &data);
    expect(Ok, status);

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat24bppRGB, &data2);
    expect(WrongState, status);

    status = GdipCloneBitmapAreaI(0, 0, 1, 1, PixelFormat24bppRGB, bitmap, &copy);
    expect(Ok, status);

    status = GdipBitmapUnlockBits(bitmap, &data);
    expect(Ok, status);

    GdipDisposeImage((GpImage *)copy);
    GdipDisposeImage((GpImage *)bitmap);
}

static BOOL get_encoder_clsid(LPCWSTR mime, GUID *format, CLSID *clsid)
{
    GpStatus status;
    UINT n_codecs, info_size, i;
    ImageCodecInfo *info;
    BOOL ret = FALSE;

    status = GdipGetImageEncodersSize(&n_codecs, &info_size);
    expect(Ok, status);

    info = GdipAlloc(info_size);

    status = GdipGetImageEncoders(n_codecs, info_size, info);
    expect(Ok, status);

    for (i = 0; i < n_codecs; i++)
    {
        if (!lstrcmpW(info[i].MimeType, mime))
        {
            *format = info[i].FormatID;
            *clsid = info[i].Clsid;
            ret = TRUE;
            break;
        }
    }

    GdipFree(info);
    return ret;
}

static void test_supported_encoders(void)
{
    static const WCHAR bmp_mimetype[] = { 'i', 'm', 'a','g', 'e', '/', 'b', 'm', 'p',0 };
    static const WCHAR jpeg_mimetype[] = { 'i','m','a','g','e','/','j','p','e','g',0 };
    static const WCHAR gif_mimetype[] = { 'i','m','a','g','e','/','g','i','f',0 };
    static const WCHAR tiff_mimetype[] = { 'i','m','a','g','e','/','t','i','f','f',0 };
    static const WCHAR png_mimetype[] = { 'i','m','a','g','e','/','p','n','g',0 };
    static const struct test_data
    {
        LPCWSTR mime;
        const GUID *format;
    } td[] =
    {
        { bmp_mimetype, &ImageFormatBMP },
        { jpeg_mimetype, &ImageFormatJPEG },
        { gif_mimetype, &ImageFormatGIF },
        { tiff_mimetype, &ImageFormatTIFF },
        { png_mimetype, &ImageFormatPNG }
    };
    GUID format, clsid;
    BOOL ret;
    HRESULT hr;
    GpStatus status;
    GpBitmap *bm;
    IStream *stream;
    HGLOBAL hmem;
    int i;

    status = GdipCreateBitmapFromScan0(1, 1, 0, PixelFormat24bppRGB, NULL, &bm);
    ok(status == Ok, "GdipCreateBitmapFromScan0 error %d\n", status);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        ret = get_encoder_clsid(td[i].mime, &format, &clsid);
        ok(ret, "%s encoder is not in the list\n", wine_dbgstr_w(td[i].mime));
        expect_guid(td[i].format, &format, __LINE__, FALSE);

        hmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, 16);

        hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
        ok(hr == S_OK, "CreateStreamOnHGlobal error %#x\n", hr);

        status = GdipSaveImageToStream((GpImage *)bm, stream, &clsid, NULL);
        ok(status == Ok, "GdipSaveImageToStream error %d\n", status);

        IStream_Release(stream);
    }

    GdipDisposeImage((GpImage *)bm);
}

static void test_createeffect(void)
{
    static const GUID noneffect = { 0xcd0c3d4b, 0xe15e, 0x4cf2, { 0x9e, 0xa8, 0x6e, 0x1d, 0x65, 0x48, 0xc5, 0xa5 } };
    GpStatus (WINAPI *pGdipCreateEffect)( const GUID guid, CGpEffect **effect);
    GpStatus (WINAPI *pGdipDeleteEffect)( CGpEffect *effect);
    GpStatus stat;
    CGpEffect *effect;
    HMODULE mod = GetModuleHandleA("gdiplus.dll");
    int i;
    const GUID * const effectlist[] =
               {&BlurEffectGuid, &SharpenEffectGuid, &ColorMatrixEffectGuid, &ColorLUTEffectGuid,
                &BrightnessContrastEffectGuid, &HueSaturationLightnessEffectGuid, &LevelsEffectGuid,
                &TintEffectGuid, &ColorBalanceEffectGuid, &RedEyeCorrectionEffectGuid, &ColorCurveEffectGuid};

    pGdipCreateEffect = (void*)GetProcAddress( mod, "GdipCreateEffect");
    pGdipDeleteEffect = (void*)GetProcAddress( mod, "GdipDeleteEffect");
    if(!pGdipCreateEffect || !pGdipDeleteEffect)
    {
        /* GdipCreateEffect/GdipDeleteEffect was introduced in Windows Vista. */
        win_skip("GDIPlus version 1.1 not available\n");
        return;
    }

    stat = pGdipCreateEffect(BlurEffectGuid, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipCreateEffect(noneffect, &effect);
    todo_wine expect(Win32Error, stat);

    for(i=0; i < ARRAY_SIZE(effectlist); i++)
    {
        stat = pGdipCreateEffect(*effectlist[i], &effect);
        todo_wine expect(Ok, stat);
        if(stat == Ok)
        {
            stat = pGdipDeleteEffect(effect);
            expect(Ok, stat);
        }
    }
}

static void test_getadjustedpalette(void)
{
    ColorMap colormap;
    GpImageAttributes *imageattributes;
    ColorPalette *palette;
    GpStatus stat;

    stat = GdipCreateImageAttributes(&imageattributes);
    expect(Ok, stat);

    colormap.oldColor.Argb = 0xffffff00;
    colormap.newColor.Argb = 0xffff00ff;
    stat = GdipSetImageAttributesRemapTable(imageattributes, ColorAdjustTypeBitmap,
        TRUE, 1, &colormap);
    expect(Ok, stat);

    colormap.oldColor.Argb = 0xffffff80;
    colormap.newColor.Argb = 0xffff80ff;
    stat = GdipSetImageAttributesRemapTable(imageattributes, ColorAdjustTypeDefault,
        TRUE, 1, &colormap);
    expect(Ok, stat);

    palette = GdipAlloc(sizeof(*palette) + sizeof(ARGB) * 2);
    palette->Count = 0;

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, palette, ColorAdjustTypeBitmap);
    expect(InvalidParameter, stat);

    palette->Count = 3;
    palette->Entries[0] = 0xffffff00;
    palette->Entries[1] = 0xffffff80;
    palette->Entries[2] = 0xffffffff;

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, palette, ColorAdjustTypeBitmap);
    expect(Ok, stat);
    expect(0xffff00ff, palette->Entries[0]);
    expect(0xffffff80, palette->Entries[1]);
    expect(0xffffffff, palette->Entries[2]);

    palette->Entries[0] = 0xffffff00;
    palette->Entries[1] = 0xffffff80;
    palette->Entries[2] = 0xffffffff;

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, palette, ColorAdjustTypeBrush);
    expect(Ok, stat);
    expect(0xffffff00, palette->Entries[0]);
    expect(0xffff80ff, palette->Entries[1]);
    expect(0xffffffff, palette->Entries[2]);

    stat = GdipGetImageAttributesAdjustedPalette(NULL, palette, ColorAdjustTypeBitmap);
    expect(InvalidParameter, stat);

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, NULL, ColorAdjustTypeBitmap);
    expect(InvalidParameter, stat);

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, palette, -1);
    expect(InvalidParameter, stat);

    stat = GdipGetImageAttributesAdjustedPalette(imageattributes, palette, ColorAdjustTypeDefault);
    expect(InvalidParameter, stat);

    GdipFree(palette);
    GdipDisposeImageAttributes(imageattributes);
}

static void test_histogram(void)
{
    UINT ch0[256], ch1[256], ch2[256], ch3[256];
    HistogramFormat test_formats[] =
    {
        HistogramFormatARGB,
        HistogramFormatPARGB,
        HistogramFormatRGB,
        HistogramFormatGray,
        HistogramFormatB,
        HistogramFormatG,
        HistogramFormatR,
        HistogramFormatA,
    };
    const UINT WIDTH = 8, HEIGHT = 16;
    UINT num, i, x;
    GpStatus stat;
    GpBitmap *bm;

    if (!pGdipBitmapGetHistogramSize)
    {
        win_skip("GdipBitmapGetHistogramSize is not supported\n");
        return;
    }

    stat = pGdipBitmapGetHistogramSize(HistogramFormatARGB, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogramSize(0xff, NULL);
    expect(InvalidParameter, stat);

    num = 123;
    stat = pGdipBitmapGetHistogramSize(10, &num);
    expect(Ok, stat);
    expect(256, num);

    for (i = 0; i < ARRAY_SIZE(test_formats); i++)
    {
        num = 0;
        stat = pGdipBitmapGetHistogramSize(test_formats[i], &num);
        expect(Ok, stat);
        expect(256, num);
    }

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    /* Three solid rgb rows, next three rows are rgb shades. */
    for (x = 0; x < WIDTH; x++)
    {
        GdipBitmapSetPixel(bm, x, 0, 0xffff0000);
        GdipBitmapSetPixel(bm, x, 1, 0xff00ff00);
        GdipBitmapSetPixel(bm, x, 2, 0xff0000ff);

        GdipBitmapSetPixel(bm, x, 3, 0xff010000);
        GdipBitmapSetPixel(bm, x, 4, 0xff003f00);
        GdipBitmapSetPixel(bm, x, 5, 0xff000020);
    }

    stat = pGdipBitmapGetHistogram(NULL, HistogramFormatRGB, 256, ch0, ch1, ch2, ch3);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, 123, 256, ch0, ch1, ch2, ch3);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, 123, 256, ch0, ch1, ch2, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, 123, 256, ch0, ch1, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, 123, 256, ch0, NULL, NULL, NULL);
    expect(InvalidParameter, stat);

    /* Requested format matches bitmap format */
    stat = pGdipBitmapGetHistogram(bm, HistogramFormatRGB, 256, ch0, ch1, ch2, ch3);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatRGB, 100, ch0, ch1, ch2, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatRGB, 257, ch0, ch1, ch2, NULL);
    expect(InvalidParameter, stat);

    /* Channel 3 is not used, must be NULL */
    stat = pGdipBitmapGetHistogram(bm, HistogramFormatRGB, 256, ch0, ch1, ch2, NULL);
    expect(Ok, stat);

    ok(ch0[0xff] == WIDTH, "Got red (0xff) %u\n", ch0[0xff]);
    ok(ch1[0xff] == WIDTH, "Got green (0xff) %u\n", ch1[0xff]);
    ok(ch2[0xff] == WIDTH, "Got blue (0xff) %u\n", ch1[0xff]);
    ok(ch0[0x01] == WIDTH, "Got red (0x01) %u\n", ch0[0x01]);
    ok(ch1[0x3f] == WIDTH, "Got green (0x3f) %u\n", ch1[0x3f]);
    ok(ch2[0x20] == WIDTH, "Got blue (0x20) %u\n", ch1[0x20]);

    /* ARGB histogram from RGB data. */
    stat = pGdipBitmapGetHistogram(bm, HistogramFormatARGB, 256, ch0, ch1, ch2, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatARGB, 256, ch0, ch1, ch2, ch3);
    expect(Ok, stat);

    ok(ch1[0xff] == WIDTH, "Got red (0xff) %u\n", ch1[0xff]);
    ok(ch2[0xff] == WIDTH, "Got green (0xff) %u\n", ch2[0xff]);
    ok(ch3[0xff] == WIDTH, "Got blue (0xff) %u\n", ch3[0xff]);
    ok(ch1[0x01] == WIDTH, "Got red (0x01) %u\n", ch1[0x01]);
    ok(ch2[0x3f] == WIDTH, "Got green (0x3f) %u\n", ch2[0x3f]);
    ok(ch3[0x20] == WIDTH, "Got blue (0x20) %u\n", ch3[0x20]);

    ok(ch0[0xff] == WIDTH * HEIGHT, "Got alpha (0xff) %u\n", ch0[0xff]);

    /* Request grayscale histogram from RGB bitmap. */
    stat = pGdipBitmapGetHistogram(bm, HistogramFormatGray, 256, ch0, ch1, ch2, ch3);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatGray, 256, ch0, ch1, ch2, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatGray, 256, ch0, ch1, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipBitmapGetHistogram(bm, HistogramFormatGray, 256, ch0, NULL, NULL, NULL);
    expect(Ok, stat);

    GdipDisposeImage((GpImage*)bm);
}

static void test_imageabort(void)
{
    GpStatus stat;
    GpBitmap *bm;

    if (!pGdipImageSetAbort)
    {
        win_skip("GdipImageSetAbort() is not supported.\n");
        return;
    }

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(8, 8, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    stat = pGdipImageSetAbort(NULL, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipImageSetAbort((GpImage*)bm, NULL);
    expect(Ok, stat);

    GdipDisposeImage((GpImage*)bm);
}

/* RGB 24 bpp 1x1 pixel PNG image */
static const char png_1x1_data[] = {
  0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a,
  0x00,0x00,0x00,0x0d,'I','H','D','R',0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,0xde,
  0x00,0x00,0x03,0x00,'P','L','T','E',
  0x01,0x01,0x01,0x02,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x07,0x08,0x08,0x08,
  0x09,0x09,0x09,0x0a,0x0a,0x0a,0x0b,0x0b,0x0b,0x0c,0x0c,0x0c,0x0d,0x0d,0x0d,0x0e,0x0e,0x0e,0x0f,0x0f,0x0f,0x10,0x10,0x10,
  0x11,0x11,0x11,0x12,0x12,0x12,0x13,0x13,0x13,0x14,0x14,0x14,0x15,0x15,0x15,0x16,0x16,0x16,0x17,0x17,0x17,0x18,0x18,0x18,
  0x19,0x19,0x19,0x1a,0x1a,0x1a,0x1b,0x1b,0x1b,0x1c,0x1c,0x1c,0x1d,0x1d,0x1d,0x1e,0x1e,0x1e,0x1f,0x1f,0x1f,0x20,0x20,0x20,
  0x21,0x21,0x21,0x22,0x22,0x22,0x23,0x23,0x23,0x24,0x24,0x24,0x25,0x25,0x25,0x26,0x26,0x26,0x27,0x27,0x27,0x28,0x28,0x28,
  0x29,0x29,0x29,0x2a,0x2a,0x2a,0x2b,0x2b,0x2b,0x2c,0x2c,0x2c,0x2d,0x2d,0x2d,0x2e,0x2e,0x2e,0x2f,0x2f,0x2f,0x30,0x30,0x30,
  0x31,0x31,0x31,0x32,0x32,0x32,0x33,0x33,0x33,0x34,0x34,0x34,0x35,0x35,0x35,0x36,0x36,0x36,0x37,0x37,0x37,0x38,0x38,0x38,
  0x39,0x39,0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,0x3f,0x3f,0x3f,0x40,0x40,0x40,
  0x41,0x41,0x41,0x42,0x42,0x42,0x43,0x43,0x43,0x44,0x44,0x44,0x45,0x45,0x45,0x46,0x46,0x46,0x47,0x47,0x47,0x48,0x48,0x48,
  0x49,0x49,0x49,0x4a,0x4a,0x4a,0x4b,0x4b,0x4b,0x4c,0x4c,0x4c,0x4d,0x4d,0x4d,0x4e,0x4e,0x4e,0x4f,0x4f,0x4f,0x50,0x50,0x50,
  0x51,0x51,0x51,0x52,0x52,0x52,0x53,0x53,0x53,0x54,0x54,0x54,0x55,0x55,0x55,0x56,0x56,0x56,0x57,0x57,0x57,0x58,0x58,0x58,
  0x59,0x59,0x59,0x5a,0x5a,0x5a,0x5b,0x5b,0x5b,0x5c,0x5c,0x5c,0x5d,0x5d,0x5d,0x5e,0x5e,0x5e,0x5f,0x5f,0x5f,0x60,0x60,0x60,
  0x61,0x61,0x61,0x62,0x62,0x62,0x63,0x63,0x63,0x64,0x64,0x64,0x65,0x65,0x65,0x66,0x66,0x66,0x67,0x67,0x67,0x68,0x68,0x68,
  0x69,0x69,0x69,0x6a,0x6a,0x6a,0x6b,0x6b,0x6b,0x6c,0x6c,0x6c,0x6d,0x6d,0x6d,0x6e,0x6e,0x6e,0x6f,0x6f,0x6f,0x70,0x70,0x70,
  0x71,0x71,0x71,0x72,0x72,0x72,0x73,0x73,0x73,0x74,0x74,0x74,0x75,0x75,0x75,0x76,0x76,0x76,0x77,0x77,0x77,0x78,0x78,0x78,
  0x79,0x79,0x79,0x7a,0x7a,0x7a,0x7b,0x7b,0x7b,0x7c,0x7c,0x7c,0x7d,0x7d,0x7d,0x7e,0x7e,0x7e,0x7f,0x7f,0x7f,0x80,0x80,0x80,
  0x01,0x01,0x01,0x02,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x07,0x08,0x08,0x08,
  0x09,0x09,0x09,0x0a,0x0a,0x0a,0x0b,0x0b,0x0b,0x0c,0x0c,0x0c,0x0d,0x0d,0x0d,0x0e,0x0e,0x0e,0x0f,0x0f,0x0f,0x10,0x10,0x10,
  0x11,0x11,0x11,0x12,0x12,0x12,0x13,0x13,0x13,0x14,0x14,0x14,0x15,0x15,0x15,0x16,0x16,0x16,0x17,0x17,0x17,0x18,0x18,0x18,
  0x19,0x19,0x19,0x1a,0x1a,0x1a,0x1b,0x1b,0x1b,0x1c,0x1c,0x1c,0x1d,0x1d,0x1d,0x1e,0x1e,0x1e,0x1f,0x1f,0x1f,0x20,0x20,0x20,
  0x21,0x21,0x21,0x22,0x22,0x22,0x23,0x23,0x23,0x24,0x24,0x24,0x25,0x25,0x25,0x26,0x26,0x26,0x27,0x27,0x27,0x28,0x28,0x28,
  0x29,0x29,0x29,0x2a,0x2a,0x2a,0x2b,0x2b,0x2b,0x2c,0x2c,0x2c,0x2d,0x2d,0x2d,0x2e,0x2e,0x2e,0x2f,0x2f,0x2f,0x30,0x30,0x30,
  0x31,0x31,0x31,0x32,0x32,0x32,0x33,0x33,0x33,0x34,0x34,0x34,0x35,0x35,0x35,0x36,0x36,0x36,0x37,0x37,0x37,0x38,0x38,0x38,
  0x39,0x39,0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,0x3f,0x3f,0x3f,0x40,0x40,0x40,
  0x41,0x41,0x41,0x42,0x42,0x42,0x43,0x43,0x43,0x44,0x44,0x44,0x45,0x45,0x45,0x46,0x46,0x46,0x47,0x47,0x47,0x48,0x48,0x48,
  0x49,0x49,0x49,0x4a,0x4a,0x4a,0x4b,0x4b,0x4b,0x4c,0x4c,0x4c,0x4d,0x4d,0x4d,0x4e,0x4e,0x4e,0x4f,0x4f,0x4f,0x50,0x50,0x50,
  0x51,0x51,0x51,0x52,0x52,0x52,0x53,0x53,0x53,0x54,0x54,0x54,0x55,0x55,0x55,0x56,0x56,0x56,0x57,0x57,0x57,0x58,0x58,0x58,
  0x59,0x59,0x59,0x5a,0x5a,0x5a,0x5b,0x5b,0x5b,0x5c,0x5c,0x5c,0x5d,0x5d,0x5d,0x5e,0x5e,0x5e,0x5f,0x5f,0x5f,0x60,0x60,0x60,
  0x61,0x61,0x61,0x62,0x62,0x62,0x63,0x63,0x63,0x64,0x64,0x64,0x65,0x65,0x65,0x66,0x66,0x66,0x67,0x67,0x67,0x68,0x68,0x68,
  0x69,0x69,0x69,0x6a,0x6a,0x6a,0x6b,0x6b,0x6b,0x6c,0x6c,0x6c,0x6d,0x6d,0x6d,0x6e,0x6e,0x6e,0x6f,0x6f,0x6f,0x70,0x70,0x70,
  0x71,0x71,0x71,0x72,0x72,0x72,0x73,0x73,0x73,0x74,0x74,0x74,0x75,0x75,0x75,0x76,0x76,0x76,0x77,0x77,0x77,0x78,0x78,0x78,
  0x79,0x79,0x79,0x7a,0x7a,0x7a,0x7b,0x7b,0x7b,0x7c,0x7c,0x7c,0x7d,0x7d,0x7d,0x7e,0x7e,0x7e,0x7f,0x7f,0x7f,0x80,0x80,0x80,
  0x76,0xb6,0x24,0x31,
  0x00,0x00,0x00,0x02,'t','R','N','S',0xff,0x00,0xe5,0xb7,0x30,0x4a,
  0x00,0x00,0x00,0x0c,'I','D','A','T',0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,0xe7,
  0x00,0x00,0x00,0x00,'I','E','N','D',0xae,0x42,0x60,0x82
};

#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_RGB 2
#define PNG_COLOR_TYPE_PALETTE 3
#define PNG_COLOR_TYPE_GRAY_ALPHA 4
#define PNG_COLOR_TYPE_RGB_ALPHA 6

static void test_png_color_formats(void)
{
    static const struct
    {
        char bit_depth, color_type;
        struct {
            PixelFormat format;
            ImageFlags flags;
            BOOL todo;
            BOOL todo_load;
        } t[3];
    } td[] =
    {
        /*  0 */
        { 1, PNG_COLOR_TYPE_RGB },
        { 2, PNG_COLOR_TYPE_RGB },
        { 4, PNG_COLOR_TYPE_RGB },
        { 8, PNG_COLOR_TYPE_RGB,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat24bppRGB, ImageFlagsColorSpaceRGB },
           { PixelFormat24bppRGB, ImageFlagsColorSpaceRGB }}},
        /* libpng refuses to load our test image complaining about extra compressed data,
         * but libpng is still able to load the image with other combination of type/depth
         * making RGB 16 bpp case special for some reason. Therefore todo = TRUE.
         */
        { 16, PNG_COLOR_TYPE_RGB,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB, TRUE, TRUE },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceRGB, TRUE, TRUE },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceRGB, TRUE, TRUE }}},

        /*  5 */
        { 1, PNG_COLOR_TYPE_GRAY,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat1bppIndexed, ImageFlagsColorSpaceRGB },
           { PixelFormat1bppIndexed, ImageFlagsColorSpaceRGB }}},
        { 2, PNG_COLOR_TYPE_GRAY,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY }}},
        { 4, PNG_COLOR_TYPE_GRAY,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY }}},
        { 8, PNG_COLOR_TYPE_GRAY,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY }}},
        { 16, PNG_COLOR_TYPE_GRAY,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceGRAY }}},

        /* 10 */
        { 1, PNG_COLOR_TYPE_PALETTE,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat1bppIndexed, ImageFlagsColorSpaceRGB },
           { 0, 0 }}},
        { 2, PNG_COLOR_TYPE_PALETTE,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { 0, 0 }}},
        { 4, PNG_COLOR_TYPE_PALETTE,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat4bppIndexed, ImageFlagsColorSpaceRGB },
           { 0, 0 }}},
        { 8, PNG_COLOR_TYPE_PALETTE,
          {{ PixelFormat32bppARGB, ImageFlagsColorSpaceRGB },
           { PixelFormat8bppIndexed, ImageFlagsColorSpaceRGB },
           { 0, 0 }}},
        { 16, PNG_COLOR_TYPE_PALETTE },
    };
    BYTE buf[sizeof(png_1x1_data)];
    GpStatus status;
    GpImage *image;
    ImageType type;
    PixelFormat format;
    ImageFlags flags;
    BOOL valid;
    int i, j, PLTE_off = 0, tRNS_off = 0;
    const ImageFlags color_space_mask = ImageFlagsColorSpaceRGB | ImageFlagsColorSpaceCMYK | ImageFlagsColorSpaceGRAY | ImageFlagsColorSpaceYCBCR | ImageFlagsColorSpaceYCCK;

    memcpy(buf, png_1x1_data, sizeof(png_1x1_data));
    buf[24] = 2;
    buf[25] = PNG_COLOR_TYPE_PALETTE;
    image = load_image(buf, sizeof(buf), TRUE, FALSE);
    status = GdipGetImageFlags(image, &flags);
    expect(status, Ok);
    ok((flags & color_space_mask) == ImageFlagsColorSpaceRGB || broken(flags == 0x12006) /* before win7 */,
       "flags = %#x\n", flags);
    if ((flags & color_space_mask) != ImageFlagsColorSpaceRGB) {
        GdipDisposeImage(image);
        win_skip("broken PNG color space support\n");
        return;
    }
    GdipDisposeImage(image);

    for (i = 0; i < sizeof(png_1x1_data) - 4; i++)
    {
        if (!memcmp(buf + i, "tRNS", 4))
            tRNS_off = i;
        else if (!memcmp(buf + i, "PLTE", 4))
            PLTE_off = i;
    }

    ok(PLTE_off && tRNS_off, "PLTE offset %d, tRNS offset %d\n", PLTE_off, tRNS_off);
    if (!PLTE_off || !tRNS_off) return;

    /* In order to test the image data with and without PLTE and tRNS
     * chunks, we mask the chunk name with private one (tEST).
     */

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        for (j = 0; j < 3; j++)
        {
            memcpy(buf, png_1x1_data, sizeof(png_1x1_data));
            buf[24] = td[i].bit_depth;
            buf[25] = td[i].color_type;
            if (j >=1) memcpy(buf + tRNS_off, "tEST", 4);
            if (j >=2) memcpy(buf + PLTE_off, "tEST", 4);

            valid = (td[i].t[j].format != 0) || (td[i].t[j].flags != 0);
            image = load_image(buf, sizeof(buf), valid, td[i].t[j].todo_load);
            todo_wine_if(td[i].t[j].todo_load)
            if (valid)
                ok(image != NULL, "%d %d: failed to load image data\n", i, j);
            else
            {
                ok(image == NULL, "%d %d: succeed to load image data\n", i, j);
                if (image) GdipDisposeImage(image);
                image = NULL;
            }
            if (!image) continue;

            status = GdipGetImageType(image, &type);
            ok(status == Ok, "%d %d: GdipGetImageType error %d\n", i, j, status);
            ok(type == ImageTypeBitmap, "%d %d: wrong image type %d\n", i, j, type);

            status = GdipGetImagePixelFormat(image, &format);
            expect(Ok, status);
            todo_wine_if(td[i].t[j].todo)
            ok(format == td[i].t[j].format,
               "%d %d: expected %#x, got %#x\n", i, j, td[i].t[j].format, format);

            status = GdipGetImageFlags(image, &flags);
            expect(Ok, status);
            ok((flags & color_space_mask) == td[i].t[j].flags,
               "%d %d: expected %#x, got %#x\n", i, j, td[i].t[j].flags, flags);
            GdipDisposeImage(image);
        }
    }
}
#undef PNG_COLOR_TYPE_GRAY
#undef PNG_COLOR_TYPE_RGB
#undef PNG_COLOR_TYPE_PALETTE
#undef PNG_COLOR_TYPE_GRAY_ALPHA
#undef PNG_COLOR_TYPE_RGB_ALPHA

static void test_GdipLoadImageFromStream(void)
{
    IStream *stream;
    GpStatus status;
    GpImage *image;
    HGLOBAL hglob;
    BYTE *data;
    HRESULT hr;

    status = GdipLoadImageFromStream(NULL, NULL);
    ok(status == InvalidParameter, "Unexpected return value %d.\n", status);

    image = (void *)0xdeadbeef;
    status = GdipLoadImageFromStream(NULL, &image);
    ok(status == InvalidParameter, "Unexpected return value %d.\n", status);
    ok(image == (void *)0xdeadbeef, "Unexpected image pointer.\n");

    hglob = GlobalAlloc(0, sizeof(pngimage));
    data = GlobalLock (hglob);
    memcpy(data, pngimage, sizeof(pngimage));
    GlobalUnlock(hglob);

    hr = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream.\n");

    status = GdipLoadImageFromStream(stream, NULL);
    ok(status == InvalidParameter, "Unexpected return value %d.\n", status);

    IStream_Release(stream);
}

static BYTE *init_bitmap(UINT *width, UINT *height, UINT *stride)
{
    BYTE *src;
    UINT i, j, scale;

    *width = 256;
    *height = 256;
    *stride = (*width * 3 + 3) & ~3;
    trace("width %d, height %d, stride %d\n", *width, *height, *stride);

    src = HeapAlloc(GetProcessHeap(), 0, *stride * *height);

    scale = 256 / *width;
    if (!scale) scale = 1;

    for (i = 0; i < *height; i++)
    {
        for (j = 0; j < *width; j++)
        {
            src[i * *stride + j*3 + 0] = scale * i;
            src[i * *stride + j*3 + 1] = scale * (255 - (i+j)/2);
            src[i * *stride + j*3 + 2] = scale * j;
        }
    }

    return src;
}

static void test_GdipInitializePalette(void)
{
    GpStatus status;
    BYTE *data;
    GpBitmap *bitmap;
    ColorPalette *palette;
    UINT width, height, stride;

    pGdipInitializePalette = (void *)GetProcAddress(GetModuleHandleA("gdiplus.dll"), "GdipInitializePalette");
    if (!pGdipInitializePalette)
    {
        win_skip("GdipInitializePalette is not supported on this platform\n");
        return;
    }

    data = init_bitmap(&width, &height, &stride);

    status = GdipCreateBitmapFromScan0(width, height, stride, PixelFormat24bppRGB, data, &bitmap);
    expect(Ok, status);

    palette = GdipAlloc(sizeof(*palette) + sizeof(ARGB) * 255);

    palette->Flags = 0;
    palette->Count = 15;
    status = pGdipInitializePalette(palette, PaletteTypeOptimal, 16, FALSE, bitmap);
    expect(GenericError, status);

    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeOptimal, 16, FALSE, NULL);
    expect(InvalidParameter, status);

    memset(palette->Entries, 0x11, sizeof(ARGB) * 256);
    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeCustom, 16, FALSE, NULL);
    expect(Ok, status);
    expect(0, palette->Flags);
    expect(256, palette->Count);
    expect(0x11111111, palette->Entries[0]);
    expect(0x11111111, palette->Entries[128]);
    expect(0x11111111, palette->Entries[255]);

    memset(palette->Entries, 0x11, sizeof(ARGB) * 256);
    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeFixedBW, 0, FALSE, bitmap);
    expect(Ok, status);
todo_wine
    expect(0x200, palette->Flags);
    expect(2, palette->Count);
    expect(0xff000000, palette->Entries[0]);
    expect(0xffffffff, palette->Entries[1]);

    memset(palette->Entries, 0x11, sizeof(ARGB) * 256);
    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeFixedHalftone8, 1, FALSE, NULL);
    expect(Ok, status);
todo_wine
    expect(0x300, palette->Flags);
    expect(16, palette->Count);
    expect(0xff000000, palette->Entries[0]);
    expect(0xffc0c0c0, palette->Entries[8]);
    expect(0xff008080, palette->Entries[15]);

    memset(palette->Entries, 0x11, sizeof(ARGB) * 256);
    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeFixedHalftone8, 1, FALSE, bitmap);
    expect(Ok, status);
todo_wine
    expect(0x300, palette->Flags);
    expect(16, palette->Count);
    expect(0xff000000, palette->Entries[0]);
    expect(0xffc0c0c0, palette->Entries[8]);
    expect(0xff008080, palette->Entries[15]);

    memset(palette->Entries, 0x11, sizeof(ARGB) * 256);
    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeFixedHalftone252, 1, FALSE, bitmap);
    expect(Ok, status);
todo_wine
    expect(0x800, palette->Flags);
    expect(252, palette->Count);
    expect(0xff000000, palette->Entries[0]);
    expect(0xff990066, palette->Entries[128]);
    expect(0xffffffff, palette->Entries[251]);

    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeOptimal, 1, FALSE, bitmap);
    expect(InvalidParameter, status);

    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeOptimal, 2, FALSE, bitmap);
    expect(Ok, status);
    expect(0, palette->Flags);
    expect(2, palette->Count);

    palette->Flags = 0;
    palette->Count = 256;
    status = pGdipInitializePalette(palette, PaletteTypeOptimal, 16, FALSE, bitmap);
    expect(Ok, status);
    expect(0, palette->Flags);
    expect(16, palette->Count);

    /* passing invalid enumeration palette type crashes under most Windows versions */

    GdipFree(palette);
    GdipDisposeImage((GpImage *)bitmap);
}

#include "pshpack2.h"
static const struct tiff_1x1_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[12];
    ULONG next_IFD;
    struct IFD_rational res;
    short palette_data[3][256];
    short bps_data[4];
    BYTE pixel_data[32];
} tiff_1x1_data =
{
#ifdef WORDS_BIGENDIAN
    'M' | 'M' << 8,
#else
    'I' | 'I' << 8,
#endif
    42,
    FIELD_OFFSET(struct tiff_1x1_data, number_of_entries),
    12,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 3, FIELD_OFFSET(struct tiff_1x1_data, bps_data) }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 2 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_1x1_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 3 }, /* SAMPLESPERPIXEL */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1x1_data, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1x1_data, res) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0x140, IFD_SHORT, 256*3, FIELD_OFFSET(struct tiff_1x1_data, palette_data) } /* COLORMAP */
    },
    0,
    { 96, 1 },
    { { 0 } },
    { 8,8,8,0 },
    { 1,0,2,3,4,5,6,7,8,9,0,1,2,3,4,5 }
};
#include "poppack.h"

static void test_tiff_color_formats(void)
{
    static const struct
    {
        int photometric; /* PhotometricInterpretation */
        int samples; /* SamplesPerPixel */
        int bps; /* BitsPerSample */
        PixelFormat format;
    } td[] =
    {
        /* 2 - RGB */
        { 2, 3, 1, PixelFormat24bppRGB },
        { 2, 3, 4, PixelFormat24bppRGB },
        { 2, 3, 8, PixelFormat24bppRGB },
        { 2, 3, 16, PixelFormat48bppRGB },
        { 2, 3, 24, 0 },
#if 0 /* FIXME */
        { 2, 3, 32, 0 },
#endif
        { 2, 4, 1, PixelFormat32bppARGB },
        { 2, 4, 4, PixelFormat32bppARGB },
        { 2, 4, 8, PixelFormat32bppARGB },
        { 2, 4, 16, PixelFormat48bppRGB },
        { 2, 4, 24, 0 },
        { 2, 4, 32, 0 },
        /* 1 - BlackIsZero (Bilevel) */
        { 1, 1, 1, PixelFormat1bppIndexed },
#if 0 /* FIXME: PNG vs TIFF mismatch */
        { 1, 1, 4, PixelFormat8bppIndexed },
#endif
        { 1, 1, 8, PixelFormat8bppIndexed },
        { 1, 1, 16, PixelFormat32bppARGB },
        { 1, 1, 24, 0 },
        { 1, 1, 32, PixelFormat32bppARGB },
        /* 3 - Palette Color */
        { 3, 1, 1, PixelFormat1bppIndexed },
        { 3, 1, 4, PixelFormat4bppIndexed },
        { 3, 1, 8, PixelFormat8bppIndexed },
#if 0 /* FIXME: for some reason libtiff replaces photometric 3 by 1 for bps > 8 */
        { 3, 1, 16, 0 },
        { 3, 1, 24, 0 },
        { 3, 1, 32, 0 },
#endif
        /* 5 - Separated */
        { 5, 4, 1, 0 },
        { 5, 4, 4, 0 },
        { 5, 4, 8, PixelFormat32bppCMYK },
        { 5, 4, 16, PixelFormat48bppRGB },
        { 5, 4, 24, 0 },
        { 5, 4, 32, 0 },
    };
    BYTE buf[sizeof(tiff_1x1_data)];
    GpStatus status;
    GpImage *image;
    UINT count, i;
    struct IFD_entry *tag, *tag_photo = NULL, *tag_bps = NULL, *tag_samples = NULL, *tag_colormap = NULL;
    short *bps;
    ImageType type;
    PixelFormat format;

    memcpy(buf, &tiff_1x1_data, sizeof(tiff_1x1_data));

    count = *(short *)(buf + tiff_1x1_data.dir_offset);
    tag = (struct IFD_entry *)(buf + tiff_1x1_data.dir_offset + sizeof(short));

    /* verify the TIFF structure */
    for (i = 0; i < count; i++)
    {
        if (tag[i].id == 0x102) /* BitsPerSample */
            tag_bps = &tag[i];
        else if (tag[i].id == 0x106) /* PhotometricInterpretation */
            tag_photo = &tag[i];
        else if (tag[i].id == 0x115) /* SamplesPerPixel */
            tag_samples = &tag[i];
        else if (tag[i].id == 0x140) /* ColorMap */
            tag_colormap = &tag[i];
    }

    ok(tag_bps && tag_photo && tag_samples && tag_colormap, "tag 0x102,0x106,0x115 or 0x140 is missing\n");
    if (!tag_bps || !tag_photo || !tag_samples || !tag_colormap) return;

    ok(tag_bps->type == IFD_SHORT, "tag 0x102 should have type IFD_SHORT\n");
    bps = (short *)(buf + tag_bps->value);
    ok(bps[0] == 8 && bps[1] == 8 && bps[2] == 8 && bps[3] == 0,
       "expected bps 8,8,8,0 got %d,%d,%d,%d\n", bps[0], bps[1], bps[2], bps[3]);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        tag_colormap->count = (1 << td[i].bps) * 3;
        tag_photo->value = td[i].photometric;
        tag_bps->count = td[i].samples;
        tag_samples->value = td[i].samples;

        if (td[i].samples == 1)
            tag_bps->value = td[i].bps;
        else if (td[i].samples == 2)
            tag_bps->value = MAKELONG(td[i].bps, td[i].bps);
        else if (td[i].samples == 3)
        {
            tag_bps->value = (BYTE *)bps - buf;
            bps[0] = bps[1] = bps[2] = td[i].bps;
        }
        else if (td[i].samples == 4)
        {
            tag_bps->value = (BYTE *)bps - buf;
            bps[0] = bps[1] = bps[2] = bps[3] = td[i].bps;
        }
        else
        {
            ok(0, "%u: unsupported samples count %d\n", i, td[i].samples);
            continue;
        }

        image = load_image(buf, sizeof(buf), TRUE, FALSE);
        if (!td[i].format)
#ifdef __REACTOS__
            ok(!image || (((GetVersion() & 0xFF) < 6) &&
               (i == 19 || i == 20)),
#else
            ok(!image,
#endif
               "%u: (%d,%d,%d) TIFF image loading should have failed\n", i, td[i].photometric, td[i].samples, td[i].bps);
        else
            ok(image != NULL || broken(!image) /* XP */, "%u: failed to load TIFF image data (%d,%d,%d)\n",
               i, td[i].photometric, td[i].samples, td[i].bps);
        if (!image) continue;

        status = GdipGetImageType(image, &type);
        ok(status == Ok, "%u: GdipGetImageType error %d\n", i, status);
        ok(type == ImageTypeBitmap, "%u: wrong image type %d\n", i, type);

        status = GdipGetImagePixelFormat(image, &format);
        expect(Ok, status);
#ifdef __REACTOS__
        ok(format == td[i].format || (((GetVersion() & 0xFF) < 6) &&
           ((i == 0 && format == 0) ||
            (i == 1 && format == 0) ||
            (i == 5 && format == 0) ||
            (i == 6 && format == 0) ||
            (i == 13 && format == 0) ||
            (i == 19 && format == PixelFormat32bppARGB) ||
            (i == 20 && format == PixelFormat32bppARGB) ||
            (i == 21 && format == PixelFormat32bppARGB) ||
            (i == 22 && format == PixelFormat32bppARGB))),
#else
        ok(format == td[i].format,
#endif
           "%u: expected %#x, got %#x\n", i, td[i].format, format);

        GdipDisposeImage(image);
    }
}

START_TEST(image)
{
    HMODULE mod = GetModuleHandleA("gdiplus.dll");
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    pGdipBitmapGetHistogramSize = (void*)GetProcAddress(mod, "GdipBitmapGetHistogramSize");
    pGdipBitmapGetHistogram = (void*)GetProcAddress(mod, "GdipBitmapGetHistogram");
    pGdipImageSetAbort = (void*)GetProcAddress(mod, "GdipImageSetAbort");

    test_tiff_color_formats();
    test_GdipInitializePalette();
    test_png_color_formats();
    test_supported_encoders();
    test_CloneBitmapArea();
    test_ARGB_conversion();
    test_DrawImage_scale();
    test_image_format();
    test_DrawImage();
    test_DrawImage_SourceCopy();
    test_GdipDrawImagePointRect();
    test_bitmapbits();
    test_tiff_palette();
    test_GdipGetAllPropertyItems();
    test_tiff_properties();
    test_gif_properties();
    test_image_properties();
    test_Scan0();
    test_FromGdiDib();
    test_GetImageDimension();
    test_GdipImageGetFrameDimensionsCount();
    test_LoadingImages();
    test_SavingImages();
    test_encoders();
    test_LockBits();
    test_LockBits_UserBuf();
    test_GdipCreateBitmapFromHBITMAP();
    test_GdipGetImageFlags();
    test_GdipCloneImage();
    test_testcontrol();
    test_fromhicon();
    test_getrawformat();
    test_loadwmf();
    test_createfromwmf();
    test_createfromwmf_noplaceable();
    test_resolution();
    test_createhbitmap();
    test_getthumbnail();
    test_getsetpixel();
    test_palette();
    test_colormatrix();
    test_gamma();
    test_multiframegif();
    test_rotateflip();
    test_remaptable();
    test_colorkey();
    test_dispose();
    test_createeffect();
    test_getadjustedpalette();
    test_histogram();
    test_imageabort();
    test_GdipLoadImageFromStream();

    GdiplusShutdown(gdiplusToken);
}
