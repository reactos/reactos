/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtGdiSetPixel
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "../win32nt.h"
#include <gditools.h>

#define DIBINDEX(n) MAKELONG((n),0x10FF)

static void Test_NtGdiSetPixel_generic(void)
{
    COLORREF cr;

    /* Test hdc = NULL */
    SetLastError(0xdeadbeef);
    cr = NtGdiSetPixel(NULL, 0, 0, 0);
    ok_eq_hex(cr, CLR_INVALID);
    todo_ros ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test invalid hdc */
    SetLastError(0xdeadbeef);
    cr = NtGdiSetPixel((HDC)0x12345678, 0, 0, 0);
    ok_eq_hex(cr, CLR_INVALID);
    todo_ros ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test empty memory DC */
    SetLastError(0xdeadbeef);
    HDC hdcMemEmpty = CreateCompatibleDC(NULL);
    cr = NtGdiSetPixel(hdcMemEmpty, 0, 0, 0);
    ok_eq_hex(cr, CLR_INVALID);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test Info-DC */
    cr = NtGdiSetPixel(ghdcInfo, 0, 0, 0x123456);
    todo_ros ok_eq_hex(cr, (GetNTVersion() >= _WIN32_WINNT_VISTA) ? 0x123456 : CLR_INVALID);

    /* Test bounds */
    BITMAP bm = { 0 };
    GetObject(ghbmpDIB32, sizeof(BITMAP), &bm);
    cr = NtGdiSetPixel(ghdcDIB32, -1, 0, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    cr = NtGdiSetPixel(ghdcDIB32, 0, -1, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    cr = NtGdiSetPixel(ghdcDIB32, bm.bmWidth - 1, 0, 0x123456);
    ok_eq_hex(cr, 0x123456);
    cr = NtGdiSetPixel(ghdcDIB32, bm.bmWidth, 0, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    cr = NtGdiSetPixel(ghdcDIB32, 0, bm.bmHeight - 1, 0x123456);
    ok_eq_hex(cr, 0x123456);
    cr = NtGdiSetPixel(ghdcDIB32, 0, bm.bmHeight, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test WindowOrg */
    ((PULONG)gpvDIB32)[0] = 0x00;
    SetWindowOrgEx(ghdcDIB32, 2, 2, NULL);
    cr = NtGdiSetPixel(ghdcDIB32, 1, 1, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    cr = NtGdiSetPixel(ghdcDIB32, 2, 2, 0x123456);
    ok_eq_hex(cr, 0x123456);
    ok_eq_hex(((PULONG)gpvDIB32)[0], 0x00563412);
    SetWindowOrgEx(ghdcDIB32, 0, 0, NULL);

    /* Test ViewPortOrg */
    ((PULONG)gpvDIB32)[0] = 0x00;
    SetViewportOrgEx(ghdcDIB32, 2, 2, NULL);
    cr = NtGdiSetPixel(ghdcDIB32, -3, 0, 0x123456);
    todo_ros ok_eq_hex(cr, CLR_INVALID);
    cr = NtGdiSetPixel(ghdcDIB32, -2, -2, 0x123456);
    ok_eq_hex(cr, 0x123456);
    ok_eq_hex(((PULONG)gpvDIB32)[0], 0x00563412);
    SetViewportOrgEx(ghdcDIB32, 0, 0, NULL);

    /* Test with an arbitrary transform */
    int iOldGraphicsMode = SetGraphicsMode(ghdcDIB32, GM_ADVANCED);
    XFORM xformOld, xform = { 0 };
    GetWorldTransform(ghdcDIB32, &xformOld);
    xform.eM11 = 1.5f; xform.eM12 = 0.5f;
    xform.eM21 = -0.5f; xform.eM22 = 1.0f;
    xform.eDx = 2.0f; xform.eDy = 1.0f;
    SetWorldTransform(ghdcDIB32, &xform);
    memset(gpDIB32, 0, sizeof(*gpDIB32));
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x123456);
    ok_eq_hex(cr, 0x123456);
    ok_eq_hex((*gpDIB32)[1][2], 0x00563412);
    cr = NtGdiSetPixel(ghdcDIB32, 4, 4, 0xABCDEF);
    ok_eq_hex(cr, 0xABCDEF);
    ok_eq_hex((*gpDIB32)[7][6], 0x00EFCDAB);
    SetWorldTransform(ghdcDIB32, &xformOld);
    SetGraphicsMode(ghdcDIB32, iOldGraphicsMode);
}

static void Test_NtGdiSetPixel_1Bpp(void)
{
    COLORREF cr;

    ((PUCHAR)gpvDIB1)[0] = 0x00;
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);

    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x80);

    cr = NtGdiSetPixel(ghdcDIB1, 7, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x81);

    /* The pixel is set to 1, if the sum of R+G+B is >= 383 (0x17F) */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0080807E);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0080807F);
    ok_eq_hex(cr, 0x00FFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FF007F);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FF0080);
    ok_eq_hex(cr, 0x00FFFFFF);

    /* Test if SetBkMode() has any effect */
    ((PUCHAR)gpvDIB1)[0] = 0x00;
    int iOldBkMode = SetBkMode(ghdcDIB1, TRANSPARENT);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    SetBkMode(ghdcDIB1, OPAQUE);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    SetBkMode(ghdcDIB1, iOldBkMode);

    /* Test if SetBkColor() has any effect */
    ((PUCHAR)gpvDIB1)[0] = 0x00;
    COLORREF crOldBkColor = SetBkColor(ghdcDIB1, 0x00010101);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x80);
    SetBkColor(ghdcDIB1, crOldBkColor);

    /* Test if SetDCPenColor() has any effect */
    ((PUCHAR)gpvDIB1)[0] = 0x00;
    COLORREF crOldPenColor = SetDCPenColor(ghdcDIB1, 0x00FEFEFE);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x80);
    SetDCPenColor(ghdcDIB1, crOldPenColor);

    /* Test if SetTextColor() has any effect */
    ((PUCHAR)gpvDIB1)[0] = 0x00;
    COLORREF crOldTextColor = SetTextColor(ghdcDIB1, 0x00FEFEFE);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x00000000);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0x00FFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x80);
    SetTextColor(ghdcDIB1, crOldTextColor);

    /* Test PALETTEINDEX without a palette selected */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(0));
    ok_eq_hex(cr, 0x000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(1));
    ok_eq_hex(cr, 0x000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(7));
    ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(19));
    ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(20));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(255));
    ok_eq_hex(cr, 0);

    /* Test the PALETTEINDEX color mask (16 bits) */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x01FF0007);
    todo_ros ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x01FF8007);
    ok_eq_hex(cr, 0);

    /* Test the PALETTEINDEX flag (crColor & 0x01000000) */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0FF000007);
    todo_ros ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0FE000007);
    ok_eq_hex(cr, 0x000000);

    HPALETTE hOldPal = SelectPalette(ghdcDIB1, ghpal, FALSE);

    /* Test color matching with DC palette again */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0080807E);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x0080807F);
    ok_eq_hex(cr, 0x00FFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FF007F);
    ok_eq_hex(cr, 0x00000000);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x00FF0080);
    ok_eq_hex(cr, 0x00FFFFFF);

    /* Test PALETTEINDEX */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(0));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(7));
    ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(8));
    ok_eq_hex(cr, 0);

    /* Test PALETTERGB (not used) */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTERGB(0x6F, 0x7F, 0x90)); // sum is 382 -> black
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTERGB(0x6F, 0x80, 0x90)); // sum is 383 -> white
    ok_eq_hex(cr, 0x00FFFFFF);

    /* Test DIBINDEX */
    ((PUCHAR)gpvDIB1)[0] = 0x00;
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(0));
    ok_eq_hex(cr, 0);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(1));
    ok_eq_hex(cr, 0xFFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x80);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(2));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(3));
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(0x81));
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(0xFF01));
    ok_eq_hex(cr, 0xFFFFFF);

    /* Test invalid COLORREFs */
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, CLR_INVALID);
    todo_ros ok_eq_hex(cr, 0);
    todo_ros ok_eq_hex(((PUCHAR)gpvDIB1)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, DIBINDEX(100));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, PALETTEINDEX(100));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x03FFFFFF);
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB1, 0, 0, 0x10FE0001);
    todo_ros ok_eq_hex(cr, 0);

    SelectPalette(ghdcDIB1, hOldPal, FALSE);

    /* Test color inverted bitmap */
    ((PUCHAR)gpvDIB1_InvCol)[0] = 0x00;
    cr = NtGdiSetPixel(ghdcDIB1_InvCol, 0, 0, 0x00000000);
    ok_eq_hex(cr, 0x000000);
    ok_eq_hex(((PUCHAR)gpvDIB1_InvCol)[0], 0x80);
    cr = NtGdiSetPixel(ghdcDIB1_InvCol, 0, 0, 0x00FFFFFF);
    ok_eq_hex(cr, 0xFFFFFF);
    ok_eq_hex(((PUCHAR)gpvDIB1_InvCol)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1_InvCol, 0, 0, CLR_INVALID);
    todo_ros ok_eq_hex(cr, 0);
    todo_ros ok_eq_hex(((PUCHAR)gpvDIB1_InvCol)[0], 0x80);

    /* Test Red-Blue bitmap */
    ((PUCHAR)gpvDIB1_RB)[0] = 0x00;
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0x000000);
    ok_eq_hex(cr, 0xFF0000);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0xFFFFFF);
    ok_eq_hex(cr, 0xFF0000);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0xFF0000);
    ok_eq_hex(cr, 0xFF0000);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0x0000FF);
    ok_eq_hex(cr, 0x0000FF);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x80);
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0x00FF00);
    ok_eq_hex(cr, 0xFF0000);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x00);
    cr = NtGdiSetPixel(ghdcDIB1_RB, 0, 0, 0x00FF01);
    ok_eq_hex(cr, 0x0000FF);
    ok_eq_hex(((PUCHAR)gpvDIB1_RB)[0], 0x80);
}

static void Test_NtGdiSetPixel_32Bpp(void)
{
    COLORREF cr;

    /* Normal RGB value */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x00123456);
    ok_eq_hex(cr, 0x123456);
    ok_eq_hex(((PULONG)gpvDIB32)[0], 0x00563412);

    /* Test PALETTEINDEX without a palette selected */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(0));
    ok_eq_hex(cr, 0x000000);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(1));
    ok_eq_hex(cr, 0x000080);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(7));
    ok_eq_hex(cr, 0xC0C0C0);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(19));
    ok_eq_hex(cr, 0xFFFFFF);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(20));
    ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(255));
    ok_eq_hex(cr, 0);

    /* Test the PALETTEINDEX color mask (16 bits) */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x01FF0001);
    todo_ros ok_eq_hex(cr, 0x000080);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x01FF8001);
    ok_eq_hex(cr, 0);

    /* Test the PALETTEINDEX flag (crColor & 0x01000000) */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x0FF000007);
    todo_ros ok_eq_hex(cr, 0xC0C0C0);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, 0x0FE000007);
    ok_eq_hex(cr, 0x000007);

    /* Test PALETTEINDEX with custom DC palette */
    HPALETTE hOldPal = SelectPalette(ghdcDIB32, ghpal, FALSE);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTEINDEX(3));
    ok_eq_hex(cr, 0x605040);
    SelectPalette(ghdcDIB32, hOldPal, FALSE);

    /* Test PALETTERGB (not used) */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, PALETTERGB(0x38, 0x41, 0x9B));
    ok_eq_hex(cr, 0x9B4138);

    /* Test DIBINDEX (invalid) */
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, DIBINDEX(0));
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, DIBINDEX(1));
    todo_ros ok_eq_hex(cr, 0);
    cr = NtGdiSetPixel(ghdcDIB32, 0, 0, DIBINDEX(0xC080));
    todo_ros ok_eq_hex(cr, 0);
}

START_TEST(NtGdiSetPixel)
{
    if (!GdiToolsInit())
    {
        skip("GdiToolsInit failed\n");
        return;
    }

    Test_NtGdiSetPixel_generic();
    Test_NtGdiSetPixel_1Bpp();
    Test_NtGdiSetPixel_32Bpp();
}
