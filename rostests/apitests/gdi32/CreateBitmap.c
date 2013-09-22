/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>

#define DEFAULT_BITMAP 21

void Test_CreateBitmap_Params()
{
    HBITMAP hbmp;

    /* All of these should get us the default bitmap */
    hbmp = CreateBitmap(0, 0, 0, 0, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");
    hbmp = CreateBitmap(1, 0, 0, 0, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");
    hbmp = CreateBitmap(0, 1, 0, 0, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");
    hbmp = CreateBitmap(0, 1, 1, 0, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");
    hbmp = CreateBitmap(0, 1, 63, 33, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");
    hbmp = CreateBitmap(0, -4, -32, 233, NULL);
    ok(hbmp == GetStockObject(21), "should get the default bitmap\n");

    SetLastError(0);
    hbmp = CreateBitmap(1, -1, 1, 0, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(-1, 1, 1, 0, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(-1, 1, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(1, -1, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check if an overflow in cPlanes * cBitsPixel is handled */
    SetLastError(0);
    hbmp = CreateBitmap(1, 1, 2, 0x80000004, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check for maximum width */
    hbmp = CreateBitmap(0x7FFFFFF, 1, 1, 1, NULL);
    ok(hbmp != 0, "\n");
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(0x8000000, 1, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check for maximum height */
    hbmp = CreateBitmap(1, 0x1FFFFF00, 1, 1, NULL);
    //ok(hbmp != 0, "\n"); // fails on windows 2003
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(1, 0x1FFFFFFF, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(0);

    /* Check for overflow in width * height */
    hbmp = CreateBitmap(0x20000, 0x1FFFF, 1, 1, NULL);
    //ok(hbmp != 0, "\n"); // fails on windows 2003
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(0x20000, 0x20000, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(0);

    /* Check huge allocation */
    SetLastError(0);
    hbmp = CreateBitmap(0x2000, 0x20000, 32, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

}

void Test_CreateBitmap()
{
    HBITMAP hbmp;
    BITMAP bitmap;
    ULONG cjWidthBytes, cBitsPixel, cExpectedBitsPixel;

    hbmp = CreateBitmap(0, 0, 0, 0, NULL);
    ok(hbmp != 0, "should get a 1x1 bitmap\n");
    ok(hbmp == GetStockObject(DEFAULT_BITMAP), "\n");
    ok_int(GetObject(hbmp, sizeof(bitmap), &bitmap), sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 1);
    ok_int(bitmap.bmHeight, 1);
    ok_int(bitmap.bmWidthBytes, 2);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 1);
    ok_ptr(bitmap.bmBits, 0);
    DeleteObject(hbmp);

    hbmp = CreateBitmap(1, 2, 1, 1, NULL);
    ok(hbmp != 0, "should get a 1x2 bitmap\n");
    ok_int(GetObject(hbmp, sizeof(bitmap), &bitmap), sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 1);
    ok_int(bitmap.bmHeight, 2);
    ok_int(bitmap.bmWidthBytes, 2);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 1);
    ok_ptr(bitmap.bmBits, 0);
    DeleteObject(hbmp);

    for (cBitsPixel = 0; cBitsPixel <= 32; cBitsPixel++)
    {
        /* CreateBitmap API accepts any number as BitsPixels param.
           but it just can create 1, 4, 8, 16, 24, 32 bpp Bitmaps */
        if (cBitsPixel <= 1) cExpectedBitsPixel = 1;
        else if (cBitsPixel <= 4) cExpectedBitsPixel = 4;
        else if (cBitsPixel <= 8) cExpectedBitsPixel = 8;
        else if (cBitsPixel <= 16) cExpectedBitsPixel = 16;
        else if (cBitsPixel <= 24) cExpectedBitsPixel = 24;
        else if (cBitsPixel <= 32) cExpectedBitsPixel = 32;

        hbmp = CreateBitmap(1, 2, 1, cBitsPixel, NULL);
        ok(hbmp != 0, "should get a 1x2 bitmap %ld\n", cBitsPixel);
        ok_int(GetObject(hbmp, sizeof(bitmap), &bitmap), sizeof(BITMAP));

        /* calculate expected line width */
        cjWidthBytes = ((((ULONG)bitmap.bmWidth) * ((ULONG)bitmap.bmBitsPixel) + 15) & ~15) >> 3;

        ok_int(bitmap.bmType, 0);
        ok_int(bitmap.bmWidth, 1);
        ok_int(bitmap.bmHeight, 2);
        ok_int(bitmap.bmPlanes, 1);
        ok_int(bitmap.bmBitsPixel, cExpectedBitsPixel);
        ok_int(bitmap.bmWidthBytes, cjWidthBytes);
        ok_ptr(bitmap.bmBits, 0);
        DeleteObject(hbmp);

    }

    hbmp = CreateBitmap(1, 2, 1, 33, NULL);
    ok(hbmp == 0, "Expected failure for 33 bpp\n");



}

START_TEST(CreateBitmap)
{
    //Test_CreateBitmap_Params();
    Test_CreateBitmap();

}
