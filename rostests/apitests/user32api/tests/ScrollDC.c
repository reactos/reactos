#include "../user32api.h"

INT
Test_ScrollDC(PTESTINFO pti)
{
	HWND hWnd, hWnd2;
	HDC hDC;
	HRGN hrgn;
	RECT rcClip;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    100, 100, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	UpdateWindow(hWnd);
	hDC = GetDC(hWnd);

	/* Assert that no update region is there */
	hrgn = CreateRectRgn(0,0,0,0);
	ASSERT1(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

    /* Test normal scrolling */
	TEST(ScrollDC(hDC, 0, 0, NULL, NULL, hrgn, NULL) == TRUE);

    /* Scroll with invalid update region */
	DeleteObject(hrgn);
	TEST(ScrollDC(hDC, 50, 0, NULL, NULL, hrgn, NULL) == FALSE);
	hrgn = CreateRectRgn(0,0,0,0);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

    /* Scroll with invalid update rect pointer */
	TEST(ScrollDC(hDC, 50, 0, NULL, NULL, NULL, (PRECT)1) == 0);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

    /* Scroll with a clip rect */
    rcClip.left = 50; rcClip.top = 0; rcClip.right = 100; rcClip.bottom = 100;
	TEST(ScrollDC(hDC, 50, 0, NULL, &rcClip, hrgn, NULL) == TRUE);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

    /* Scroll with a clip rect */
    rcClip.left = 50; rcClip.top = 0; rcClip.right = 100; rcClip.bottom = 100;
	TEST(ScrollDC(hDC, 50, 50, NULL, &rcClip, hrgn, NULL) == TRUE);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

	/* Overlap with another window */
	hWnd2 = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    30, 160, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	UpdateWindow(hWnd2);

    /* Cleanup */
	ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
    DestroyWindow(hWnd2);

	return APISTATUS_NORMAL;
}
