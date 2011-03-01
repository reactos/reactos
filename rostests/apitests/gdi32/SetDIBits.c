/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetDIBits
 * PROGRAMMERS:     Jérôme Gardou
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>


void Test_SetDIBits()
{
    char buffer[sizeof(BITMAPINFOHEADER)+2*sizeof(RGBQUAD)];
    ULONG* dibBuffer;
    BITMAPINFO* pBMI = (BITMAPINFO*)buffer;
    DWORD bits1bpp[2] = {0, 1};
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
    pBMI->bmiColors[0].rgbGreen = 0xFF;
    pBMI->bmiColors[0].rgbRed = 0xFF;

    ret = SetDIBits(NULL, hbmp, 0, 1, bits1bpp, pBMI, DIB_RGB_COLORS);
    ok(ret == 1, "Copied %i scanlines\n", ret);

    ok(dibBuffer[0] = 0xFFFFFF, "Wrong color 0x%08x after SetDIBits\n", (unsigned int)dibBuffer[0]);
    ok(dibBuffer[1] = 0xFFFFFF, "Wrong color 0x%08x after SetDIBits\n", (unsigned int)dibBuffer[1]);

    DeleteObject(hbmp);
}

START_TEST(SetDIBits)
{
    Test_SetDIBits();
}
