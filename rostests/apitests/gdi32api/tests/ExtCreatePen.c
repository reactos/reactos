#include "../gdi32api.h"

INT
Test_ExtCreatePen(PTESTINFO pti)
{
	HPEN hPen;
	LOGBRUSH logbrush;
	DWORD dwStyles[17] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(1,2,3);
	logbrush.lbHatch = 0;
	hPen = ExtCreatePen(PS_COSMETIC, 1,&logbrush, 0, 0);
	if (!hPen) return FALSE;

	/* Test if we have an EXTPEN */
	RTEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN);
	DeleteObject(hPen);

	/* test userstyles */
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 17, (CONST DWORD*)&dwStyles);
	RTEST(hPen == 0);
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, (CONST DWORD*)&dwStyles);
	RTEST(hPen != 0);

	DeleteObject(hPen);

	return APISTATUS_NORMAL;
}


