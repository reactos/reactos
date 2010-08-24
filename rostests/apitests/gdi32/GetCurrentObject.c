/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetCurrentObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include <winddi.h>
#include <reactos/win32k/ntgdityp.h>
#include <reactos/win32k/ntgdihdl.h>

void Test_GetCurrentObject()
{
	HWND hWnd;
	HDC hDC;
	HBITMAP hBmp;

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, 0, 0);
	/* Get the DC */
	hDC = GetDC(hWnd);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(NULL, 0) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(NULL, OBJ_BITMAP) == 0, "\n");
	ok(GetCurrentObject(NULL, OBJ_BRUSH) == 0, "\n");
	ok(GetCurrentObject(NULL, OBJ_COLORSPACE) == 0, "\n");
	ok(GetCurrentObject(NULL, OBJ_FONT) == 0, "\n");
	ok(GetCurrentObject(NULL, OBJ_PAL) == 0, "\n");
	ok(GetCurrentObject(NULL, OBJ_PEN) == 0, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject((HDC)-123, 0) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject((HDC)-123, OBJ_BITMAP) == 0, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Test invalid types */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 0) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 3) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 4) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 8) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 9) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 10) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 12) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, 13) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	/* Default bitmap */
	SetLastError(ERROR_SUCCESS);
	hBmp = GetCurrentObject(hDC, OBJ_BITMAP);
	ok(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Other bitmap */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(21));
	ok(hBmp == GetCurrentObject(hDC, OBJ_BITMAP), "\n");
	ok(GDI_HANDLE_GET_TYPE(hBmp) == GDI_OBJECT_TYPE_BITMAP, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Default brush */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(WHITE_BRUSH), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Other brush */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	ok(GetCurrentObject(hDC, OBJ_BRUSH) == GetStockObject(BLACK_BRUSH), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Default colorspace */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, OBJ_COLORSPACE) == GetStockObject(20), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Default font */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(SYSTEM_FONT), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Other font */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	ok(GetCurrentObject(hDC, OBJ_FONT) == GetStockObject(DEFAULT_GUI_FONT), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Default palette */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, OBJ_PAL) == GetStockObject(DEFAULT_PALETTE), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Default pen */
	SetLastError(ERROR_SUCCESS);
	ok(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(BLACK_PEN), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* Other pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	ok(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(WHITE_PEN), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	/* DC pen */
	SetLastError(ERROR_SUCCESS);
	SelectObject(hDC, GetStockObject(DC_PEN));
	ok(GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(DC_PEN), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
}

START_TEST(GetCurrentObject)
{
    Test_GetCurrentObject();
}

