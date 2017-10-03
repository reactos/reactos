/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiSelectFont
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiSelectFont)
{
	HDC hDC;
	HFONT hFont, hOldFont;

	hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);

	hFont = GetStockObject(DEFAULT_GUI_FONT);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldFont = NtGdiSelectFont(NULL, hFont);
	TEST(hOldFont == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldFont = NtGdiSelectFont((HDC)((ULONG_PTR)hDC & 0x0000ffff), hFont);
	TEST(hOldFont == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL font */
	SetLastError(ERROR_SUCCESS);
	hOldFont = NtGdiSelectFont(hDC, NULL);
	TEST(hOldFont == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid font */
	SetLastError(ERROR_SUCCESS);
	hOldFont = NtGdiSelectFont(hDC, (HFONT)((ULONG_PTR)hFont & 0x0000ffff));
	TEST(hOldFont == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	hOldFont = NtGdiSelectFont(hDC, hFont);
	TEST(hOldFont != NULL);
	hOldFont = NtGdiSelectFont(hDC, hOldFont);
	TEST(hOldFont == hFont);
	TEST(GetLastError() == ERROR_SUCCESS);


	DeleteDC(hDC);
}

