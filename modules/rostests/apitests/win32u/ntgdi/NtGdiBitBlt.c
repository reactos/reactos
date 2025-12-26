/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiBitBlt
 * PROGRAMMERS:
 */

#include "../win32nt.h"

START_TEST(NtGdiBitBlt)
{
    BOOL bRet;
    HDC hdc1, hdc2;
    HBITMAP hbmp1, hOldBmp1, hbmp2, hOldBmp2;
    DWORD bytes1[4] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00ffffff};
    DWORD bytes2[4] = {0x00000000, 0x0000000, 0x0000000, 0x00000000};

    /* Test NULL dc */
    SetLastError(0xDEADBEEF);
    bRet = NtGdiBitBlt((HDC)0, 0, 0, 10, 10, (HDC)0, 10, 10, SRCCOPY, 0, 0);
    ok_int(bRet, FALSE);
    ok_long(GetLastError(), 0xDEADBEEF);

    /* Test invalid dc */
    SetLastError(0xDEADBEEF);
    bRet = NtGdiBitBlt((HDC)0x123456, 0, 0, 10, 10, (HDC)0x123456, 10, 10, SRCCOPY, 0, 0);
    ok_int(bRet, FALSE);
    ok_long(GetLastError(), 0xDEADBEEF);

    hdc1 = NtGdiCreateCompatibleDC(0);
    ok(hdc1 != NULL, "hdc1 was NULL.\n");

    hdc2 = NtGdiCreateCompatibleDC(0);
    ok(hdc2 != NULL, "hdc2 was NULL.\n");

    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 2;
    bi.bmiHeader.biHeight = -2; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    PVOID pvDibBits;

    hbmp1 = CreateDIBSection(hdc1, &bi, DIB_RGB_COLORS, &pvDibBits, NULL, 0);
    ok(hbmp1 != NULL, "hbmp1 was NULL.\n");
    memcpy(pvDibBits, bytes1, sizeof(bytes1));
    hOldBmp1 = SelectObject(hdc1, hbmp1);

    ok_eq_hex(NtGdiGetPixel(hdc1, 0, 0), 0x000000ff);
    ok_eq_hex(NtGdiGetPixel(hdc1, 0, 1), 0x00ff0000);
    ok_eq_hex(NtGdiGetPixel(hdc1, 1, 0), 0x0000ff00);
    ok_eq_hex(NtGdiGetPixel(hdc1, 1, 1), 0x00ffffff);

    hbmp2 = CreateDIBSection(hdc2, &bi, DIB_RGB_COLORS, &pvDibBits, NULL, 0);
    ok(hbmp2 != NULL, "hbmp2 was NULL.\n");
    memcpy(pvDibBits, bytes2, sizeof(bytes1));
    hOldBmp2 = SelectObject(hdc2, hbmp2);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 1, 1, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x000000ff);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00000000);

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 2, 2, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x00ffffff);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x00000000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00000000);

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 2, 2, -2, -2, hdc1, 2, 2, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x000000ff);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00ff0000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x0000ff00);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00ffffff);

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);
    NtGdiSetPixel(hdc2, 1, 0, 0x00000000);
    NtGdiSetPixel(hdc2, 0, 1, 0x00000000);
    NtGdiSetPixel(hdc2, 1, 1, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 0, 0, 2, 2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 0), 0x000000ff);
    ok_eq_hex(NtGdiGetPixel(hdc2, 0, 1), 0x00ff0000);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 0), 0x0000ff00);
    ok_eq_hex(NtGdiGetPixel(hdc2, 1, 1), 0x00ffffff);

    SelectObject(hdc2, hOldBmp2);
    SelectObject(hdc1, hOldBmp1);

    DeleteObject(hbmp2);
    DeleteObject(hbmp1);

    DeleteDC(hdc1);
    DeleteDC(hdc2);
}
