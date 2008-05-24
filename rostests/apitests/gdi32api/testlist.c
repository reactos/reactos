#ifndef _GDITESTLIST_H
#define _GDITESTLIST_H

#include "gdi32api.h"

/* include the tests */
#include "tests/AddFontResource.c"
#include "tests/AddFontResourceEx.c"
#include "tests/CreateBitmapIndirect.c"
#include "tests/CreateCompatibleDC.c"
#include "tests/CreateFont.c"
#include "tests/CreatePen.c"
#include "tests/CreateRectRgn.c"
#include "tests/ExtCreatePen.c"
#include "tests/GdiConvertBitmap.c"
#include "tests/GdiConvertBrush.c"
#include "tests/GdiConvertDC.c"
#include "tests/GdiConvertFont.c"
#include "tests/GdiConvertPalette.c"
#include "tests/GdiConvertRegion.c"
#include "tests/GdiGetLocalBrush.c"
#include "tests/GdiGetLocalDC.c"
#include "tests/GetClipRgn.c"
#include "tests/GetCurrentObject.c"
#include "tests/GetDIBits.c"
#include "tests/GetObject.c"
#include "tests/GetStockObject.c"
#include "tests/SelectObject.c"
#include "tests/SetDCPenColor.c"
#include "tests/SetSysColors.c"
#include "tests/SetWorldTransform.c"









/* The List of tests */
TESTENTRY TestList[] =
{
	{ L"AddFontResourceA", Test_AddFontResourceA },
	{ L"AddFontResourceEx", Test_AddFontResourceEx },
	{ L"CreateBitmapIndirect", Test_CreateBitmapIndirect },
	{ L"CreateCompatibleDC", Test_CreateCompatibleDC },
	{ L"CreateFont", Test_CreateFont },
	{ L"CreatePen", Test_CreatePen },
	{ L"CreateRectRgn", Test_CreateRectRgn },
	{ L"ExtCreatePen", Test_ExtCreatePen },
	{ L"GdiConvertBitmap", Test_GdiConvertBitmap },
	{ L"GdiConvertBrush", Test_GdiConvertBrush },
	{ L"GdiConvertBrush", Test_GdiConvertDC },
	{ L"GdiConvertFont", Test_GdiConvertFont },
	{ L"GdiConvertPalette", Test_GdiConvertPalette },
	{ L"GdiConvertRegion", Test_GdiConvertRegion },
	{ L"GdiGetLocalBrush", Test_GdiGetLocalBrush },
	{ L"GdiGetLocalDC", Test_GdiGetLocalDC },
	{ L"GetClipRgn", Test_GetClipRgn },
	{ L"GetCurrentObject", Test_GetCurrentObject },
	{ L"GetDIBits", Test_GetDIBits },
	{ L"GetObject", Test_GetObject },
	{ L"GetStockObject", Test_GetStockObject },
	{ L"SelectObject", Test_SelectObject },
	{ L"SetDCPenColor", Test_SetDCPenColor },
	{ L"SetSysColors", Test_SetSysColors },
	{ L"SetWorldTransform", Test_SetWorldTransform },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

#endif /* _GDITESTLIST_H */

/* EOF */
