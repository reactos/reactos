/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for Rectangle
 * PROGRAMMERS:     Jérôme Gardou
 */

#include "precomp.h"

void Test_Rectangle(void)
{
    HDC hdc;
    HBITMAP hBmp;
    BOOL ret;
    HBRUSH hBrush;
    HPEN hPen;
    COLORREF color;

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to create the DC!\n");
    hBmp = CreateCompatibleBitmap(hdc, 4, 4);
    ok(hBmp != NULL, "Failed to create the Bitmap!\n");
    hBmp = SelectObject(hdc, hBmp);
    ok(hBmp != NULL, "Failed to select the Bitmap!\n");

    hBrush = CreateSolidBrush(RGB(0, 0, 0));
    ok(hBrush != NULL, "Failed to create a solid brush!\n");
    hBrush = SelectObject(hdc, hBrush);
    ok(hBrush != NULL, "Failed to select the brush!\n");

    /* Blank the bitmap */
    ret = BitBlt(hdc, 0, 0, 4, 4, NULL, 0, 0, WHITENESS);
    ok(ret, "BitBlt failed to blank the bitmap!\n");

    /* Try inverted rectangle coordinates */
    ret = Rectangle(hdc, 0, 2, 2, 0);
    ok(ret, "Rectangle failed!");
    color = GetPixel(hdc, 0, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 0, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 0);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 1);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);

    ret = BitBlt(hdc, 0, 0, 4, 4, NULL, 0, 0, WHITENESS);
    ok(ret, "BitBlt failed to blank the bitmap!\n");
    /* Try well ordered rectangle coordinates */
    ret = Rectangle(hdc, 0, 0, 2, 2);
    ok(ret, "Rectangle failed!");
    color = GetPixel(hdc, 0, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 0, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 0);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 1);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);

    /* tests with NULL pen */
    hPen = SelectObject(hdc, GetStockObject(NULL_PEN));

    /* Blank the bitmap */
    ret = BitBlt(hdc, 0, 0, 4, 4, NULL, 0, 0, WHITENESS);
    ok(ret, "BitBlt failed to blank the bitmap!\n");

    ret = Rectangle(hdc, 0, 0, 3, 3);
    ok(ret, "Rectangle failed!");
    color = GetPixel(hdc, 0, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 0, 2);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 0);
    ok( color == RGB(255, 255, 255), "Expected 0x00FFFFFF, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 1);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);

    SelectObject(hdc, hPen);

    /* Same tests with GM_ADVANCED */
    ok(SetGraphicsMode(hdc, GM_ADVANCED) == GM_COMPATIBLE, "Default mode for the DC is not GM_COMPATIBLE.\n");

    /* Blank the bitmap */
    ret = BitBlt(hdc, 0, 0, 4, 4, NULL, 0, 0, WHITENESS);
    ok(ret, "BitBlt failed to blank the bitmap!\n");

    /* Try inverted rectangle coordinates */
    ret = Rectangle(hdc, 0, 2, 2, 0);
    ok(ret, "Rectangle failed!");
    color = GetPixel(hdc, 0, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 2);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 0, 2);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 1);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);

    ret = BitBlt(hdc, 0, 0, 4, 4, NULL, 0, 0, WHITENESS);
    ok(ret, "BitBlt failed to blank the bitmap!\n");
    /* Try well ordered rectangle coordinates */
    ret = Rectangle(hdc, 0, 0, 2, 2);
    ok(ret, "Rectangle failed!");
    color = GetPixel(hdc, 0, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 2);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 0, 2);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 2, 0);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1, 1);
    ok( color == RGB(0, 0, 0), "Expected 0, got 0x%08x\n", (UINT)color);


    hBmp = SelectObject(hdc, hBmp);
    hBrush = SelectObject(hdc, hBrush);
    DeleteObject(hBmp);
    DeleteObject(hBrush);
    DeleteDC(hdc);
}


START_TEST(Rectangle)
{
    Test_Rectangle();
}
