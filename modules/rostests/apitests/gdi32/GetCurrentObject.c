/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetCurrentObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_GetCurrentObject()
{
	HWND hWnd;
	HDC hDC;
	HBITMAP hBmp;
	HGDIOBJ hObj;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, 0, 0);
	/* Get the DC */
	hDC = GetDC(hWnd);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(NULL, 0);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(NULL, OBJ_BITMAP);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	hObj = GetCurrentObject(NULL, OBJ_BRUSH);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	hObj = GetCurrentObject(NULL, OBJ_COLORSPACE);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	hObj = GetCurrentObject(NULL, OBJ_FONT);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	hObj = GetCurrentObject(NULL, OBJ_PAL);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	hObj = GetCurrentObject(NULL, OBJ_PEN);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject((HDC)-123, OBJ_PEN);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject((HDC)-123, OBJ_BITMAP);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Test invalid types */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 0);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject((HDC)-123, 0);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 3);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(NULL, 3);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 4);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 8);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 9);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 10);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 12);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, 13);
	ok(hObj == 0, "Expected 0, got %p\n", hObj);
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

	/* Default bitmap */
	SetLastError(ERROR_SUCCESS);
	hBmp = GetCurrentObject(hDC, OBJ_BITMAP);
	ok(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP, "Expected GDI_OBJECT_TYPE_BITMAP, got %lu\n", GDI_HANDLE_GET_TYPE(hBmp));
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Other bitmap */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(21));
	ok(hBmp == GetCurrentObject(hDC, OBJ_BITMAP), "\n");
	ok(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP, "Expected GDI_OBJECT_TYPE_BITMAP, got %lu\n", GDI_HANDLE_GET_TYPE(hBmp));
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Default brush */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, OBJ_BRUSH);
	ok(hObj == GetStockObject(WHITE_BRUSH), "Expected %p, got %p\n", GetStockObject(WHITE_BRUSH), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Other brush */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	hObj = GetCurrentObject(hDC, OBJ_BRUSH);
	ok(hObj == GetStockObject(BLACK_BRUSH), "Expected %p, got %p\n", GetStockObject(BLACK_BRUSH), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Default colorspace */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, OBJ_COLORSPACE);
	ok(hObj == GetStockObject(20), "Expected %p, got %p\n", GetStockObject(20), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Default font */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, OBJ_FONT);
	ok(hObj == GetStockObject(SYSTEM_FONT), "Expected %p, got %p\n", GetStockObject(SYSTEM_FONT), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Other font */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	hObj = GetCurrentObject(hDC, OBJ_FONT);
	ok(hObj == GetStockObject(DEFAULT_GUI_FONT), "Expected %p, got %p\n", GetStockObject(DEFAULT_GUI_FONT), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Default palette */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, OBJ_PAL);
	ok(hObj == GetStockObject(DEFAULT_PALETTE), "Expected %p, got %p\n", GetStockObject(DEFAULT_PALETTE), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Default pen */
	SetLastError(ERROR_SUCCESS);
	hObj = GetCurrentObject(hDC, OBJ_PEN);
	ok(hObj == GetStockObject(BLACK_PEN), "Expected %p, got %p\n", GetStockObject(BLACK_PEN), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Other pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	hObj = GetCurrentObject(hDC, OBJ_PEN);
	ok(hObj == GetStockObject(WHITE_PEN), "Expected %p, got %p\n", GetStockObject(WHITE_PEN), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* DC pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DC_PEN));
	hObj = GetCurrentObject(hDC, OBJ_PEN);
	ok(hObj == GetStockObject(DC_PEN), "Expected %p, got %p\n", GetStockObject(DC_PEN), hObj);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
}

START_TEST(GetCurrentObject)
{
    Test_GetCurrentObject();
}

