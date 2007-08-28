#include "../user32api.h"

INT
Test_ScrollWindowEx(PTESTINFO pti)
{
	HWND hWnd;
	HRGN hrgn;
	int Result;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	UpdateWindow(hWnd);

	/* Assert that no update region is there */
	hrgn = CreateRectRgn(0,0,0,0);
	ASSERT(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, 0);
	TEST(Result == SIMPLEREGION);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == NULLREGION);

	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
	TEST(Result == SIMPLEREGION);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == SIMPLEREGION);
	UpdateWindow(hWnd);

	// test invalid update region
	DeleteObject(hrgn);
	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, hrgn, NULL, SW_INVALIDATE);
	TEST(Result == ERROR);
	hrgn = CreateRectRgn(0,0,0,0);
	UpdateWindow(hWnd);

	// Test invalid updaterect pointer
	Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, (LPRECT)1, SW_INVALIDATE);
	TEST(Result == ERROR);
	TEST(GetUpdateRgn(hWnd, hrgn, FALSE) == SIMPLEREGION);

// test for alignment of rects

	DeleteObject(hrgn);
    DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
