/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreatePen
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_CreatePen()
{
	HPEN hPen;
	LOGPEN logpen;

	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_DASHDOT, 5, RGB(1,2,3));
	ok(hPen != 0, "CreatePen failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* Test if we have a PEN */
	ok(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_PEN, "Expected GDI_OBJECT_TYPE_PEN, got %lu\n", GDI_HANDLE_GET_TYPE(hPen));

	ok(GetObject(hPen, sizeof(logpen), &logpen), "GetObject failed\n");
	ok(logpen.lopnStyle == PS_DASHDOT, "Expected PS_DASHDOT, got %u\n", logpen.lopnStyle);
	ok(logpen.lopnWidth.x == 5, "Expected 5, got %lu\n", logpen.lopnWidth.x);
	ok(logpen.lopnColor == RGB(1,2,3), "Expected %x, got %x\n", (unsigned)RGB(1,2,3), (unsigned)logpen.lopnColor);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
	if(hPen)
		ok(DeleteObject(hPen), "DeleteObject failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* PS_GEOMETRIC | PS_DASHDOT = 0x00001011 will become PS_SOLID */
	SetLastError(ERROR_SUCCESS);
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_GEOMETRIC | PS_DASHDOT, 5, RGB(1,2,3));
	ok(hPen != 0, "CreatePen failed\n");
	ok(GetObject(hPen, sizeof(logpen), &logpen), "GetObject failed\n");
	ok(logpen.lopnStyle == PS_SOLID, "Expected PS_SOLID, got %u\n", logpen.lopnStyle);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
	if(hPen)
		ok(DeleteObject(hPen), "DeleteObject failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* PS_USERSTYLE will become PS_SOLID */
	logpen.lopnStyle = 22;
	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_USERSTYLE, 5, RGB(1,2,3));
	ok(hPen != 0, "CreatePen failed\n");
	ok(GetObject(hPen, sizeof(logpen), &logpen), "GetObject failed\n");
	ok(logpen.lopnStyle == PS_SOLID, "Expected PS_SOLID, got %u\n", logpen.lopnStyle);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
	if(hPen)
		ok(DeleteObject(hPen), "DeleteObject failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* PS_ALTERNATE will become PS_SOLID */
	logpen.lopnStyle = 22;
	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_ALTERNATE, 5, RGB(1,2,3));
	ok(hPen != 0, "CreatePen failed\n");
	ok(GetObject(hPen, sizeof(logpen), &logpen), "GetObject failed\n");
	ok(logpen.lopnStyle == PS_SOLID, "Expected PS_SOLID, got %u\n", logpen.lopnStyle);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
	if(hPen)
		ok(DeleteObject(hPen), "DeleteObject failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());

	/* PS_INSIDEFRAME is ok */
	logpen.lopnStyle = 22;
	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_INSIDEFRAME, 5, RGB(1,2,3));
	ok(hPen != 0, "CreatePen failed\n");
	ok(GetObject(hPen, sizeof(logpen), &logpen), "GetObject failed\n");
	ok(logpen.lopnStyle == PS_INSIDEFRAME, "Expected PS_INSIDEFRAME, got %u\n", logpen.lopnStyle);
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
	if(hPen)
		ok(DeleteObject(hPen), "DeleteObject failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", GetLastError());
}

START_TEST(CreatePen)
{
    Test_CreatePen();
}

