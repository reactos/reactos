#ifndef _GDITESTLIST_H
#define _GDITESTLIST_H

#include "gdi32api.h"

/* include the tests */
#include "tests/AddFontResource.c"
#include "tests/AddFontResourceEx.c"
#include "tests/BeginPath.c"
#include "tests/CreateBitmapIndirect.c"
#include "tests/CreateCompatibleDC.c"
#include "tests/CreateFontIndirect.c"
#include "tests/CreateFont.c"
#include "tests/CreatePen.c"
#include "tests/CreateRectRgn.c"
#include "tests/EngCreateSemaphore.c"
#include "tests/EngAcquireSemaphore.c"
#include "tests/EngDeleteSemaphore.c"
#include "tests/EngReleaseSemaphore.c"
#include "tests/ExtCreatePen.c"
#include "tests/GdiConvertBitmap.c"
#include "tests/GdiConvertBrush.c"
#include "tests/GdiConvertDC.c"
#include "tests/GdiConvertFont.c"
#include "tests/GdiConvertPalette.c"
#include "tests/GdiConvertRegion.c"
#include "tests/GdiDeleteLocalDC.c"
#include "tests/GdiGetCharDimensions.c"
#include "tests/GdiGetLocalBrush.c"
#include "tests/GdiGetLocalDC.c"
#include "tests/GdiReleaseLocalDC.c"
#include "tests/GdiSetAttrs.c"
#include "tests/GetClipRgn.c"
#include "tests/GetCurrentObject.c"
#include "tests/GetDIBits.c"
#include "tests/GetObject.c"
#include "tests/GetStockObject.c"
#include "tests/GetTextFace.c"
#include "tests/SelectObject.c"
#include "tests/SetDCPenColor.c"
#include "tests/SetMapMode.c"
#include "tests/SetSysColors.c"
#include "tests/SetWorldTransform.c"


/* The List of tests */
TESTENTRY TestList[] =
{
	{ L"AddFontResourceA", Test_AddFontResourceA },
	{ L"AddFontResourceEx", Test_AddFontResourceEx },
	{ L"BeginPath", Test_BeginPath },
	{ L"CreateBitmapIndirect", Test_CreateBitmapIndirect },
	{ L"CreateCompatibleDC", Test_CreateCompatibleDC },
	{ L"CreateFontIndirect", Test_CreateFontIndirect },
	{ L"CreateFont", Test_CreateFont },
	{ L"CreatePen", Test_CreatePen },
	{ L"EngCreateSemaphore", Test_EngCreateSemaphore },
	{ L"EngAcquireSemaphore", Test_EngAcquireSemaphore },
	{ L"EngDeleteSemaphore", Test_EngDeleteSemaphore },
	{ L"EngReleaseSemaphore", Test_EngReleaseSemaphore },
	{ L"CreateRectRgn", Test_CreateRectRgn },
	{ L"ExtCreatePen", Test_ExtCreatePen },
	{ L"GdiConvertBitmap", Test_GdiConvertBitmap },
	{ L"GdiConvertBrush", Test_GdiConvertBrush },
	{ L"GdiConvertDC", Test_GdiConvertDC },
	{ L"GdiConvertFont", Test_GdiConvertFont },
	{ L"GdiConvertPalette", Test_GdiConvertPalette },
	{ L"GdiConvertRegion", Test_GdiConvertRegion },
	{ L"GdiDeleteLocalDC", Test_GdiDeleteLocalDC },
	{ L"GdiGetCharDimensions", Test_GdiGetCharDimensions },
	{ L"GdiGetLocalBrush", Test_GdiGetLocalBrush },
	{ L"GdiGetLocalDC", Test_GdiGetLocalDC },
	{ L"GdiReleaseLocalDC", Test_GdiReleaseLocalDC },
	{ L"GdiSetAttrs", Test_GdiSetAttrs },
	{ L"GetClipRgn", Test_GetClipRgn },
	{ L"GetCurrentObject", Test_GetCurrentObject },
	{ L"GetDIBits", Test_GetDIBits },
	{ L"GetObject", Test_GetObject },
	{ L"GetStockObject", Test_GetStockObject },
	{ L"GetTextFace", Test_GetTextFace },
	{ L"SelectObject", Test_SelectObject },
	{ L"SetDCPenColor", Test_SetDCPenColor },
	{ L"SetMapMode", Test_SetMapMode },
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
