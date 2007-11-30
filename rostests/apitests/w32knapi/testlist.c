#include "w32knapi.h"

/* include the tests */

#include "ntdd/NtGdiDdCreateDirectDrawObject.c"
#include "ntdd/NtGdiDdDeleteDirectDrawObject.c"
#include "ntdd/NtGdiDdQueryDirectDrawObject.c"

#include "ntgdi/NtGdiArcInternal.c"
#include "ntgdi/NtGdiBitBlt.c"
#include "ntgdi/NtGdiCreateBitmap.c"
#include "ntgdi/NtGdiCreateCompatibleBitmap.c"
#include "ntgdi/NtGdiDoPalette.c"
#include "ntgdi/NtGdiEngCreatePalette.c"
//#include "ntgdi/NtGdiEnumFontChunk.c"
#include "ntgdi/NtGdiEnumFontOpen.c"
#include "ntgdi/NtGdiGetBitmapBits.c"
#include "ntgdi/NtGdiGetRandomRgn.c"
#include "ntgdi/NtGdiSelectBitmap.c"
#include "ntgdi/NtGdiSelectBrush.c"
#include "ntgdi/NtGdiSelectFont.c"
#include "ntgdi/NtGdiSelectPen.c"
#include "ntgdi/NtGdiSetBitmapBits.c"
//#include "ntgdi/NtGdiSTROBJ_vEnumStart.c"
#include "ntgdi/NtGdiGetDIBits.c"

#include "ntuser/NtUserCountClipboardFormats.c"
#include "ntuser/NtUserFindExistingCursorIcon.c"
#include "ntuser/NtUserRedrawWindow.c"
#include "ntuser/NtUserScrollDC.c"
#include "ntuser/NtUserToUnicodeEx.c"

/* The List of tests */
TESTENTRY TestList[] =
{
	/* DirectDraw */
	{ L"NtGdiDdCreateDirectDrawObject", Test_NtGdiDdCreateDirectDrawObject },
	{ L"NtGdiDdQueryDirectDrawObject", Test_NtGdiDdQueryDirectDrawObject },
	{ L"NtGdiDdDeleteDirectDrawObject", Test_NtGdiDdDeleteDirectDrawObject },

	/* ntgdi */
	{ L"NtGdiArcInternal", Test_NtGdiArcInternal },
	{ L"NtGdiBitBlt", Test_NtGdiBitBlt },
	{ L"NtGdiCreateBitmap", Test_NtGdiCreateBitmap },
	{ L"NtGdiCreateCompatibleBitmap", Test_NtGdiCreateCompatibleBitmap },
	{ L"NtGdiDoPalette", Test_NtGdiDoPalette },
	{ L"NtGdiEngCreatePalette", Test_NtGdiEngCreatePalette },
//	{ L"NtGdiEnumFontChunk", Test_NtGdiEnumFontChunk },
	{ L"NtGdiEnumFontOpen", Test_NtGdiEnumFontOpen },
	{ L"NtGdiGetBitmapBits", Test_NtGdiGetBitmapBits },
	{ L"NtGdiGetRandomRgn", Test_NtGdiGetRandomRgn },
	{ L"NtGdiSetBitmapBits", Test_NtGdiSetBitmapBits },
	{ L"NtGdiSelectBitmap", Test_NtGdiSelectBitmap },
	{ L"NtGdiSelectBrush", Test_NtGdiSelectBrush },
	{ L"NtGdiSelectFont", Test_NtGdiSelectFont },
	{ L"NtGdiSelectPen", Test_NtGdiSelectPen },
//	{ L"NtGdiSTROBJ_vEnumStart", Test_NtGdiSTROBJ_vEnumStart },
	{ L"NtGdiGetDIBitsInternal", Test_NtGdiGetDIBitsInternal },

	/* ntuser */
	{ L"NtUserCountClipboardFormats", Test_NtUserCountClipboardFormats },
	{ L"NtUserFindExistingCursorIcon", Test_NtUserFindExistingCursoricon },
	{ L"NtUserRedrawWindow", Test_NtUserRedrawWindow },
	{ L"NtUserScrollDC", Test_NtUserScrollDC },
	{ L"NtUserToUnicodeEx", Test_NtUserToUnicodeEx }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

