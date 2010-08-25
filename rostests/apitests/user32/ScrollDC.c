/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ScrollDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void Test_ScrollDC()
{
	HWND hWnd, hWnd2;
	HDC hDC;
	HRGN hrgn;
	RECT rcClip;
	int iResult;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    100, 100, 100, 100,
	                    NULL, NULL, 0, 0);
	UpdateWindow(hWnd);
	hDC = GetDC(hWnd);

	/* Test that no update region is there */
	hrgn = CreateRectRgn(0,0,0,0);
	iResult = GetUpdateRgn(hWnd, hrgn, FALSE);
	ok (iResult == NULLREGION, "\n");

    /* Test normal scrolling */
	ok(ScrollDC(hDC, 0, 0, NULL, NULL, hrgn, NULL) == TRUE, "\n");

    /* Scroll with invalid update region */
	DeleteObject(hrgn);
	ok(ScrollDC(hDC, 50, 0, NULL, NULL, hrgn, NULL) == FALSE, "\n");
	hrgn = CreateRectRgn(0,0,0,0);
	ok(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION, "\n");

    /* Scroll with invalid update rect pointer */
	ok(ScrollDC(hDC, 50, 0, NULL, NULL, NULL, (PRECT)1) == 0, "\n");
	ok(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION, "\n");

    /* Scroll with a clip rect */
    rcClip.left = 50; rcClip.top = 0; rcClip.right = 100; rcClip.bottom = 100;
	ok(ScrollDC(hDC, 50, 0, NULL, &rcClip, hrgn, NULL) == TRUE, "\n");
	ok(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION, "\n");

    /* Scroll with a clip rect */
    rcClip.left = 50; rcClip.top = 0; rcClip.right = 100; rcClip.bottom = 100;
	ok(ScrollDC(hDC, 50, 50, NULL, &rcClip, hrgn, NULL) == TRUE, "\n");
	ok(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION, "\n");

	/* Overlap with another window */
	hWnd2 = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    30, 160, 100, 100,
	                    NULL, NULL, 0, 0);
	UpdateWindow(hWnd2);

    /* Cleanup */
	ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
    DestroyWindow(hWnd2);

}

START_TEST(ScrollDC)
{
    Test_ScrollDC();
}

