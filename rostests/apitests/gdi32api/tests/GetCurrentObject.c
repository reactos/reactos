
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
	TEST(GetCurrentObject(NULL, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(NULL, OBJ_BITMAP) == 0);
	TEST(GetCurrentObject(NULL, OBJ_BRUSH) == 0);
	TEST(GetCurrentObject(NULL, OBJ_COLORSPACE) == 0);
	TEST(GetCurrentObject(NULL, OBJ_FONT) == 0);
	TEST(GetCurrentObject(NULL, OBJ_PAL) == 0);
	TEST(GetCurrentObject(NULL, OBJ_PEN) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject((HDC)-123, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject((HDC)-123, OBJ_BITMAP) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid types */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 3) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 4) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 8) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 9) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 10) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 12) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, 13) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Default bitmap */
	SetLastError(ERROR_SUCCESS);
	HBITMAP hBmp;
	hBmp = GetCurrentObject(hDC, OBJ_BITMAP);
	TEST(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Other bitmap */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(21));
	TEST(hBmp == GetCurrentObject(hDC, OBJ_BITMAP));
	TEST(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Default brush */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(WHITE_BRUSH));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Other brush */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	TEST(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(BLACK_BRUSH));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Default colorspace */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, OBJ_COLORSPACE) == GetStockObject(20));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Default font */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(SYSTEM_FONT));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Other font */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	TEST(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(DEFAULT_GUI_FONT));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Default palette */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, OBJ_PAL) == GetStockObject(DEFAULT_PALETTE));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Default pen */
	SetLastError(ERROR_SUCCESS);
	TEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(BLACK_PEN));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Other pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	TEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(WHITE_PEN));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* DC pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DC_PEN));
	TEST(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(DC_PEN));
	TEST(GetLastError() == ERROR_SUCCESS);

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}

