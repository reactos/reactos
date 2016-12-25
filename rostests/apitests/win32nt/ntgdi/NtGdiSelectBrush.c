/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiSelectBrush
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiSelectBrush)
{
	HDC hDC;
	HBRUSH hBrush, hOldBrush;
	DC_ATTR *pdcattr;

	hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);

	hBrush = GetStockObject(GRAY_BRUSH);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(NULL, hBrush);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush((HDC)((ULONG_PTR)hDC & 0x0000ffff), hBrush);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL brush */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, NULL);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid brush */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, (HBRUSH)((ULONG_PTR)hBrush & 0x0000ffff));
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, hBrush);
	TEST(hOldBrush != NULL);
	hOldBrush = NtGdiSelectBrush(hDC, hOldBrush);
	TEST(hOldBrush == hBrush);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Begin with a white brush */
	NtGdiSelectBrush(hDC, GetStockObject(WHITE_BRUSH));
	/* Select a brush in user mode */
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	/* See what we get returned */
	hOldBrush = NtGdiSelectBrush(hDC, GetStockObject(WHITE_BRUSH));
	TEST(hOldBrush == GetStockObject(BLACK_BRUSH));


	/* Begin with a white brush */
	NtGdiSelectBrush(hDC, GetStockObject(WHITE_BRUSH));

	pdcattr = GdiGetHandleUserData(hDC);
	/* Change the brush in user mode, without setting flags */
	pdcattr->hbrush = (HBRUSH)12345;

	hOldBrush = NtGdiSelectBrush(hDC, GetStockObject(BLACK_BRUSH));
	TEST(hOldBrush == (HBRUSH)12345);


	DeleteDC(hDC);
}

