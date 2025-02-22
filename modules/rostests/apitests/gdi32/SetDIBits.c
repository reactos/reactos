/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetDIBits
 * PROGRAMMERS:     Jérôme Gardou
 */

#include "precomp.h"

void Test_SetDIBits()
{
    char buffer[sizeof(BITMAPINFOHEADER)+2*sizeof(RGBQUAD)];
    ULONG* dibBuffer;
    BITMAPINFO* pBMI = (BITMAPINFO*)buffer;
    char bits1bpp[] = {0x80, 0, 0, 0};
    HBITMAP hbmp;
    int ret;

    ZeroMemory(buffer, sizeof(buffer));

    pBMI->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    pBMI->bmiHeader.biWidth=2;
    pBMI->bmiHeader.biHeight=1;
    pBMI->bmiHeader.biPlanes=1;
    pBMI->bmiHeader.biBitCount=32;
    pBMI->bmiHeader.biCompression=BI_RGB;
    pBMI->bmiHeader.biSizeImage=0;
    pBMI->bmiHeader.biXPelsPerMeter=0;
    pBMI->bmiHeader.biYPelsPerMeter=0;
    pBMI->bmiHeader.biClrUsed=0;
    pBMI->bmiHeader.biClrImportant=0;

    hbmp = CreateDIBSection(NULL, pBMI, DIB_RGB_COLORS, (PVOID*)&dibBuffer, NULL, 0);
    ok(hbmp!=NULL, "Failed to create a DIB section\n");

    pBMI->bmiHeader.biBitCount = 1;
    pBMI->bmiColors[0].rgbBlue = 0xFF;
    pBMI->bmiColors[0].rgbGreen = 0;
    pBMI->bmiColors[0].rgbRed = 0xFF;

    ret = SetDIBits(NULL, hbmp, 0, 1, bits1bpp, pBMI, DIB_RGB_COLORS);
    ok(ret == 1, "Copied %i scanlines\n", ret);

    ok(dibBuffer[0] == 0, "Wrong color 0x%08x after SetDIBits\n", (unsigned int)dibBuffer[0]);
    ok(dibBuffer[1] == 0xFF00FF, "Wrong color 0x%08x after SetDIBits\n", (unsigned int)dibBuffer[1]);

    DeleteObject(hbmp);
}

void Test_SetDIBits_1bpp()
{
    char buffer[sizeof(BITMAPINFOHEADER)+2*sizeof(RGBQUAD)];
    HDC hdc;
    BITMAPINFO* pBMI = (BITMAPINFO*)buffer;
    char bits1bpp[] = {0x80, 0, 0, 0};
    HBITMAP hbmp;
    int ret;
    COLORREF color;

    hdc = CreateCompatibleDC(0);
    if(!hdc)
    {
        trace("No device contexr !?\n");
        return;
    }

    ZeroMemory(buffer, sizeof(buffer));

    pBMI->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    pBMI->bmiHeader.biWidth=2;
    pBMI->bmiHeader.biHeight=1;
    pBMI->bmiHeader.biPlanes=1;
    pBMI->bmiHeader.biBitCount=1;
    pBMI->bmiHeader.biCompression=BI_RGB;
    pBMI->bmiHeader.biSizeImage=0;
    pBMI->bmiHeader.biXPelsPerMeter=0;
    pBMI->bmiHeader.biYPelsPerMeter=0;
    pBMI->bmiHeader.biClrUsed=2;
    pBMI->bmiHeader.biClrImportant=0;
    pBMI->bmiColors[0].rgbBlue = 0xFF;
    pBMI->bmiColors[0].rgbGreen = 0xFF;
    pBMI->bmiColors[0].rgbRed = 0xFF;

    hbmp = CreateBitmap(2, 1, 1, 1, NULL);
    ok(hbmp!=NULL, "Failed to create a monochrome bitmap\n");

    ret = SetDIBits(NULL, hbmp, 0, 1, bits1bpp, pBMI, DIB_RGB_COLORS);
    ok(ret == 1, "Copied %i scanlines\n", ret);

    hbmp = SelectObject(hdc, hbmp);
    ok(hbmp != NULL, "Could not select the bitmap into the context.\n");
    color = GetPixel(hdc, 0,0);
    ok(color == 0, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1,0);
    ok(color == 0xFFFFFF, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    hbmp = SelectObject(hdc, hbmp);

    /* Try something else than 0xFFFFFF */
    pBMI->bmiColors[0].rgbBlue = 0xFF;
    pBMI->bmiColors[0].rgbGreen = 0;
    pBMI->bmiColors[0].rgbRed = 0;

    ret = SetDIBits(NULL, hbmp, 0, 1, bits1bpp, pBMI, DIB_RGB_COLORS);
    ok(ret == 1, "Copied %i scanlines\n", ret);

    hbmp = SelectObject(hdc, hbmp);
    ok(hbmp != NULL, "Could not select the bitmap into the context.\n");
    color = GetPixel(hdc, 0,0);
    ok(color == 0, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1,0);
    ok(color == 0xFFFFFF, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    hbmp = SelectObject(hdc, hbmp);

    /* Special : try 0 */
    pBMI->bmiColors[0].rgbBlue = 0;
    pBMI->bmiColors[0].rgbGreen = 0;
    pBMI->bmiColors[0].rgbRed = 0;

    ret = SetDIBits(NULL, hbmp, 0, 1, bits1bpp, pBMI, DIB_RGB_COLORS);
    ok(ret == 1, "Copied %i scanlines\n", ret);

    hbmp = SelectObject(hdc, hbmp);
    ok(hbmp != NULL, "Could not select the bitmap into the context.\n");
    color = GetPixel(hdc, 0,0);
    ok(color == 0, "Wrong color at 0,0 : 0x%08x\n", (UINT)color);
    color = GetPixel(hdc, 1,0);
    ok(color == 0xFFFFFF, "Wrong color at 1,0 : 0x%08x\n", (UINT)color);

    hbmp = SelectObject(hdc, hbmp);
    DeleteObject(hbmp);
    DeleteDC(hdc);
}

START_TEST(SetDIBits)
{
    Test_SetDIBits();
    Test_SetDIBits_1bpp();
}
