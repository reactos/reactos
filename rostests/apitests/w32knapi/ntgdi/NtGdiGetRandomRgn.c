#include "../w32knapi.h"

W32KAPI
INT
APIENTRY
NtGdiGetRandomRgn(
    IN HDC hdc,
    IN HRGN hrgn,
    IN INT iRgn)
{
	return (INT)Syscall(L"NtGdiGetRandomRgn", 3, &hdc);
}


INT
Test_NtGdiGetRandomRgn(PTESTINFO pti)
{
	HWND hWnd;
	HDC hDC;
	HRGN hrgn, hrgn2;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
//	UpdateWindow(hWnd);
	hDC = GetDC(hWnd);

	ASSERT1(hDC != NULL);

	hrgn = CreateRectRgn(0,0,0,0);
	hrgn2 = CreateRectRgn(3,3,10,10);
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiGetRandomRgn(0, hrgn, 0) == -1);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiGetRandomRgn(hDC, 0, 0) == 0);
	TEST(NtGdiGetRandomRgn(hDC, (HRGN)-5, 0) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 0) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 1) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 2) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 3) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 4) == 1);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 5) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 10) == 0);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, -10) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SelectClipRgn(hDC, hrgn2);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 1) == 1);
	TEST(CombineRgn(hrgn, hrgn, hrgn, RGN_OR) == SIMPLEREGION);
	TEST(CombineRgn(hrgn, hrgn, hrgn2, RGN_XOR) == NULLREGION);

	SetRectRgn(hrgn2,0,0,0,0);
	SelectClipRgn(hDC, hrgn2);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 1) == 1);

	TEST(CombineRgn(hrgn2, hrgn, hrgn2, RGN_XOR) == NULLREGION);
	TEST(CombineRgn(hrgn2, hrgn, hrgn, RGN_OR) == NULLREGION);

	SelectClipRgn(hDC, NULL);
	TEST(NtGdiGetRandomRgn(hDC, hrgn, 1) == 0);


	TEST(NtGdiGetRandomRgn(hDC, hrgn, 4) == 1);

	RTEST(GetLastError() == ERROR_SUCCESS);

	DeleteDC(hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
