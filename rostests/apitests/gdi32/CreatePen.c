/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreatePen
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include <winddi.h>
#include <reactos/win32k/ntgdityp.h>
#include <reactos/win32k/ntgdihdl.h>

void Test_CreatePen()
{
	HPEN hPen;
	LOGPEN logpen;

	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_DASHDOT, 5, RGB(1,2,3));
	ok(hPen != 0, "\n");

	/* Test if we have a PEN */
	ok(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_PEN, "\n");

	GetObject(hPen, sizeof(logpen), &logpen);
	ok(logpen.lopnStyle == PS_DASHDOT, "\n");
	ok(logpen.lopnWidth.x == 5, "\n");
	ok(logpen.lopnColor == RGB(1,2,3), "\n");
	DeleteObject(hPen);

	/* PS_GEOMETRIC | PS_DASHDOT = 0x00001011 will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_GEOMETRIC | PS_DASHDOT, 5, RGB(1,2,3));
	ok(hPen != 0, "\n");
	GetObject(hPen, sizeof(logpen), &logpen);
	ok(logpen.lopnStyle == PS_SOLID, "\n");
	DeleteObject(hPen);

	/* PS_USERSTYLE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_USERSTYLE, 5, RGB(1,2,3));
	ok(hPen != 0, "\n");
	GetObject(hPen, sizeof(logpen), &logpen);
	ok(logpen.lopnStyle == PS_SOLID, "\n");
	DeleteObject(hPen);

	/* PS_ALTERNATE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_ALTERNATE, 5, RGB(1,2,3));
	ok(hPen != 0, "\n");
	GetObject(hPen, sizeof(logpen), &logpen);
	ok(logpen.lopnStyle == PS_SOLID, "\n");
	DeleteObject(hPen);

	/* PS_INSIDEFRAME is ok */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_INSIDEFRAME, 5, RGB(1,2,3));
	ok(hPen != 0, "\n");
	GetObject(hPen, sizeof(logpen), &logpen);
	ok(logpen.lopnStyle == PS_INSIDEFRAME, "\n");
	DeleteObject(hPen);

	ok(GetLastError() == ERROR_SUCCESS, "\n");
}

START_TEST(CreatePen)
{
    Test_CreatePen();
}

