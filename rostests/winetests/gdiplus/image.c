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

#define COBJMACROS

#include <math.h>

#include "initguid.h"
#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok((UINT)(got) == (UINT)(expected), "Expected %.8x, got %.8x\n", (UINT)(expected), (UINT)(got))
#define expectf(expected, got) ok(fabs(expected - got) < 0.0001, "Expected %.2f, got %.2f\n", expected, got)

static void expect_guid(REFGUID expected, REFGUID got, int line, BOOL todo)
{
    WCHAR bufferW[39];
    char buffer[39];
    char buffer2[39];

    StringFromGUID2(got, bufferW, sizeof(bufferW)/sizeof(bufferW[0]));
    WideCharToMultiByte(CP_ACP, 0, bufferW, sizeof(bufferW)/sizeof(bufferW[0]), buffer, sizeof(buffer), NULL, NULL);
    StringFromGUID2(expected, bufferW, sizeof(bufferW)/sizeof(bufferW[0]));
    WideCharToMultiByte(CP_ACP, 0, bufferW, sizeof(bufferW)/sizeof(bufferW[0]), buffer2, sizeof(buffer2), NULL, NULL);
    if(todo)
        todo_wine ok_(__FILE__, line)(IsEqualGUID(expected, got), "Expected %s, got %s\n", buffer2, buffer);
    else
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
    GpBitmap *bmp;

    hglob = GlobalAlloc (0, size);
    data = GlobalLock (hglob);
    memcpy(data, buff, size);
    GlobalUnlock(hglob); data = NULL;

    hres = CreateStreamOnHGlobal(hglob, TRUE, &stream);
    ok_(__FILE__, line)(hres == S_OK, "Failed to create a stream\n");
    if(hres != S_OK) return;

    stat = GdipCreateBitmapFromStream(stream, &bmp);
    ok_(__FILE__, line)(stat == Ok, "Failed to create a Bitmap\n");
    if(stat != Ok){
        IStream_Release(stream);
        return;
    }

    expect_rawformat(expected, (GpImage*)bmp, line, todo);

    GdipDisposeImage((GpImage*)bmp);
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
    expect_guid(&FrameDimensionPage, &dimension, __LINE__, TRUE);

    count = 12345;
    stat = GdipImageGetFrameCount((GpImage*)bm, &dimension, &count);
    todo_wine expect(Ok, stat);
    todo_wine expect(1, count);

    GdipBitmapSetPixel(bm, 0, 0, 0xffffffff);

    stat = GdipImageSelectActiveFrame((GpImage*)bm, &dimension, 0);
    expect(Ok, stat);

    /* SelectActiveFrame has no effect on image data of memory bitmaps */
    color = 0xdeadbeef;
    GdipBitmapGetPixel(bm, 0, 0, &color);
    expect(0xffffffff, color);

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
    expect(stat, Ok);

    GdipDisposeImage((GpImage*)bm);
    bm = 0;

    /* re-load and check image stats */
    stat = GdipLoadImageFromFile(filename, (GpImage**)&bm);
    expect(stat, Ok);
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
    BYTE *bits;

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
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biHeight = HEIGHT1;
    bmi.bmiHeader.biWidth = WIDTH1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
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
    todo_wine
    {
        expect(Ok, stat);
    }
    if (stat == Ok)
        GdipDisposeImage((GpImage*)gpbm);

    DeleteObject(hpal);
    DeleteObject(hbm);
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
static void test_getrawformat(void)
{
    test_bufferrawformat((void*)pngimage, sizeof(pngimage), &ImageFormatPNG,  __LINE__, FALSE);
    test_bufferrawformat((void*)gifimage, sizeof(gifimage), &ImageFormatGIF,  __LINE__, FALSE);
    test_bufferrawformat((void*)bmpimage, sizeof(bmpimage), &ImageFormatBMP,  __LINE__, FALSE);
    test_bufferrawformat((void*)jpgimage, sizeof(jpgimage), &ImageFormatJPEG, __LINE__, FALSE);
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

    stat = GdipBitmapSetPixel(bitmap, 1, -1, 0);
    ok(stat == InvalidParameter ||
       broken(stat == Ok), /* Older gdiplus */
       "Expected InvalidParameter, got %.8x\n", stat);

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

    expect(0xff000000, palette->Entries[0]);
    expect(0xffffffff, palette->Entries[1]);

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

    /* test setting/getting a different palette */
    palette->Entries[1] = 0xffcccccc;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    expect(Ok, stat);

    palette->Entries[1] = 0;

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*256, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(PaletteFlagsHalftone, palette->Flags);
    expect(256, palette->Count);
    expect(0xffcccccc, palette->Entries[1]);

    /* test count < 256 */
    palette->Flags = 12345;
    palette->Count = 3;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    expect(Ok, stat);

    palette->Entries[1] = 0;
    palette->Entries[3] = 0xdeadbeef;

    stat = GdipGetImagePaletteSize((GpImage*)bitmap, &size);
    expect(Ok, stat);
    expect(sizeof(UINT)*2+sizeof(ARGB)*3, size);

    stat = GdipGetImagePalette((GpImage*)bitmap, palette, size);
    expect(Ok, stat);
    expect(12345, palette->Flags);
    expect(3, palette->Count);
    expect(0xffcccccc, palette->Entries[1]);
    expect(0xdeadbeef, palette->Entries[3]);

    /* test count > 256 */
    palette->Count = 257;

    stat = GdipSetImagePalette((GpImage*)bitmap, palette);
    ok(stat == InvalidParameter ||
       broken(stat == Ok), /* Old gdiplus behavior */
       "Expected %.8x, got %.8x\n", InvalidParameter, stat);

    GdipDisposeImage((GpImage*)bitmap);
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
    test_fromhicon();
    test_getrawformat();
    test_createhbitmap();
    test_getsetpixel();
    test_palette();

    GdiplusShutdown(gdiplusToken);
}
