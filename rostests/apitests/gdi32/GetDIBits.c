/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetDIBits
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

#include "init.h"

void
Test_GetDIBits_xBpp(
    ULONG cBitsPixel)
{
    UCHAR ajBuffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    PBITMAPINFO pbmi = (PBITMAPINFO)ajBuffer;
    HBITMAP hbmp;
    ULONG cjSizeImage, cColors;
    HDC hdc;

    hdc = GetDC(0);
    hbmp = CreateBitmap(3, 3, 1, cBitsPixel, NULL);

    /* Fill in the size field */
    ZeroMemory(pbmi, sizeof(ajBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    /* Get info */
    ok_int(GetDIBits(hdc, hbmp, 0, 0, NULL, pbmi, DIB_RGB_COLORS), 1);
    cjSizeImage = (((pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount) + 31) & ~31) * pbmi->bmiHeader.biHeight / 8;
    cColors = cBitsPixel <= 8 ? 1 << pbmi->bmiHeader.biBitCount : 0;

    ok_int(pbmi->bmiHeader.biSize, sizeof(BITMAPINFOHEADER));
    ok_int(pbmi->bmiHeader.biWidth, 3);
    ok_int(pbmi->bmiHeader.biHeight, 3);
    ok_int(pbmi->bmiHeader.biPlanes, 1);
    ok_int(pbmi->bmiHeader.biBitCount, cBitsPixel);
    ok_int(pbmi->bmiHeader.biCompression, (cBitsPixel == 16) || (cBitsPixel == 32) ? BI_BITFIELDS : BI_RGB);
    ok_int(pbmi->bmiHeader.biSizeImage, cjSizeImage);
    ok_int(pbmi->bmiHeader.biXPelsPerMeter, 0);
    ok_int(pbmi->bmiHeader.biYPelsPerMeter, 0);
    ok_int(pbmi->bmiHeader.biClrUsed, cColors);
    ok_int(pbmi->bmiHeader.biClrImportant, cColors);

//    pbmi->bmiHeader.biSizeImage = 0;
   //ok_int(GetDIBits(NULL, hbmp, 0, 0, NULL, pbmi, DIB_RGB_COLORS), 1);

    /* Test a bitmap with values partly set */
    ZeroMemory(pbmi, sizeof(ajBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 12;
    pbmi->bmiHeader.biHeight = 14;
    pbmi->bmiHeader.biPlanes = 2;
    pbmi->bmiHeader.biBitCount = 0; // keep biBitCount == 0!
    pbmi->bmiHeader.biCompression = 7;
    pbmi->bmiHeader.biSizeImage = 123;
    ok_int(GetDIBits(hdc, hbmp, 0, 5, NULL, pbmi, DIB_RGB_COLORS), 1);
    ok_int(pbmi->bmiHeader.biWidth, 3);
    ok_int(pbmi->bmiHeader.biHeight, 3);
    ok_int(pbmi->bmiHeader.biPlanes, 1);
    ok_int(pbmi->bmiHeader.biBitCount, cBitsPixel);
    ok_int(pbmi->bmiHeader.biSizeImage, cjSizeImage);
    ok_int(pbmi->bmiHeader.biXPelsPerMeter, 0);
    ok_int(pbmi->bmiHeader.biYPelsPerMeter, 0);
    ok_int(pbmi->bmiHeader.biClrUsed, cColors);
    ok_int(pbmi->bmiHeader.biClrImportant, cColors);


#if 0
    /* Get info including the color table */
    ZeroMemory(pbmi, sizeof(ajBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = 1;
    ok_int(GetDIBits(NULL, hbmp, 0, 0, NULL, pbmi, DIB_PAL_COLORS), 1);

    /* Check a different bit depth */
    pbmi->bmiHeader.biBitCount, cBitsPixel = (cBitsPixel == 1) ? 8 : 1;
    ok_int(GetDIBits(hdc, hbmp, 0, 0, NULL, pbmi, DIB_RGB_COLORS), 1);
    ok_int(pbmi->bmiHeader.biBitCount, (cBitsPixel == 1) ? 8 : 1);

    /* Get the bits */
    SetLastError(0);
    ok_int(GetDIBits(hdc, hbmp, 0, 4, pvBits, pbmi, DIB_PAL_COLORS), 3);
    ok_int(GetDIBits(hdc, hbmp, 3, 7, pvBits, pbmi, DIB_RGB_COLORS), 1);
    ok_err(0);

    pbmi->bmiHeader.biPlanes = 2;
    pbmi->bmiHeader.biClrUsed = 0;
    ok_int(GetDIBits(hdc, hbmp, 0, 4, pvBits, pbmi, DIB_RGB_COLORS), 3);
#endif

    DeleteObject(hbmp);
    ReleaseDC(NULL, hdc);
}

void Test_GetDIBits()
{
    HDC hdcScreen, hdcMem;
    HBITMAP hbmp;
    PBITMAPINFO pbi;
    DWORD bisize;
    PBITMAPCOREHEADER pbch;
    PBITMAPV5HEADER pbV5Header;
    INT ScreenBpp;
    DWORD ajBits[100] = {0xff, 0x00, 0xcc, 0xf0, 0x0f};
    PVOID pvBits;
    ULONG cjSizeImage;

    bisize = sizeof(BITMAPV5HEADER) + 256 * sizeof(DWORD);
    pbi = malloc(bisize);
    pbch = (PVOID)pbi;
    pbV5Header = (PVOID)pbi;

    hdcScreen = GetDC(NULL);
    ok(hdcScreen != 0, "GetDC failed, skipping tests\n");
    if (hdcScreen == NULL) return;

    hdcMem = CreateCompatibleDC(0);
    ok(hdcMem != 0, "CreateCompatibleDC failed, skipping tests\n");
    if (hdcMem == NULL) return;

    hbmp = CreateCompatibleBitmap(hdcScreen, 16, 16);
    ok(hbmp != NULL, "CreateCompatibleBitmap failed\n");

    /* misc */
    SetLastError(ERROR_SUCCESS);
    ok(GetDIBits(0, 0, 0, 0, NULL, NULL, 0) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok(GetDIBits((HDC)2345, 0, 0, 0, NULL, NULL, 0) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok(GetDIBits((HDC)2345, hbmp, 0, 0, NULL, NULL, 0) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok(GetDIBits((HDC)2345, hbmp, 0, 15, NULL, pbi, 0) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);



    /* null hdc */
    SetLastError(ERROR_SUCCESS);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok(GetDIBits(NULL, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* null bitmap */
    SetLastError(ERROR_SUCCESS);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok(GetDIBits(hdcScreen, NULL, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "\n");
    ok_err(ERROR_SUCCESS);

    /* 0 scan lines */
    SetLastError(ERROR_SUCCESS);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS) > 0, "\n");
    ok_err(ERROR_SUCCESS);

    /* null bitmap info - crashes XP*/
    //SetLastError(ERROR_SUCCESS);
    //ok(GetDIBits(hdcScreen, NULL, 0, 15, NULL, NULL, DIB_RGB_COLORS) == 0);
    //ok(GetLastError() == ERROR_INVALID_PARAMETER);

    /* bad bmi colours (uUsage) */
    SetLastError(0);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, 100) == 0, "\n");
    ok_err(ERROR_SUCCESS);
    ok(pbi->bmiHeader.biWidth == 0, "\n");
    ok(pbi->bmiHeader.biHeight == 0, "\n");
    ok(pbi->bmiHeader.biBitCount == 0, "\n");
    ok(pbi->bmiHeader.biSizeImage == 0, "\n");

    /* basic call */
    SetLastError(ERROR_SUCCESS);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) > 0, "\n");
    ok_err(ERROR_SUCCESS);
    ScreenBpp = GetDeviceCaps(hdcScreen, BITSPIXEL);
    ok(pbi->bmiHeader.biWidth == 16, "\n");
    ok(pbi->bmiHeader.biHeight == 16, "\n");
    ok(pbi->bmiHeader.biBitCount == ScreenBpp, "\n");
    ok(pbi->bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8), "\n");

    /* Test if COREHEADER is supported */
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPCOREHEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biSize, sizeof(BITMAPCOREHEADER));
    ok_int(pbch->bcWidth, 16);
    ok_int(pbch->bcHeight, 16);
    ok_int(pbch->bcPlanes, 1);
    //ok_int(pbch->bcBitCount, ScreenBpp > 16 ? 24 : ScreenBpp); // fails on XP with screenbpp == 16

    /* Test different header sizes */
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPCOREHEADER) + 4;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 0);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) + 4;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPV4HEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPV4HEADER) + 4;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPV5HEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPV5HEADER) + 4;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbV5Header->bV5RedMask, 0);
    ok_int(pbV5Header->bV5GreenMask, 0);
    ok_int(pbV5Header->bV5BlueMask, 0);
    ok_int(pbV5Header->bV5AlphaMask, 0);
    ok_int(pbV5Header->bV5CSType, 0);
    // CIEXYZTRIPLE bV5Endpoints;
    ok_int(pbV5Header->bV5GammaRed, 0);
    ok_int(pbV5Header->bV5GammaGreen, 0);
    ok_int(pbV5Header->bV5GammaBlue, 0);
    ok_int(pbV5Header->bV5Intent, 0);
    ok_int(pbV5Header->bV5ProfileData, 0);
    ok_int(pbV5Header->bV5ProfileSize, 0);
    ok_int(pbV5Header->bV5Reserved, 0);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 234, 43, NULL, pbi, DIB_RGB_COLORS), 1);

    DeleteObject(hbmp);

    /* Test a mono bitmap */
    hbmp = CreateBitmap(13, 7, 1, 1, ajBits);
    ok(hbmp != 0, "failed to create bitmap\n");
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biWidth, 13);
    ok_int(pbi->bmiHeader.biHeight, 7);
    ok_int(pbi->bmiHeader.biBitCount, 1);
    ok_int(pbi->bmiHeader.biSizeImage, 28);

    /* Test with values set, except biSizeImage */
    pbi->bmiHeader.biSizeImage = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biSizeImage, 28);

    /* Test with different biWidth set */
    pbi->bmiHeader.biWidth = 17;
    pbi->bmiHeader.biHeight = 3;
    pbi->bmiHeader.biSizeImage = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biSizeImage, 12);
    ok_int(pbi->bmiHeader.biWidth, 17);
    ok_int(pbi->bmiHeader.biHeight, 3);

    /* Test with different biBitCount set */
    pbi->bmiHeader.biBitCount = 4;
    pbi->bmiHeader.biSizeImage = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biSizeImage, 36);
    ok_int(pbi->bmiHeader.biBitCount, 4);

    /* Set bitmap dimensions */
    ok_int(SetBitmapDimensionEx(hbmp, 110, 220, NULL), 1);
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biXPelsPerMeter, 0);
    ok_int(pbi->bmiHeader.biYPelsPerMeter, 0);

    /* Set individual values */
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biWidth = 12;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biWidth, 13);
    ok_int(pbi->bmiHeader.biSizeImage, 28);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biSizeImage = 123;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);

    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biCompression = BI_RGB;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);

    /* Set only biBitCount */
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biBitCount = 5;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biBitCount = 1;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biBitCount = 8;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biBitCount = 32;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    ok_int(pbi->bmiHeader.biWidth, 0);
    ok_int(pbi->bmiHeader.biHeight, 0);
    ok_int(pbi->bmiHeader.biPlanes, 0);
    ok_int(pbi->bmiHeader.biBitCount, 32);
    ok_int(pbi->bmiHeader.biCompression, 0);
    ok_int(pbi->bmiHeader.biSizeImage, 0);

    /* Get the bitmap bits */
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biWidth = 4;
    pbi->bmiHeader.biHeight = 4;
    pbi->bmiHeader.biPlanes = 1;
    pbi->bmiHeader.biBitCount = 32;
    pbi->bmiHeader.biCompression = BI_RGB;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);
    pbi->bmiHeader.biWidth = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biWidth = -3;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biWidth = 4;
    pbi->bmiHeader.biHeight = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biHeight = -4;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);
    pbi->bmiHeader.biBitCount = 31;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biBitCount = 16;
    pbi->bmiHeader.biPlanes = 23;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biPlanes, 1);
    pbi->bmiHeader.biCompression = BI_JPEG;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);
    pbi->bmiHeader.biCompression = BI_PNG;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS), 0);


    /* Get the bitmap bits */
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biWidth = 4;
    pbi->bmiHeader.biHeight = 4;
    pbi->bmiHeader.biPlanes = 1;
    pbi->bmiHeader.biBitCount = 32;
    pbi->bmiHeader.biCompression = BI_RGB;
    pbi->bmiHeader.biSizeImage = 0;
    pbi->bmiHeader.biXPelsPerMeter = 0;
    pbi->bmiHeader.biYPelsPerMeter = 0;
    pbi->bmiHeader.biClrUsed = 0;
    pbi->bmiHeader.biClrImportant = 0;
    cjSizeImage = ((pbi->bmiHeader.biWidth * pbi->bmiHeader.biBitCount + 31) / 32) * 4 * pbi->bmiHeader.biHeight;
    pvBits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 4);

    /* Set biBitCount to 0 */
    pbi->bmiHeader.biBitCount = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 0);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(GetDIBits(NULL, hbmp, 0, 4, NULL, pbi, DIB_RGB_COLORS), 0);

    /* Try different biBitCount and biWidth */
    pbi->bmiHeader.biBitCount = 24;
    pbi->bmiHeader.biWidth = 3;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 4);

    /* Try different biBitCount and biWidth */
    pbi->bmiHeader.biBitCount = 24;
    pbi->bmiHeader.biWidth = 3;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 4);

    /* Set only biBitCount and pjInit */
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbi->bmiHeader.biBitCount = 5;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 0);

    DeleteObject(hbmp);
    HeapFree(GetProcessHeap(), 0, pvBits);

    /* Test a 4 bpp bitmap */
    hbmp = CreateBitmap(3, 5, 1, 4, NULL);
    ok(hbmp != 0, "failed to create bitmap\n");
    ZeroMemory(pbi, bisize);
    pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 1);
    ok_int(pbi->bmiHeader.biWidth, 3);
    ok_int(pbi->bmiHeader.biHeight, 5);
    ok_int(pbi->bmiHeader.biBitCount, 4);
    ok_int(pbi->bmiHeader.biSizeImage, 20);

    /* This does NOT work with incompatible bitmaps */
    pbi->bmiHeader.biSizeImage = 0;
    ok_int(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS), 0);

    cjSizeImage = ((pbi->bmiHeader.biWidth * pbi->bmiHeader.biBitCount + 31) / 32) * 4 * pbi->bmiHeader.biHeight;
    pvBits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cjSizeImage);

    ok(SelectObject(hdcMem, ghbmpDIB4) != 0, "\n");;
    ok_int(GetDIBits(hdcMem, hbmp, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 0);
    ok_int(GetDIBits(hdcMem, ghbmpDIB4, 0, 4, pvBits, pbi, DIB_RGB_COLORS), 3);


    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void Test_GetDIBits_BI_BITFIELDS()
{
}

START_TEST(GetDIBits)
{
    //getchar();
    InitStuff();
    Test_GetDIBits_xBpp(1);
    Test_GetDIBits_xBpp(4);
    Test_GetDIBits_xBpp(8);
    Test_GetDIBits_xBpp(16);
    Test_GetDIBits_xBpp(24);
    Test_GetDIBits_xBpp(32);
    Test_GetDIBits();
}

