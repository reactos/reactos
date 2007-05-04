#line 2 "CreatePen.c"

#include "../gditest.h"

BOOL
Test_CreatePen(INT* passed, INT* failed)
{
	HPEN hPen;
	LOGPEN logpen;

	SetLastError(ERROR_SUCCESS);

	hPen = CreatePen(PS_DASHDOT, 5, RGB(1,2,3));
	TEST(hPen);

	/* Test if we have a PEN */
	TEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_PEN);

	GetObject(hPen, sizeof(logpen), &logpen);
	TEST(logpen.lopnStyle == PS_DASHDOT);
	TEST(logpen.lopnWidth.x == 5);
	TEST(logpen.lopnColor == RGB(1,2,3));
	DeleteObject(hPen);

	/* PS_GEOMETRIC | PS_DASHDOT = 0x00001011 will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_GEOMETRIC | PS_DASHDOT, 5, RGB(1,2,3));
	TEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	TEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_USERSTYLE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_USERSTYLE, 5, RGB(1,2,3));
	TEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	TEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_ALTERNATE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_ALTERNATE, 5, RGB(1,2,3));
	TEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	TEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_INSIDEFRAME is ok */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_INSIDEFRAME, 5, RGB(1,2,3));
	TEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	TEST(logpen.lopnStyle == PS_INSIDEFRAME);
	DeleteObject(hPen);

	TEST(GetLastError() == ERROR_SUCCESS);

	return TRUE;
}

