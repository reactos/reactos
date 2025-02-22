/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetClipRgn
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_GetClipRgn()
{
	HWND hWnd;
	HDC hDC;
	HRGN hrgn;//, hrgn2;
	int ret;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, 0, 0);

	hDC = GetDC(hWnd);
	hrgn = CreateRectRgn(0,0,0,0);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	ret = GetClipRgn((HDC)0x12345, hrgn);
	ok(ret == -1, "Expected -1, got %d\n", ret);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

	/* Test invalid hrgn */
	SetLastError(ERROR_SUCCESS);
	ret = GetClipRgn(hDC, (HRGN)0x12345);
	ok(ret == 0, "Expected 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
}

START_TEST(GetClipRgn)
{
    Test_GetClipRgn();
}

