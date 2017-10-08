/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserSelectPalette
 * PROGRAMMERS:
 */

#include <win32nt.h>

FORCEINLINE
PALETTEENTRY
PALENTRY(BYTE r, BYTE g, BYTE b)
{
	PALETTEENTRY ret;

	ret.peRed = r;
	ret.peGreen = g;
	ret.peBlue = b;
	ret.peFlags = 0;
	return ret;
}

START_TEST(NtUserSelectPalette)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
	HPALETTE hPal, hOldPal;
	HWND hWnd;
	HDC hDC, hCompDC;
	struct
	{
		LOGPALETTE logpal;
		PALETTEENTRY entry[20];
	} pal;

	ZeroMemory(&pal, sizeof(pal));

	pal.logpal.palVersion = 0x300;
	pal.logpal.palNumEntries = 6;
	pal.entry[0] = PALENTRY(0,0,0);
	pal.entry[1] = PALENTRY(255,255,255);
	pal.entry[2] = PALENTRY(128,128,128);
	pal.entry[3] = PALENTRY(128,0,0);
	pal.entry[4] = PALENTRY(0,128,0);
	pal.entry[5] = PALENTRY(0,0,128);

	hPal = CreatePalette(&pal.logpal);
	ASSERT(hPal);
	TEST(DeleteObject(hPal) == 1);
	hPal = CreatePalette(&pal.logpal);
	ASSERT(hPal);

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, hinst, 0);
	hDC = GetDC(hWnd);
	ASSERT(hDC);
	hCompDC = CreateCompatibleDC(hDC);
	ASSERT(hCompDC);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(NULL, hPal, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette((HDC)-1, hPal, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL palette */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(hDC, NULL, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid palette */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(hDC, (HPALETTE)-1, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test valid palette */
	hOldPal = NtUserSelectPalette(hDC, hPal, 0);
	TEST(hOldPal != 0);
	TEST(hOldPal == GetStockObject(DEFAULT_PALETTE));

	/* We cannot Delete the palette */
	TEST(DeleteObject(hPal) == 0);

	/* We can still select the Palette into a compatible DC */
	hOldPal = NtUserSelectPalette(hCompDC, hPal, 0);
	TEST(hOldPal != 0);



#if 0
	RealizePalette(hDC);

	GetClientRect(hWnd, &rect);
	FillRect(hDC, &rect, GetSysColorBrush(COLOR_BTNSHADOW));

	TEST(GetNearestColor(hDC, RGB(0,0,0)) == RGB(0,0,0));
	TEST(GetNearestColor(hDC, RGB(0,0,1)) == RGB(0,0,1));

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
	RECT rect;
	HBITMAP hBmp;


	BITMAPINFOHEADER bmih = {sizeof(BITMAPINFOHEADER), // biSize
	                         3, // biWidth
	                         3, // biHeight
	                         1, // biPlanes
	                         8, // biBitCount
	                         BI_RGB, // biCompression
	                         0, // biSizeImage
	                         92, // biXPelsPerMeter
	                         92, // biYPelsPerMeter
	                         6,  // biClrUsed
	                         6}; // biClrImportant
	BYTE bits[3][3] = {{0,1,2},{3,4,5},{6,1,2}};

	struct
	{
		BITMAPINFOHEADER bmih;
		RGBQUAD colors[6];
	} bmi = {{sizeof(BITMAPINFOHEADER),3,3,1,8,BI_RGB,0,92,92,6,6},
	                  {{0,0,0,0},{255,255,255,0},{255,0,0,0},
	                   {0,255,0,0},{0,0,255,0},{128,128,128,0}}};

	hBmp = CreateDIBitmap(hCompDC, &bmih, CBM_INIT, &bits, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
	ASSERT(hBmp);

	SetLastError(0);
	TEST(NtGdiSelectBitmap(hCompDC, hBmp));
	hOldPal = NtUserSelectPalette(hCompDC, hPal, 0);
	TEST(hOldPal != NULL);
	RealizePalette(hCompDC);

	TEST(GetNearestColor(hCompDC, RGB(0,0,0)) == RGB(0,0,0));
	TEST(GetNearestColor(hCompDC, RGB(0,0,1)) == RGB(0,0,0));
	TEST(GetNearestColor(hCompDC, RGB(100,0,0)) == RGB(0,0,0));
	TEST(GetNearestColor(hCompDC, RGB(250,250,250)) == RGB(255,255,255));
	TEST(GetNearestColor(hCompDC, RGB(120,100,110)) == RGB(128,128,128));

printf("nearest = 0x%x\n", GetNearestColor(hCompDC, RGB(120,100,110)));
#endif

}
