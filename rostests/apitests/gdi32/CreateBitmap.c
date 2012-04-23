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

    hbmp = CreateBitmap(1, -1, 1, 0, NULL);
    ok(hbmp == 0, "\n");

    hbmp = CreateBitmap(-1, 1, 1, 0, NULL);
    ok(hbmp == 0, "\n");

}

START_TEST(CreateBitmap)
{
    Test_CreateBitmap();
}

