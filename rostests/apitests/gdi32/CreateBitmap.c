/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

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
    ok(hbmp != 0, "\n");
    DeleteObject(hbmp);
    SetLastError(0);
    hbmp = CreateBitmap(1, 0x1FFFFFFF, 1, 1, NULL);
    ok(hbmp == 0, "\n");
    ok_err(0);

    /* Check for overflow in width * height */
    hbmp = CreateBitmap(0x20000, 0x1FFFF, 1, 1, NULL);
    ok(hbmp != 0, "\n");
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
    int result;

    hbmp = CreateBitmap(0, 0, 0, 0, NULL);
    ok(hbmp != 0, "should get a 1x1 bitmap\n");
    ok(hbmp == GetStockObject(DEFAULT_BITMAP), "\n");
    result = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(result > 0, "result = %d\n", result);
    ok(bitmap.bmType == 0, "bmType = %ld\n", bitmap.bmType);
    ok(bitmap.bmWidth == 1, "bmWidth = %ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 1, "bmHeight = %ld\n", bitmap.bmHeight);
    ok(bitmap.bmWidthBytes == 2, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "bmPlanes = %d\n", bitmap.bmPlanes);
    ok(bitmap.bmBitsPixel == 1, "bmBitsPixel = %d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == 0, "bmBits = %p\n", bitmap.bmBits);
    DeleteObject(hbmp);

    hbmp = CreateBitmap(1, 2, 1, 1, NULL);
    ok(hbmp != 0, "should get a 1x2 bitmap\n");
    result = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(result > 0, "result = %d\n", result);
    ok(bitmap.bmType == 0, "bmType = %ld\n", bitmap.bmType);
    ok(bitmap.bmWidth == 1, "bmWidth = %ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 2, "bmHeight = %ld\n", bitmap.bmHeight);
    ok(bitmap.bmWidthBytes == 2, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "bmPlanes = %d\n", bitmap.bmPlanes);
    ok(bitmap.bmBitsPixel == 1, "bmBitsPixel = %d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == 0, "bmBits = %p\n", bitmap.bmBits);
    DeleteObject(hbmp);


}

START_TEST(CreateBitmap)
{
    Test_CreateBitmap_Params();
    Test_CreateBitmap();
}

