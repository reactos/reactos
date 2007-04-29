#include "..\gditest.h"

BOOL
Test_ExtCreatePen(INT* passed, INT* failed)
{
	HPEN hPen;
	LOGBRUSH logbrush;

	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(1,2,3);
	logbrush.lbHatch = 0;
	hPen = ExtCreatePen(PS_COSMETIC, 1,&logbrush, 0, 0);
	if (!hPen) return FALSE;

	/* Test if we have an EXTPEN */
	TEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN);

	
	DeleteObject(hPen);
	return TRUE;
}


