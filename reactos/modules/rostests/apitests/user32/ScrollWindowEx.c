/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ScrollWindowEx
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

void Test_ScrollWindowEx()
{
	HWND hWnd;
	HRGN hrgn;
	int Result;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, 0, 0);
	UpdateWindow(hWnd);

	/* Assert that no update region is there */
	hrgn = CreateRectRgn(0,0,0,0);
	Result = GetUpdateRgn(hWnd, hrgn, FALSE);
	ok(Result == NULLREGION, "Result = %d\n", Result);

	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, 0);
	ok(Result == SIMPLEREGION, "Result = %d\n", Result);
	Result = GetUpdateRgn(hWnd, hrgn, FALSE);
	ok(Result == NULLREGION, "Result = %d\n", Result);

	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
	ok(Result == SIMPLEREGION, "Result = %d\n", Result);
	Result = GetUpdateRgn(hWnd, hrgn, FALSE);
	ok(Result == SIMPLEREGION, "Result = %d\n", Result);
	UpdateWindow(hWnd);

	// test invalid update region
	DeleteObject(hrgn);
	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, hrgn, NULL, SW_INVALIDATE);
	ok(Result == ERROR, "Result = %d\n", Result);
	hrgn = CreateRectRgn(0,0,0,0);
	UpdateWindow(hWnd);

	// Test invalid updaterect pointer
	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, (LPRECT)1, SW_INVALIDATE);
	ok(Result == ERROR, "Result = %d\n", Result);
	Result = GetUpdateRgn(hWnd, hrgn, FALSE);
	ok(Result == SIMPLEREGION, "Result = %d\n", Result);

// test for alignment of rects

	DeleteObject(hrgn);
    DestroyWindow(hWnd);
}

START_TEST(ScrollWindowEx)
{
    Test_ScrollWindowEx();
}

