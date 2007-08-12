#include "../apitest.h"

/* include the tests */

#include "ntdd/NtGdiDdCreateDirectDrawObject.c"
#include "ntdd/NtGdiDdDeleteDirectDrawObject.c"
#include "ntdd/NtGdiDdQueryDirectDrawObject.c"

#include "ntgdi/NtGdiArcInternal.c"
#include "ntgdi/NtGdiCreateBitmap.c"
#include "ntgdi/NtGdiCreateCompatibleBitmap.c"
//#include "ntgdi/NtGdiDoPalette.c"
#include "ntgdi/NtGdiGetBitmapBits.c"
#include "ntgdi/NtGdiGetRandomRgn.c"
#include "ntgdi/NtGdiSetBitmapBits.c"
//#include "ntgdi/NtGdiSTROBJ_vEnumStart.c"

#include "ntuser/NtUserCountClipboardFormats.c"

/* The List of tests */
TESTENTRY TestList[] =
{
	/* DirectDraw */
	{ L"NtGdiDdCreateDirectDrawObject", Test_NtGdiDdCreateDirectDrawObject },
	{ L"NtGdiDdQueryDirectDrawObject", Test_NtGdiDdQueryDirectDrawObject },
	{ L"NtGdiDdDeleteDirectDrawObject", Test_NtGdiDdDeleteDirectDrawObject },

	/* ntgdi */
	{ L"NtGdiArcInternal", Test_NtGdiArcInternal },
	{ L"NtGdiCreateBitmap", Test_NtGdiCreateBitmap },
	{ L"NtGdiCreateCompatibleBitmap", Test_NtGdiCreateCompatibleBitmap },
//	{ L"NtGdiDoPalette", Test_NtGdiDoPalette },
	{ L"NtGdiGetBitmapBits", Test_NtGdiGetBitmapBits },
	{ L"NtGdiGetRandomRgn", Test_NtGdiGetRandomRgn },
	{ L"NtGdiSetBitmapBits", Test_NtGdiSetBitmapBits },
//	{ L"NtGdiSTROBJ_vEnumStart", Test_NtGdiSTROBJ_vEnumStart },

	/* ntuser */
	{ L"NtUserCountClipboardFormats", Test_NtUserCountClipboardFormats }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

