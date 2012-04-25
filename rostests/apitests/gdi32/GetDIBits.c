/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetDIBits
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void Test_GetDIBits()
{
	HDC hdcScreen, hdcMem;
	HBITMAP hbmp;
	PBITMAPINFO pbi;
	INT ret, ScreenBpp;
	DWORD ajBits[10] = {0xff, 0x00, 0xcc, 0xf0, 0x0f};

    pbi = malloc(sizeof(BITMAPV5HEADER) + 256 * sizeof(DWORD));

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
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ok(GetDIBits(NULL, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "\n");
	ok_err(ERROR_INVALID_PARAMETER);

	/* null bitmap */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ok(GetDIBits(hdcScreen, NULL, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "\n");
	ok_err(ERROR_SUCCESS);

	/* 0 scan lines */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ok(GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS) > 0, "\n");
	ok_err(ERROR_SUCCESS);

	/* null bitmap info - crashes XP*/
	//SetLastError(ERROR_SUCCESS);
	//ok(GetDIBits(hdcScreen, NULL, 0, 15, NULL, NULL, DIB_RGB_COLORS) == 0);
	//ok(GetLastError() == ERROR_INVALID_PARAMETER);

	/* bad bmi colours (uUsage) */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, 100) == 0, "\n");
	ok_err(ERROR_SUCCESS);
	ok(pbi->bmiHeader.biWidth == 0, "\n");
	ok(pbi->bmiHeader.biHeight == 0, "\n");
	ok(pbi->bmiHeader.biBitCount == 0, "\n");
	ok(pbi->bmiHeader.biSizeImage == 0, "\n");

	/* basic call */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) > 0, "\n");
	ok_err(ERROR_SUCCESS);
	ScreenBpp = GetDeviceCaps(hdcScreen, BITSPIXEL);
	ok(pbi->bmiHeader.biWidth == 16, "\n");
	ok(pbi->bmiHeader.biHeight == 16, "\n");
	ok(pbi->bmiHeader.biBitCount == ScreenBpp, "\n");
	ok(pbi->bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8), "\n");

    /* Test if COREHEADER is supported */
	pbi->bmiHeader.biSize = sizeof(BITMAPCOREHEADER);
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) > 0, "\n");
	ok(pbi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER), "\n");

    /* Test different header sizes */
	pbi->bmiHeader.biSize = sizeof(BITMAPCOREHEADER) + 4;
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "should fail.\n");
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) + 4;
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) == 0, "should fail.\n");
	pbi->bmiHeader.biSize = sizeof(BITMAPV5HEADER);
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) > 0, "should not fail.\n");
	pbi->bmiHeader.biSize = sizeof(BITMAPV5HEADER) + 4;
	ok(GetDIBits(hdcScreen, hbmp, 0, 15, NULL, pbi, DIB_RGB_COLORS) > 0, "should not fail.\n");


	DeleteObject(hbmp);

    /* Test a mono bitmap */
    hbmp = CreateBitmap(13, 7, 1, 1, ajBits);
    ok(hbmp != 0, "failed to create bitmap\n");
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ret = GetDIBits(hdcScreen, hbmp, 0, 0, NULL, pbi, DIB_RGB_COLORS);
	ok(ret == 1, "%d\n", ret);
	ok(pbi->bmiHeader.biWidth == 13, "pbi->bmiHeader.biWidth = %ld\n", pbi->bmiHeader.biWidth);
	ok(pbi->bmiHeader.biHeight == 7, "pbi->bmiHeader.biHeight = %ld\n", pbi->bmiHeader.biHeight);
	ok(pbi->bmiHeader.biBitCount == 1, "pbi->bmiHeader.biBitCount = %d\n", pbi->bmiHeader.biBitCount);
	ok(pbi->bmiHeader.biSizeImage == 28, "pbi->bmiHeader.biSizeImage = %ld\n", pbi->bmiHeader.biSizeImage);

    /* Test a mono bitmap with values set */
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbi->bmiHeader.biWidth = 12;
	pbi->bmiHeader.biHeight = 9;
	pbi->bmiHeader.biPlanes = 1;
	pbi->bmiHeader.biBitCount = 32;
	pbi->bmiHeader.biCompression = BI_RGB;
	pbi->bmiHeader.biSizeImage = 123;
	ret = GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS);
	ok(ret == 1, "%d\n", ret);
	ok(pbi->bmiHeader.biWidth == 12, "pbi->bmiHeader.biWidth = %ld\n", pbi->bmiHeader.biWidth);
	ok(pbi->bmiHeader.biHeight == 9, "pbi->bmiHeader.biHeight = %ld\n", pbi->bmiHeader.biHeight);
	ok(pbi->bmiHeader.biBitCount == 32, "pbi->bmiHeader.biBitCount = %d\n", pbi->bmiHeader.biBitCount);
	ok(pbi->bmiHeader.biSizeImage == 432, "pbi->bmiHeader.biSizeImage = %ld\n", pbi->bmiHeader.biSizeImage);

    /* Set individual values */
	ZeroMemory(pbi, sizeof(BITMAPINFO));
	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbi->bmiHeader.biWidth = 12;
	ret = GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS);
	ok(ret == 1, "%d\n", ret);
	pbi->bmiHeader.biWidth = 0;
	pbi->bmiHeader.biSizeImage = 123;
	ret = GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS);
	ok(ret == 0, "%d\n", ret);
	pbi->bmiHeader.biSizeImage = 0;
	pbi->bmiHeader.biCompression = BI_RGB;
	ret = GetDIBits(hdcScreen, hbmp, 0, 5, NULL, pbi, DIB_RGB_COLORS);
	ok(ret == 0, "%d\n", ret);

	DeleteObject(hbmp);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

START_TEST(GetDIBits)
{
    Test_GetDIBits();
}

