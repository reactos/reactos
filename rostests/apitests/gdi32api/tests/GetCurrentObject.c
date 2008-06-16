
INT
Test_GetCurrentObject(PTESTINFO pti)
{
	HWND hWnd;
	HDC hDC;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	/* Get the DC */
	hDC = GetDC(hWnd);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(NULL, OBJ_BITMAP) == 0);
	RTEST(GetCurrentObject(NULL, OBJ_BRUSH) == 0);
	RTEST(GetCurrentObject(NULL, OBJ_COLORSPACE) == 0);
	RTEST(GetCurrentObject(NULL, OBJ_FONT) == 0);
	RTEST(GetCurrentObject(NULL, OBJ_PAL) == 0);
	RTEST(GetCurrentObject(NULL, OBJ_PEN) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject((HDC)-123, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject((HDC)-123, OBJ_BITMAP) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid types */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 3) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 4) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 8) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 9) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 10) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 12) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, 13) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Default bitmap */
	SetLastError(ERROR_SUCCESS);
	HBITMAP hBmp;
	hBmp = GetCurrentObject(hDC, OBJ_BITMAP);
	RTEST(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Other bitmap */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(21));
	RTEST(hBmp == GetCurrentObject(hDC, OBJ_BITMAP));
	RTEST(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Default brush */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(WHITE_BRUSH));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Other brush */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	RTEST(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(BLACK_BRUSH));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Default colorspace */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, OBJ_COLORSPACE) == GetStockObject(20));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Default font */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(SYSTEM_FONT));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Other font */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	RTEST(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(DEFAULT_GUI_FONT));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Default palette */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, OBJ_PAL) == GetStockObject(DEFAULT_PALETTE));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Default pen */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(BLACK_PEN));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Other pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	RTEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(WHITE_PEN));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* DC pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DC_PEN));
	RTEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(DC_PEN));
	RTEST(GetLastError() == ERROR_SUCCESS);

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}

