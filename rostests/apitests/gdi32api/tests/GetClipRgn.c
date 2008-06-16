INT
Test_GetClipRgn(PTESTINFO pti)
{
	HWND hWnd;
	HDC hDC;
	HRGN hrgn;//, hrgn2;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);

	hDC = GetDC(hWnd);
	hrgn = CreateRectRgn(0,0,0,0);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetClipRgn((HDC)0x12345, hrgn) == -1);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test invalid hrgn */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetClipRgn(hDC, (HRGN)0x12345) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}

