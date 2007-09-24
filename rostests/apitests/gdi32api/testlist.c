#ifndef _GDITESTLIST_H
#define _GDITESTLIST_H

#include "gdi32api.h"

/* include the tests */
#include "tests/AddFontResourceEx.c"
#include "tests/CreateCompatibleDC.c"
#include "tests/CreateFont.c"
#include "tests/CreatePen.c"
#include "tests/CreateRectRgn.c"
#include "tests/ExtCreatePen.c"
#include "tests/GetClipRgn.c"
#include "tests/GetObject.c"
#include "tests/GetStockObject.c"
#include "tests/GetDIBits.c"
#include "tests/SelectObject.c"
#include "tests/SetDCPenColor.c"
#include "tests/SetSysColors.c"
//#include "tests/SetWorldTransform.c"

/* The List of tests */
TESTENTRY TestList[] =
{
	{ L"AddFontResourceEx", Test_AddFontResourceEx },
	{ L"CreateCompatibleDC", Test_CreateCompatibleDC },
	{ L"CreateFont", Test_CreateFont },
	{ L"CreatePen", Test_CreatePen },
	{ L"CreateRectRgn", Test_CreateRectRgn },
	{ L"ExtCreatePen", Test_ExtCreatePen },
	{ L"GetClipRgn", Test_GetClipRgn },
	{ L"GetObject", Test_GetObject },
	{ L"GetStockObject", Test_GetStockObject },
	{ L"GetDIBits", Test_GetDIBits },
	{ L"SetSysColors", Test_SetSysColors },
	{ L"SelectObject", Test_SelectObject },
	{ L"SetDCPenColor", Test_SetDCPenColor },
//	{ L"SetWorldTransform", Test_SetWorldTransform },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

#endif /* _GDITESTLIST_H */

/* EOF */
