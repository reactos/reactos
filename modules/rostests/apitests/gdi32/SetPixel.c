/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetPixel
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include <mmsystem.h>

static struct
{
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY logpalettedata[8];
} gpal =
{
    0x300, 8,
    {
        { 0x10, 0x20, 0x30, PC_NOCOLLAPSE },
        { 0x20, 0x30, 0x40, PC_NOCOLLAPSE },
        { 0x30, 0x40, 0x50, PC_NOCOLLAPSE },
        { 0x40, 0x50, 0x60, PC_NOCOLLAPSE },
        { 0x50, 0x60, 0x70, PC_NOCOLLAPSE },
        { 0x60, 0x70, 0x80, PC_NOCOLLAPSE },
        { 0x70, 0x80, 0x90, PC_NOCOLLAPSE },
        { 0x80, 0x90, 0xA0, PC_NOCOLLAPSE },
    }
};

void Test_SetPixel_Params()
{
    HDC hdc;

    SetLastError(0);
    ok_long(SetPixel(0, 0, 0, RGB(255,255,255)), -1);
    ok_err(ERROR_INVALID_HANDLE);

    /* Test an info DC */
    hdc = CreateICA("DISPLAY", NULL, NULL, NULL);
    ok(hdc != 0, "\n");
    SetLastError(0);
    ok_long(SetPixel(hdc, 0, 0, 0), -1);
    ok_long(SetPixel(hdc, 0, 0, RGB(255,255,255)), -1);
    ok_err(0);
    DeleteDC(hdc);

    /* Test a mem DC without selecting a bitmap */
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != 0, "\n");
    SetLastError(0);
    ok_long(SetPixel(hdc, 0, 0, 0), -1);
    ok_err(0);
    DeleteDC(hdc);

    /* Test deleted DC */
    ok_long(SetPixel(hdc, 0, 0, 0), -1);

}

void Test_SetPixel_PAL()
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD bmiColors[8];
    } bmibuffer;
    BITMAPINFO *pbmi = (PVOID)&bmibuffer;
    HBITMAP hbmp;
    HDC hdc;
    HPALETTE hpal, hpalOld;
    PULONG pulBits;
    USHORT i;

    /* Initialize the BITMAPINFO */
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 1;
    pbmi->bmiHeader.biYPelsPerMeter = 1;
    pbmi->bmiHeader.biClrUsed = 8;
    pbmi->bmiHeader.biClrImportant = 0;
    for( i = 0; i < 8; i++ )
    {
        bmibuffer.bmiColors[i] = i + 1;
    }

    /* Create a memory DC */
    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "failed\n");

    /* Create a DIB section and select it */
    hbmp = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (PVOID*)&pulBits, 0, 0 );
    ok(hbmp != NULL, "CreateDIBSection failed with error %ld\n", GetLastError());
    ok(SelectObject(hdc, hbmp) != 0, "SelectObject failed\n");

    ok_long(SetPixel(hdc, 0, 0, 0), 0);
    ok_long(pulBits[0], 8);
    ok_long(SetPixel(hdc, 0, 0, 1), 0);
    ok_long(pulBits[0], 8);
    ok_long(SetPixel(hdc, 0, 0, RGB(255,255,255)), 0xc0dcc0);
    ok_long(pulBits[0], 7);

    ok_long(SetPixel(hdc, 0, 0, RGB(255,0,0)), 0x80);
    ok_long(pulBits[0], 0);

    /* Test DIBINDEX */
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(0)), 0x80);
    ok_long(pulBits[0], 0);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(1)), 0x8000);
    ok_long(pulBits[0], 1);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(7)), 0xc0dcc0);
    ok_long(pulBits[0], 7);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(8)), 0);
    ok_long(pulBits[0], 8);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(126)), 0);
    ok_long(pulBits[0], 126);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(0x123456)), 0);
    ok_long(pulBits[0], 0x56);

    /* Test PALETTEINDEX */
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(0)), 0);
    ok_long(pulBits[0], 8);
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(1)), 0x80);
    ok_long(pulBits[0], 0);
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(2)), 0x8000);
    ok_long(pulBits[0], 1);

    /* Delete the DIB section */
    DeleteObject(hbmp);


    /* Initialize the logical palette and select it */
    hpal = CreatePalette((LOGPALETTE*)&gpal);
    hpalOld = SelectPalette(hdc, hpal, FALSE);
    ok(hpalOld != NULL, "error=%ld\n", GetLastError());


    /* Create a DIB section and select it */
    hbmp = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (PVOID*)&pulBits, 0, 0 );
    ok(hbmp != NULL, "CreateDIBSection failed with error %ld\n", GetLastError());
    ok(SelectObject(hdc, hbmp) != 0, "SelectObject failed\n");

    ok_long(SetPixel(hdc, 0, 0, 0), 0);
    ok_long(pulBits[0], 8);

    ok_long(SetPixel(hdc, 0, 0, RGB(255,0,0)), 0x605040);
    ok_long(pulBits[0], 2);

    /* Test DIBINDEX */
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(0)), 0x403020);
    ok_long(pulBits[0], 0);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(1)), 0x504030);
    ok_long(pulBits[0], 1);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(7)), 0x302010);
    ok_long(pulBits[0], 7);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(8)), 0);
    ok_long(pulBits[0], 8);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(126)), 0);
    ok_long(pulBits[0], 126);
    ok_long(SetPixel(hdc, 0, 0, DIBINDEX(0x123456)), 0);
    ok_long(pulBits[0], 0x56);

    /* Test PALETTEINDEX */
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(0)), 0x302010);
    ok_long(pulBits[0], 7);
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(1)), 0x403020);
    ok_long(pulBits[0], 0);
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(253)), 0x302010);
    ok_long(pulBits[0], 7);
    ok_long(SetPixel(hdc, 0, 0, PALETTEINDEX(254)), 0x302010);
    ok_long(pulBits[0], 7);


}

START_TEST(SetPixel)
{
    Test_SetPixel_Params();
    Test_SetPixel_PAL();
}

