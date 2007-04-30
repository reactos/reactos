#line 1 "CreatePen.c"

#include "..\gditest.h"

BOOL
Test_CreatePen(INT* passed, INT* failed)
{
	HPEN hPen;

	hPen = CreatePen(PS_COSMETIC, 1, RGB(1,2,3));
	if (!hPen) return FALSE;

	/* Test if we have a PEN */
	TEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_PEN);
	
	DeleteObject(hPen);
	return TRUE;
}

