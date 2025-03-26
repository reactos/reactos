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
    SetLastError(ERROR_SUCCESS);
    bRet = NtGdiBitBlt((HDC)0, 0, 0, 10, 10, (HDC)0, 10, 10, SRCCOPY, 0, 0);
    ok_int(bRet, FALSE);
    ok_long(GetLastError(), ERROR_SUCCESS);

    /* Test invalid dc */
    SetLastError(ERROR_SUCCESS);
    bRet = NtGdiBitBlt((HDC)0x123456, 0, 0, 10, 10, (HDC)0x123456, 10, 10, SRCCOPY, 0, 0);
    ok_int(bRet, FALSE);
    ok_long(GetLastError(), ERROR_SUCCESS);

    hdc1 = NtGdiCreateCompatibleDC(0);
    ok(hdc1 != NULL, "hdc1 was NULL.\n");

    hdc2 = NtGdiCreateCompatibleDC(0);
    ok(hdc2 != NULL, "hdc2 was NULL.\n");

    hbmp1 = NtGdiCreateBitmap(2, 2, 1, 32, (LPBYTE)bytes1 );
    ok(hbmp1 != NULL, "hbmp1 was NULL.\n");
    hOldBmp1 = SelectObject(hdc1, hbmp1);

    ok(NtGdiGetPixel(hdc1, 0, 0) == 0x000000ff, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc1, 0, 1) == 0x00ff0000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc1, 1, 0) == 0x0000ff00, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc1, 1, 1) == 0x00ffffff, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    hbmp2 = NtGdiCreateBitmap(2, 2, 1, 32, (LPBYTE)bytes2 );
    ok(hbmp2 != NULL, "hbmp2 was NULL.\n");
    hOldBmp2 = SelectObject(hdc2, hbmp2);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x00000000, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00000000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x00000000, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00000000, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 1, 1, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x000000ff, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00000000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x00000000, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00000000, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x00000000, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00000000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x00000000, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00000000, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    bRet = NtGdiBitBlt(hdc2, 1, 1, -2, -2, hdc1, 2, 2, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x00ffffff, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00000000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x00000000, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00000000, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 2, 2, -2, -2, hdc1, 2, 2, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x000000ff, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00ff0000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x0000ff00, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00ffffff, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    NtGdiSetPixel(hdc2, 0, 0, 0x00000000);
    NtGdiSetPixel(hdc2, 1, 0, 0x00000000);
    NtGdiSetPixel(hdc2, 0, 1, 0x00000000);
    NtGdiSetPixel(hdc2, 1, 1, 0x00000000);

    bRet = NtGdiBitBlt(hdc2, 0, 0, 2, 2, hdc1, 0, 0, SRCCOPY, 0, 0);
    ok_int(bRet, TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok(NtGdiGetPixel(hdc2, 0, 0) == 0x000000ff, "Pixel[0][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 0));
    ok(NtGdiGetPixel(hdc2, 0, 1) == 0x00ff0000, "Pixel[0][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 0, 1));
    ok(NtGdiGetPixel(hdc2, 1, 0) == 0x0000ff00, "Pixel[1][0] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 0));
    ok(NtGdiGetPixel(hdc2, 1, 1) == 0x00ffffff, "Pixel[1][1] 0x%08x\n", (UINT)NtGdiGetPixel(hdc2, 1, 1));

    SelectObject(hdc2, hOldBmp2);
    SelectObject(hdc1, hOldBmp1);

    DeleteObject(hbmp2);
    DeleteObject(hbmp1);

    DeleteDC(hdc1);
    DeleteDC(hdc2);
}
