/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_GetDIBColorTable()
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG bmiColors[8];
    } bmibuffer;
    BITMAPINFO *pbmi = (PVOID)&bmibuffer;
    HBITMAP hbmp, hbmpOld;
    HDC hdc;
    PBYTE pjBits;
    UINT cColors;
    ULONG aulColors[257];

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "failed\n");

    SetLastError(0);
    cColors = GetDIBColorTable(hdc, 0, 257, (RGBQUAD*)aulColors);
    ok_long(cColors, 2);
    ok_err(0);
    ok_long(aulColors[0], 0x000000);
    ok_long(aulColors[1], 0xffffff);

    hbmp = CreateBitmap(1, 1, 1, 1, NULL);
    ok(hbmp != 0, "\n");
    hbmpOld = SelectObject(hdc, hbmp);
    ok(hbmpOld != 0, "Failed to select bitmap\n");
    cColors = GetDIBColorTable(hdc, 0, 257, (RGBQUAD*)aulColors);
    ok_long(cColors, 2);
    ok_err(0);
    ok_long(aulColors[0], 0x000000);
    ok_long(aulColors[1], 0xffffff);
    SelectObject(hdc, hbmpOld);
    DeleteObject(hbmp);

    /* Initialize a BITMAPINFO */
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 2;
    pbmi->bmiHeader.biHeight = 2;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 1;
    pbmi->bmiHeader.biYPelsPerMeter = 1;
    pbmi->bmiHeader.biClrUsed = 3;
    pbmi->bmiHeader.biClrImportant = 0;
    bmibuffer.bmiColors[0] = 0xff0000;
    bmibuffer.bmiColors[1] = 0x00ff00;
    bmibuffer.bmiColors[2] = 0x0000ff;

    hbmp = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (PVOID*)&pjBits, 0, 0 );
    ok( hbmp != NULL, "error=%ld\n", GetLastError() );
    SelectObject(hdc, hbmp);

    cColors = GetDIBColorTable(hdc, 0, 257, (RGBQUAD*)aulColors);
    ok_long(cColors, 256);
    ok_long(aulColors[0], 0xff0000);
    ok_long(aulColors[1], 0x00ff00);
    ok_long(aulColors[2], 0x0000ff);
    ok_long(aulColors[3], 0x000000);


    cColors = SetDIBColorTable(hdc, 0, 4, (RGBQUAD*)aulColors);
    ok_long(cColors, 4);

    aulColors[3] = 0x000F0F;
    cColors = SetDIBColorTable(hdc, 0, 4, (RGBQUAD*)aulColors);
    ok_long(cColors, 4);

    cColors = GetDIBColorTable(hdc, 0, 257, (RGBQUAD*)aulColors);
    ok_long(cColors, 256);
    ok_long(aulColors[0], 0xff0000);
    ok_long(aulColors[1], 0x00ff00);
    ok_long(aulColors[2], 0x0000ff);
    ok_long(aulColors[3], 0x000F0F);


    SelectObject(hdc, GetStockObject(21));
    DeleteObject(hbmp);

    bmibuffer.bmiColors[0] = 1;
    bmibuffer.bmiColors[1] = 2;
    bmibuffer.bmiColors[2] = 3;

    hbmp = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (PVOID*)&pjBits, 0, 0 );
    ok( hbmp != NULL, "error=%ld\n", GetLastError() );
    SelectObject(hdc, hbmp);


}

START_TEST(GetDIBColorTable)
{
    Test_GetDIBColorTable();
}

