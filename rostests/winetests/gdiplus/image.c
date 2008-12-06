/*
 * Unit test suite for images
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"
#include <math.h>
#include "wingdi.h"

#define expect(expected, got) ok(((UINT)got) == ((UINT)expected), "Expected %.8x, got %.8x\n", (UINT)expected, (UINT)got)

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
    todo_wine{
        expect(Ok, stat);
        ok(NULL != bm, "Expected bitmap to be initialized\n");
    }
    if (stat == Ok)
        GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, -10, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    ok( !bm, "expected null bitmap\n" );
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
    ok(fabs(WIDTH - w) < 0.0001, "Width wrong\n");
    ok(fabs(HEIGHT - h) < 0.0001, "Height wrong\n");
    GdipDisposeImage((GpImage*)bm);
}

static void test_GdipImageGetFrameDimensionsCount(void)
{
    GpBitmap *bm;
    GpStatus stat;
    const REAL WIDTH = 10.0, HEIGHT = 20.0;
    UINT w;

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
    GdipDisposeImage((GpImage*)bm);
}

static void test_LoadingImages(void)
{
    GpStatus stat;

    stat = GdipCreateBitmapFromFile(0, 0);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromFile(0, (GpBitmap**)0xdeadbeef);
    expect(InvalidParameter, stat);

    stat = GdipLoadImageFromFile(0, 0);
    expect(InvalidParameter, stat);

    stat = GdipLoadImageFromFile(0, (GpImage**)0xdeadbeef);
    expect(InvalidParameter, stat);

    stat = GdipLoadImageFromFileICM(0, 0);
    expect(InvalidParameter, stat);

    stat = GdipLoadImageFromFileICM(0, (GpImage**)0xdeadbeef);
    expect(InvalidParameter, stat);
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
    expect(stat, Ok);

    GdipDisposeImage((GpImage*)bm);
    bm = 0;

    /* re-load and check image stats */
    stat = GdipLoadImageFromFile(filename, (GpImage**)&bm);
    expect(stat, Ok);
    if (stat != Ok) goto cleanup;

    stat = GdipGetImageDimension((GpImage*)bm, &w, &h);
    if (stat != Ok) goto cleanup;

    ok((fabs(w - WIDTH) < 0.01) && (fabs(h - HEIGHT) < 0.01),
       "Saved image dimensions are different!\n");

 cleanup:
    GdipFree(codecs);
    if (bm)
        GdipDisposeImage((GpImage*)bm);
    ok(DeleteFileW(filename), "Delete failed.\n");
}

static void test_encoders(void)
{
    GpStatus stat;
    UINT n;
    UINT s;
    ImageCodecInfo *codecs;
    int i;
    int bmp_found;

    static const WCHAR bmp_format[] = {'B', 'M', 'P', 0};

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
            if (CompareStringW(LOCALE_SYSTEM_DEFAULT, 0,
                               codecs[i].FormatDescription, -1,
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

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);

    rect.X = 2;
    rect.Y = 3;
    rect.Width = 4;
    rect.Height = 5;

    /* read-only */
    stat = GdipBitmapLockBits(bm, &rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

    /* read-only, with NULL rect -> whole bitmap lock */
    stat = GdipBitmapLockBits(bm, NULL, ImageLockModeRead, PixelFormat24bppRGB, &bd);
    expect(Ok, stat);
    expect(bd.Width,  WIDTH);
    expect(bd.Height, HEIGHT);

    if (stat == Ok) {
        stat = GdipBitmapUnlockBits(bm, &bd);
        expect(Ok, stat);
    }

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
    BITMAPINFO bmi;

    stat = GdipCreateBitmapFromHBITMAP(NULL, NULL, NULL);
    expect(InvalidParameter, stat);

    hbm = CreateBitmap(WIDTH1, HEIGHT1, 1, 1, NULL);
    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, NULL);
    expect(InvalidParameter, stat);

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    ok(fabs(WIDTH1 - width) < .0001, "width wrong\n");
    ok(fabs(HEIGHT1 - height) < .0001, "height wrong\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);
    GlobalFree(hbm);

    hbm = CreateBitmap(WIDTH2, HEIGHT2, 1, 1, &buff);
    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    ok(fabs(WIDTH2 - width) < .0001, "width wrong\n");
    ok(fabs(HEIGHT2 - height) < .0001, "height wrong\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);
    GlobalFree(hbm);

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biHeight = HEIGHT1;
    bmi.bmiHeader.biWidth = WIDTH1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBSection failed\n");

    stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, &gpbm);
    expect(Ok, stat);
    expect(Ok, GdipGetImageDimension((GpImage*) gpbm, &width, &height));
    ok(fabs(WIDTH1 - width) < .0001, "width wrong\n");
    ok(fabs(HEIGHT1 - height) < .0001, "height wrong\n");
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);

    LogPal = GdipAlloc(sizeof(LOGPALETTE));
    ok(LogPal != NULL, "unable to allocate LOGPALETTE\n");
    LogPal->palVersion = 0x300;
    hpal = CreatePalette((const LOGPALETTE*) LogPal);
    ok(hpal != NULL, "CreatePalette failed\n");
    GdipFree(LogPal);

    stat = GdipCreateBitmapFromHBITMAP(hbm, hpal, &gpbm);
    todo_wine
    {
        expect(Ok, stat);
    }
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);

    GlobalFree(hpal);
    GlobalFree(hbm);
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

    image_src = ((GpImage*)bm);
    stat = GdipCloneImage(image_src, &image_dest);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bm);
    expect(Ok, stat);
    stat = GdipGetImageBounds(image_dest, &rectF, &unit);
    expect(Ok, stat);

    /* Treat FP values carefully */
    ok(fabsf(rectF.Width-WIDTH)<1e-5, "Expected: %d, got %.05f\n", WIDTH, rectF.Width);
    ok(fabsf(rectF.Height-HEIGHT)<1e-5, "Expected: %d, got %.05f\n", HEIGHT, rectF.Height);

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

START_TEST(image)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_Scan0();
    test_GetImageDimension();
    test_GdipImageGetFrameDimensionsCount();
    test_LoadingImages();
    test_SavingImages();
    test_encoders();
    test_LockBits();
    test_GdipCreateBitmapFromHBITMAP();
    test_GdipGetImageFlags();
    test_GdipCloneImage();
    test_testcontrol();

    GdiplusShutdown(gdiplusToken);
}
