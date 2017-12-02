/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetPixel
 * PROGRAMMERS:     Jérôme Gardou
 */

#include "precomp.h"

void Test_GetPixel_1bpp()
{
    HDC hdc;
    HBITMAP hbmp;
    char buffer[] = {0x80, 0x0};
    COLORREF color;

    hbmp = CreateBitmap(2,1,1,1,buffer);
    ok(hbmp != NULL, "Failed to create a monochrom bitmap...\n");
    hdc = CreateCompatibleDC(0);
    hbmp = SelectObject(hdc, hbmp);
    ok(hbmp != NULL, "Could not select the bitmap into the DC.\n");

    color = GetPixel(hdc, 0, 0);
    ok(color == 0xFFFFFF, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 0);
    ok(color == 0, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    SetBkColor(hdc, 0x0000FF);
    SetTextColor(hdc, 0x00FF00);
    color = GetPixel(hdc, 0, 0);
    ok(color == 0xFFFFFF, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 0);
    ok(color == 0, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    SetBkColor(hdc, 0x12345678);
    SetTextColor(hdc, 0x87654321);
    color = GetPixel(hdc, 0, 0);
    ok(color == 0xFFFFFF, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 0);
    ok(color == 0, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    hbmp = SelectObject(hdc, hbmp);
    DeleteObject(hbmp);
    DeleteDC(hdc);
}

START_TEST(GetPixel)
{
    Test_GetPixel_1bpp();
}
