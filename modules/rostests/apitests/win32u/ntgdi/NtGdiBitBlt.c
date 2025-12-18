/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtGdiLineTo
 * COPYRIGHT:   Copyright 2007-2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "../win32nt.h"
#include <gditools.h>

static HDC ghdcDDB1, ghdcDDB32;
static BOOL gbUseCLR_INVALID;

static void Test_NtGdiBitBlt_generic(void)
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

    // 1BPP DDB -> 32 BPP DDB -> translates to dest DC fore/back colors
    SetBkColor(ghdcDDB1, 0xFEFEFE);
    SetTextColor(ghdcDDB1, 0x040404);
    SetBkColor(ghdcDDB32, 0xFF00FF);
    SetTextColor(ghdcDDB32, 0x00FF00);
    NtGdiSetPixel(ghdcDDB1, 0, 0, 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB32, 0, 0, 1, 1, ghdcDDB1, 0, 0, SRCCOPY, 0xF0F0F0, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB32, 0, 0), 0x00FF00);
    NtGdiSetPixel(ghdcDDB1, 0, 0, 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB32, 0, 0, 1, 1, ghdcDDB1, 0, 0, SRCCOPY, 0xF0F0F0, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB32, 0, 0), 0xFF00FF);

    // 1BPP DIB -> 1BPP DDB -> use crBackColor parameter, or src DC back color if CLR_INVALID
    SetBkColor(ghdcDIB1, 0xFFFFFF);
    SetTextColor(ghdcDIB1, 0x000000);
    SetBkColor(ghdcDDB1, 0xFFFFFF);
    SetTextColor(ghdcDDB1, 0x000000);
    NtGdiSetPixel(ghdcDIB1, 0, 0, 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0x000000, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0xFFFFFF, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, CLR_INVALID, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);
    NtGdiSetPixel(ghdcDIB1, 0, 0, 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0x000000, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0xFFFFFF, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, CLR_INVALID, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);

    SetBkColor(ghdcDIB1, 0x000000);
    SetTextColor(ghdcDIB1, 0xFFFFFF);
    NtGdiSetPixel(ghdcDIB1, 0, 0, 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0x000000, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0xFFFFFF, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, CLR_INVALID, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);
    NtGdiSetPixel(ghdcDIB1, 0, 0, 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0x000000, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, 0xFFFFFF, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0x000000);
    ok_eq_bool(NtGdiBitBlt(ghdcDDB1, 0, 0, 1, 1, ghdcDIB1, 0, 0, SRCCOPY, CLR_INVALID, 0), TRUE);
    ok_eq_hex(NtGdiGetPixel(ghdcDDB1, 0, 0), 0xFFFFFF);

    SelectObject(hdc2, hOldBmp2);
    SelectObject(hdc1, hOldBmp1);

    DeleteObject(hbmp2);
    DeleteObject(hbmp1);

    DeleteDC(hdc1);
    DeleteDC(hdc2);
}

static
COLORREF
Do_BitBlt(
    COLORREF crColor,
    HDC hdcSrc,
    COLORREF crSrcBack,
    COLORREF crSrcText,
    HDC hdcDst,
    COLORREF crDstBack,
    COLORREF crDstText)
{
    COLORREF cr;
    SetBkColor(hdcSrc, gbUseCLR_INVALID ? crSrcBack : 0xFFFFFF);
    SetTextColor(hdcSrc, crSrcText);
    SetBkColor(hdcDst, crDstBack);
    SetTextColor(hdcDst, crDstText);
    NtGdiSetPixel(hdcSrc, 0, 0, crColor);
    NtGdiBitBlt(hdcDst, 1, 0, 1, 1, hdcSrc, 0, 0, SRCCOPY, gbUseCLR_INVALID ? CLR_INVALID : crSrcBack, 0);
    cr = NtGdiGetPixel(hdcDst, 1, 0);
    return cr;
}

void Test_NtGdiBitBlt_1BPP(void)
{
    /* 1BPP DDB -> 1BPP DDB: bg and fg colors are ignored */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0x000000), 0x000000); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0x000000), 0xFFFFFF); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0xFFFFFF), 0x000000); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x0000FF, 0x00FF00), 0xFFFFFF); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0x0000FF, 0x00FF00), 0xFFFFFF); // inverted src colors, changed dst colors => no inversion

    /* 1BPP DIB -> 1BPP DIB: bg and fg colors are ignored, src DIB colors are mapped to dst DIB colors */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0x000000), 0x000000); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0x000000), 0xFFFFFF); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0xFFFFFF), 0x000000); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, dark dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0x0000FF, 0x00FF00), 0xFFFFFF); // inverted src colors, dark dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src DIB, normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src DIB, normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0x000000); // inverted dst DIB, normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted dst DIB, normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // RB src DIB, normal colors => black (0)
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1_RB, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // RB src DIB, normal colors => black (0)
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0x000000); // RB src DIB, inverted dst DIB, normal colors => black (1)
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1_RB, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0x000000); // RB src DIB, inverted dst DIB, normal colors => black (1)
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // RB dst DIB, normal colors => blue (1)
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // RB dst DIB, normal colors => blue (1)
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // inverted src DIB, normal colors => blue (1)
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // inverted src DIB, normal colors => blue (1)

    /* 1BPP DDB -> 1BPP DIB: black to dest fg, white to dest bg */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, dark dst colors => to black
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, dark dst colors => to black
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFF00FF, 0x00FFFF), 0xFFFFFF); // inverted src colors, bright dst colors => to white
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB1, 0xFF00FF, 0x00FFFF), 0xFFFFFF); // inverted src colors, bright dst colors => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0x000000); // inverted dst DIB, normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB1_InvCol, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted dst DIB, normal colors => no inversion

    /* 1BPP DIB -> 1BPP DDB: src bg color (src translated) determines inversion */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0x000000), 0x000000); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x000000, 0x000000), 0xFFFFFF); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0xFFFFFF), 0x000000); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB1, 0x0000FF, 0x00FF00), 0xFFFFFF); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0x0000FF, 0x00FF00), 0xFFFFFF); // inverted src colors, changed dst colors => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB1, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, changed dst colors => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src bg color => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src bg color => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // inverted src fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src fg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x0000FF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // dark src bg color => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x0000FF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // dark src bg color => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x00FFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // bright src bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x00FFFF, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // bright src bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x0000FF, ghdcDIB1_RB, 0x0000F8, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // RB DIB, close to red src bg color => inversion
    ok_eq_hex(Do_BitBlt(0xFF0000, ghdcDIB1_RB, 0x0000F8, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // RB DIB, close to red src bg color => inversion
    ok_eq_hex(Do_BitBlt(0x0000FF, ghdcDIB1_RB, 0xF80000, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0x000000); // RB DIB, close to blue src bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFF0000, ghdcDIB1_RB, 0xF80000, 0x000000, ghdcDDB1, 0xFFFFFF, 0x000000), 0xFFFFFF); // RB DIB, close to blue src bg color => no inversion

    /* 1BPP DDB -> 32BPP DDB: black to dest fg, white to dest bg */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x0000FF, 0x00FF00), 0x00FF00); // changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x0000FF, 0x00FF00), 0x0000FF); // changed dst colors => to dst bg color
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0x0000FF, 0x00FF00), 0x00FF00); // inverted src colors, changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0x0000FF, 0x00FF00), 0x0000FF); // inverted src colors, changed dst colors => to dst bg color

    /* 1BPP DDB -> 32BPP DIB: black to dest fg, white to dest bg */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => to black
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x0000FF, 0x00FF00), 0x00FF00); // changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x0000FF, 0x00FF00), 0x0000FF); // changed dst colors => to dst bg color
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0x0000FF, 0x00FF00), 0x00FF00); // inverted src colors, changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDDB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0x0000FF, 0x00FF00), 0x0000FF); // inverted src colors, changed dst colors => to dst bg color

    /* 1BPP DIB -> 32BPP DDB: bg and fg colors are ignored */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x000000, 0x000000), 0xFFFFFF); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0xFFFFFF), 0x000000); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDDB32, 0x0000FF, 0x00FF00), 0xFFFFFF); // changed dst colors => to dst bg color
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDDB32, 0x0000FF, 0x00FF00), 0xFFFFFF); // inverted src colors, changed dst colors => to dst bg color

    /* 1BPP DIB -> 32BPP DIB: bg and fg colors are ignored */
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0x000000), 0x000000); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // normal colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0xFFFFFF, 0x000000), 0x000000); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0xFFFFFF, 0x000000), 0xFFFFFF); // inverted src colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0xFFFFFF), 0x000000); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0xFFFFFF), 0xFFFFFF); // inverted dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0x000000), 0x000000); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x000000, 0x000000), 0xFFFFFF); // inverted dst bg color => no inversion
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0xFFFFFF), 0x000000); // inverted dst fg color => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // inverted dst fg color => to white
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x0000FF, 0x00FF00), 0x000000); // changed dst colors => no inversion
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0xFFFFFF, 0x000000, ghdcDIB32, 0x0000FF, 0x00FF00), 0xFFFFFF); // changed dst colors => to dst bg color
    ok_eq_hex(Do_BitBlt(0x000000, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0x0000FF, 0x00FF00), 0x000000); // inverted src colors, changed dst colors => to dst fg color
    ok_eq_hex(Do_BitBlt(0xFFFFFF, ghdcDIB1, 0x000000, 0xFFFFFF, ghdcDIB32, 0x0000FF, 0x00FF00), 0xFFFFFF); // inverted src colors, changed dst colors => to dst bg color
}

START_TEST(NtGdiBitBlt)
{
    ok(GdiToolsInit(), "GdiToolsInit failed\n");

    ULONG cBitsPixel;
    ChangeScreenBpp(32, &cBitsPixel);

    ghdcDDB1 = CreateCompatibleDC(NULL);
    SelectObject(ghdcDDB1, ghbmp1);
    ghdcDDB32 = CreateCompatibleDC(NULL);
    SelectObject(ghdcDDB32, ghbmp32);

    Test_NtGdiBitBlt_generic();
    printf("Testing 1BPP with CLR_INVALID...\n");
    gbUseCLR_INVALID = TRUE;
    Test_NtGdiBitBlt_1BPP();
    printf("Testing 1BPP without CLR_INVALID...\n");
    gbUseCLR_INVALID = FALSE;
    Test_NtGdiBitBlt_1BPP();

    ChangeScreenBpp(cBitsPixel, &cBitsPixel);
}
