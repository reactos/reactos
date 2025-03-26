/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

DWORD WINAPI GetBitmapAttributes(HBITMAP hbm);
HBITMAP WINAPI SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags);
HBITMAP WINAPI ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags);

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
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(-1, 1, 1, 0, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(-1, 1, 1, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(1, -1, 1, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check if an overflow in cPlanes * cBitsPixel is handled */
    SetLastError(0);
    hbmp = CreateBitmap(1, 1, 2, 0x80000004, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check for maximum width */
    hbmp = CreateBitmap(0x7FFFFFF, 1, 1, 1, NULL);
    ok(hbmp != 0, "CreateBitmap failed\n");
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(0x8000000, 1, 1, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Check for maximum height */
    hbmp = CreateBitmap(1, 0x1FFFFF00, 1, 1, NULL);
    //ok(hbmp != 0, "\n"); // fails on windows 2003
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(1, 0x1FFFFFFF, 1, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(0);

    SetLastError(0);
    hbmp = CreateBitmap(1, -1, 1, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test huge allocation (256 GB) */
    SetLastError(0);
    hbmp = CreateBitmap(0x40000, 0x40000, 32, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test planes / bpp */
    hbmp = CreateBitmap(10, 10, 32, 1, NULL);
    ok(hbmp != 0, "CreateBitmap failed\n");
    DeleteObject(hbmp);
    hbmp = CreateBitmap(10, 10, 5, 5, NULL);
    ok(hbmp != 0, "CreateBitmap failed\n");
    DeleteObject(hbmp);

    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 33, 1, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 33, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 6, 6, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 8, 8, NULL);
    ok(hbmp == 0, "CreateBitmap should fail\n");
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

    /* Above 32 bpp should fail */
    hbmp = CreateBitmap(1, 2, 1, 33, NULL);
    ok(hbmp == 0, "should fail\n");

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
           but it can only create 1, 4, 8, 16, 24, 32 bpp bitmaps */
        if (cBitsPixel <= 1)       cExpectedBitsPixel = 1;
        else if (cBitsPixel <= 4)  cExpectedBitsPixel = 4;
        else if (cBitsPixel <= 8)  cExpectedBitsPixel = 8;
        else if (cBitsPixel <= 16) cExpectedBitsPixel = 16;
        else if (cBitsPixel <= 24) cExpectedBitsPixel = 24;
        else if (cBitsPixel <= 32) cExpectedBitsPixel = 32;

        hbmp = CreateBitmap(1, 2, 1, cBitsPixel, NULL);
        ok(hbmp != 0, "should get a 1x2 bitmap %ld\n", cBitsPixel);
        ok_int(GetObject(hbmp, sizeof(bitmap), &bitmap), sizeof(BITMAP));

        /* Calculate expected line width */
        cjWidthBytes = ((bitmap.bmWidth * bitmap.bmBitsPixel + 15) & ~15) >> 3;

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

void Test_BitmapAttributes()
{
    static const WORD pattern_bits[] = { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };
    HBITMAP hbmp,hbmpso,hbmpsoc;
    INT ret;
    BITMAP bm;

    hbmp = CreateBitmap(0, 0, 0, 0, NULL);
    ok(hbmp != 0, "should get a 1x1 bitmap\n");
    ok(hbmp == GetStockObject(DEFAULT_BITMAP), "\n");
    ok(GetBitmapAttributes(hbmp) == 1,"\n");

    ok(SetBitmapAttributes(hbmp, 0x00000002) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000004) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000008) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000010) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000020) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000040) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000080) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000100) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000200) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000400) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0x00000800) == 0,"\n");
    ok(SetBitmapAttributes(hbmp, 0xFFFFFFFF) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000002) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000004) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000008) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000010) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000020) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000040) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000080) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000100) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000200) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000400) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0x00000800) == 0,"\n");
    ok(ClearBitmapAttributes(hbmp, 0xFFFFFFFF) == 0,"\n");
    DeleteObject(hbmp);

    hbmp = CreateBitmap( 16, 8, 1, 1, pattern_bits );
    ok(hbmp != 0, "should get a pattern bitmap\n");
    ok(GetBitmapAttributes(hbmp) == 0,"\n");

    hbmpso = SetBitmapAttributes(hbmp, 1);
    ok(hbmpso != 0, "should get stock pattern bitmap\n");
    ok(GetBitmapAttributes(hbmpso) == 1,"\n");
    ok(hbmpso != hbmp,"\n");

    DeleteObject(hbmpso);
    ret = GetObjectW(hbmpso, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    hbmpsoc = ClearBitmapAttributes(hbmpso, 1);
    ok(hbmpsoc != 0, "should get pattern bitmap\n");
    ok(GetBitmapAttributes(hbmp) == 0,"\n");
    ok(hbmpsoc == hbmp,"\n");

    DeleteObject(hbmp);
    ret = GetObjectW(hbmp, sizeof(bm), &bm);
    ok(ret == 0, "GetObject returned %d\n", ret);
}

START_TEST(CreateBitmap)
{
    Test_CreateBitmap_Params();
    Test_CreateBitmap();
    Test_BitmapAttributes();
}
