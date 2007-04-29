#ifndef _GDITESTLIST_H
#define _GDITESTLIST_H

#include "gditest.h"

/* include the tests */
#include "tests/CreateCompatibleDC.c"
#include "tests/CreatePen.c"
#include "tests/ExtCreatePen.c"
#include "tests/GetObject.c"
#include "tests/GetStockObject.c"
#include "tests/SelectObject.c"
#include "tests/SetDCPenColor.c"
#include "tests/SetSysColors.c"

/* The List of tests */
TEST TestList[] =
{
	{ "CreateCompatibleDC", Test_CreateCompatibleDC },
	{ "CreatePen", Test_CreatePen },
	{ "ExtCreatePen", Test_ExtCreatePen },
	{ "GetStockObject", Test_GetStockObject },
	{ "SetSysColors", Test_SetSysColors },
	{ "SelectObject", Test_SelectObject },
	{ "SetDCPenColor", Test_SetDCPenColor },
	{ "GetObject", Test_GetObject }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TEST);
}

#endif /* _GDITESTLIST_H */

/* EOF */
