/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetClipRgn
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void Test_GetClipRgn()
{
	HWND hWnd;
	HDC hDC;
	HRGN hrgn;//, hrgn2;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, 0, 0);

	hDC = GetDC(hWnd);
	hrgn = CreateRectRgn(0,0,0,0);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	ok(GetClipRgn((HDC)0x12345, hrgn) == -1, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	/* Test invalid hrgn */
	SetLastError(ERROR_SUCCESS);
	ok(GetClipRgn(hDC, (HRGN)0x12345) == 0, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
}

START_TEST(GetClipRgn)
{
    Test_GetClipRgn();
}

