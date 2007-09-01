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

	SetLastError(ERROR_SUCCESS);
//	TEST(GetClipRgn(hDC)

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}

