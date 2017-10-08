/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiCombineRgn
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiCombineRgn)
{
	HRGN hRgnDest, hRgn1, hRgn2;
// test what params are accepted for what operations
// 0? invalid? are params maybe ignored in some cases?
// LastError

	/* Preparation */
	hRgnDest = CreateRectRgn(0,0,1,1);
	hRgn1 = CreateRectRgn(1,1,4,4);
	hRgn2 = CreateRectRgn(2,2,6,3);

	/* RGN_AND = 1, RGN_OR = 2, RGN_XOR = 3, RGN_DIFF = 4, RGN_COPY = 5 */

	TEST(NtGdiCombineRgn(hRgnDest, hRgn1, hRgn2, 0) == ERROR);
	TEST(NtGdiCombineRgn(hRgnDest, hRgn1, hRgn2, 6) == ERROR);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCombineRgn(hRgnDest, 0, 0, RGN_AND) == ERROR);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCombineRgn(hRgnDest, hRgn1, 0, RGN_AND) == ERROR);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCombineRgn(hRgnDest, 0, hRgn1, RGN_AND) == ERROR);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCombineRgn(0, hRgn1, hRgn2, RGN_AND) == ERROR);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Create intersection */
	TEST(NtGdiCombineRgn(hRgnDest, hRgn1, hRgn2, RGN_AND) == SIMPLEREGION);
	SetRectRgn(hRgn1, 2, 2, 4, 3);
	TEST(NtGdiCombineRgn(hRgnDest, hRgnDest, hRgn1, RGN_XOR) == NULLREGION);

	/* Create intersection with itself */
	SetRectRgn(hRgnDest, 2, 2, 4, 3);
	TEST(NtGdiCombineRgn(hRgnDest, hRgnDest, hRgnDest, RGN_AND) == SIMPLEREGION);
	SetRectRgn(hRgn1, 2, 2, 4, 3);
	TEST(NtGdiCombineRgn(hRgnDest, hRgnDest, hRgn1, RGN_XOR) == NULLREGION);

	/* What if 2 regions are the same */
}

