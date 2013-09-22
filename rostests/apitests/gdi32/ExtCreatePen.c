/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ExtCreatePen
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winddi.h>
#include <include/ntgdityp.h>
#include <include/ntgdihdl.h>

void Test_ExtCreatePen()
{
	HPEN hPen;
	LOGBRUSH logbrush;
	DWORD dwStyles[17] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(1,2,3);
	logbrush.lbHatch = 0;
	hPen = ExtCreatePen(PS_COSMETIC, 1,&logbrush, 0, 0);
	ok(hPen != 0, "ExtCreatePen failed\n");
	if (!hPen) return;

	/* Test if we have an EXTPEN */
	ok(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN, "hPen=%p\n", hPen);
	DeleteObject(hPen);

	/* test userstyles */
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 17, (CONST DWORD*)&dwStyles);
	ok(hPen == 0, "\n");
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, (CONST DWORD*)&dwStyles);
	ok(hPen != 0, "\n");

	DeleteObject(hPen);
}

START_TEST(ExtCreatePen)
{
    Test_ExtCreatePen();
}

