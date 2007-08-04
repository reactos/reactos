#include "../apitest.h"

/* include the tests */

#include "ntdd/NtGdiDdCreateDirectDrawObject.c"
#include "ntdd/NtGdiDdDeleteDirectDrawObject.c"
#include "ntdd/NtGdiDdQueryDirectDrawObject.c"

#include "ntgdi/NtGdiArcInternal.c"
#include "ntgdi/NtGdiSetBitmapBits.c"
#include "ntgdi/NtGdiGetBitmapBits.c"

#include "ntuser/NtUserCountClipboardFormats.c"

/* The List of tests */
TESTENTRY TestList[] =
{
    /* DirectDraw */
	{ L"NtGdiDdCreateDirectDrawObject", Test_NtGdiDdCreateDirectDrawObject },
	{ L"NtGdiDdDeleteDirectDrawObject", Test_NtGdiDdDeleteDirectDrawObject },
	{ L"NtGdiDdQueryDirectDrawObject", Test_NtGdiDdQueryDirectDrawObject },

    /* ntgdi */
	{ L"NtGdiArcInternal", Test_NtGdiArcInternal },
	{ L"NtGdiGetBitmapBits", Test_NtGdiGetBitmapBits },
	{ L"NtGdiSetBitmapBits", Test_NtGdiSetBitmapBits },

    /* ntuser */
	{ L"NtUserCountClipboardFormats", Test_NtUserCountClipboardFormats }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

