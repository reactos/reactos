/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetDIBits
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define TEST(x) ok(x, #x"\n")
#define RTEST(x) ok(x, #x"\n")

void Test_GetDIBits()
{
	HDC hDCScreen;
	HBITMAP hBitmap;
	BITMAPINFO bi;
	INT ScreenBpp;

	hDCScreen = GetDC(NULL);
	ok(hDCScreen != 0, "GetDC failed, skipping tests\n");
	if (hDCScreen == NULL) return;

	hBitmap = CreateCompatibleBitmap(hDCScreen, 16, 16);
	RTEST(hBitmap != NULL);

	/* misc */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits(0, 0, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, 0, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, hBitmap, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, hBitmap, 0, 15, NULL, &bi, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);



	/* null hdc */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(NULL, hBitmap, 0, 15, NULL, &bi, DIB_RGB_COLORS) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* null bitmap */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(hDCScreen, NULL, 0, 15, NULL, &bi, DIB_RGB_COLORS) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* 0 scan lines */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(hDCScreen, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* null bitmap info - crashes XP*/
	//SetLastError(ERROR_SUCCESS);
	//RTEST(GetDIBits(hDCScreen, NULL, 0, 15, NULL, NULL, DIB_RGB_COLORS) == 0);
	//RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* bad bmi colours (uUsage) */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(hDCScreen, hBitmap, 0, 15, NULL, &bi, 100) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);
	RTEST(bi.bmiHeader.biWidth == 0);
	RTEST(bi.bmiHeader.biHeight == 0);
	RTEST(bi.bmiHeader.biBitCount == 0);
	RTEST(bi.bmiHeader.biSizeImage == 0);

	/* basic call */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(hDCScreen, hBitmap, 0, 15, NULL, &bi, DIB_RGB_COLORS) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);
	ScreenBpp = GetDeviceCaps(hDCScreen, BITSPIXEL);
	RTEST(bi.bmiHeader.biWidth == 16);
	RTEST(bi.bmiHeader.biHeight == 16);
	RTEST(bi.bmiHeader.biBitCount == ScreenBpp);
	RTEST(bi.bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8));

	DeleteObject(hBitmap);
	ReleaseDC(NULL, hDCScreen);
}

START_TEST(GetDIBits)
{
    Test_GetDIBits();
}

