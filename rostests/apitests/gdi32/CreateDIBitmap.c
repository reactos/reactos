/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateDIBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void
Test_CreateDIBitmap1(void)
{
    BITMAPINFO bmi;
    HBITMAP hbmp;
    BITMAP bitmap;
    ULONG bits[128] = {0};
    BYTE rlebits[] = {2, 0, 0, 0, 2, 1, 0, 1};
    HDC hdc;
    int ret;

    hdc = GetDC(0);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 2;
    bmi.bmiHeader.biHeight = 2;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 16;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 1;
    bmi.bmiHeader.biYPelsPerMeter = 1;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bits, &bmi, DIB_RGB_COLORS);
    ok(hbmp != 0, "failed\n");

    ret = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(ret != 0, "failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 2, "\n");
    ok(bitmap.bmHeight == 2, "\n");
    ok(bitmap.bmWidthBytes == 8, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "\n");
    ok(bitmap.bmBits == 0, "\n");

    SetLastError(0);
    bmi.bmiHeader.biCompression = BI_RLE8;
    bmi.bmiHeader.biBitCount = 8;
    bmi.bmiHeader.biSizeImage = 8;
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, rlebits, &bmi, DIB_RGB_COLORS);
    ok(hbmp != 0, "failed\n");
    ok(GetLastError() == 0, "GetLastError() == %ld\n", GetLastError());

    ret = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(ret != 0, "failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 2, "\n");
    ok(bitmap.bmHeight == 2, "\n");
    ok(bitmap.bmWidthBytes == 8, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "\n");
    ok(bitmap.bmBits == 0, "\n");


}



START_TEST(CreateDIBitmap)
{
    Test_CreateDIBitmap1();
}

