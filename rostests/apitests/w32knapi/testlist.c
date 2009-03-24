#include "w32knapi.h"

/* include the tests */

#include "ntdd/NtGdiDdCreateDirectDrawObject.c"
#include "ntdd/NtGdiDdDeleteDirectDrawObject.c"
#include "ntdd/NtGdiDdQueryDirectDrawObject.c"

#include "ntgdi/NtGdiArcInternal.c"
#include "ntgdi/NtGdiBitBlt.c"
#include "ntgdi/NtGdiCombineRgn.c"
#include "ntgdi/NtGdiCreateBitmap.c"
#include "ntgdi/NtGdiCreateCompatibleBitmap.c"
#include "ntgdi/NtGdiCreateCompatibleDC.c"
#include "ntgdi/NtGdiCreateDIBSection.c"
#include "ntgdi/NtGdiDeleteObjectApp.c"
#include "ntgdi/NtGdiDoPalette.c"
#include "ntgdi/NtGdiEngCreatePalette.c"
//#include "ntgdi/NtGdiEnumFontChunk.c"
#include "ntgdi/NtGdiEnumFontOpen.c"
//#include "ntgdi/NtGdiExtCreatePen.c"
#include "ntgdi/NtGdiExtTextOutW.c"
#include "ntgdi/NtGdiFlushUserBatch.c"
#include "ntgdi/NtGdiGetBitmapBits.c"
#include "ntgdi/NtGdiGetFontResourceInfoInternalW.c"
#include "ntgdi/NtGdiGetRandomRgn.c"
#include "ntgdi/NtGdiPolyPolyDraw.c"
#include "ntgdi/NtGdiSelectBitmap.c"
#include "ntgdi/NtGdiSelectBrush.c"
#include "ntgdi/NtGdiSelectFont.c"
#include "ntgdi/NtGdiSelectPen.c"
#include "ntgdi/NtGdiSetBitmapBits.c"
#include "ntgdi/NtGdiSetDIBitsToDeviceInternal.c"
//#include "ntgdi/NtGdiSTROBJ_vEnumStart.c"
#include "ntgdi/NtGdiGetDIBits.c"
#include "ntgdi/NtGdiGetStockObject.c"

#include "ntuser/NtUserCallHwnd.c"
#include "ntuser/NtUserCallHwndLock.c"
#include "ntuser/NtUserCallHwndOpt.c"
#include "ntuser/NtUserCallHwndParam.c"
#include "ntuser/NtUserCallHwndParamLock.c"
#include "ntuser/NtUserCallNoParam.c"
#include "ntuser/NtUserCallOneParam.c"
#include "ntuser/NtUserCountClipboardFormats.c"
//#include "ntuser/NtUserCreateWindowEx.c"
#include "ntuser/NtUserEnumDisplayMonitors.c"
#include "ntuser/NtUserEnumDisplaySettings.c"
#include "ntuser/NtUserFindExistingCursorIcon.c"
#include "ntuser/NtUserGetClassInfo.c"
#include "ntuser/NtUserGetTitleBarInfo.c"
#include "ntuser/NtUserProcessConnect.c"
#include "ntuser/NtUserRedrawWindow.c"
#include "ntuser/NtUserScrollDC.c"
#include "ntuser/NtUserSelectPalette.c"
#include "ntuser/NtUserSetTimer.c"
#include "ntuser/NtUserSystemParametersInfo.c"
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
	{ L"NtGdiCombineRgn", Test_NtGdiCombineRgn },
	{ L"NtGdiCreateBitmap", Test_NtGdiCreateBitmap },
	{ L"NtGdiCreateCompatibleBitmap", Test_NtGdiCreateCompatibleBitmap },
	{ L"NtGdiCreateCompatibleDC", Test_NtGdiCreateCompatibleDC },
	{ L"NtGdiCreateDIBSection", Test_NtGdiCreateDIBSection },
	{ L"NtGdiDeleteObjectApp", Test_NtGdiDeleteObjectApp },
	{ L"NtGdiDoPalette", Test_NtGdiDoPalette },
	{ L"NtGdiEngCreatePalette", Test_NtGdiEngCreatePalette },
//	{ L"NtGdiEnumFontChunk", Test_NtGdiEnumFontChunk },
	{ L"NtGdiEnumFontOpen", Test_NtGdiEnumFontOpen },
//	{ L"NtGdiExtCreatePen", Test_NtGdiExtCreatePen },
	{ L"NtGdiExtTextOutW", Test_NtGdiExtTextOutW },
	{ L"NtGdiFlushUserBatch", Test_NtGdiFlushUserBatch },
	{ L"NtGdiGetBitmapBits", Test_NtGdiGetBitmapBits },
	{ L"NtGdiGetFontResourceInfoInternalW", Test_NtGdiGetFontResourceInfoInternalW },
	{ L"NtGdiGetRandomRgn", Test_NtGdiGetRandomRgn },
	{ L"NtGdiPolyPolyDraw", Test_NtGdiPolyPolyDraw },
	{ L"NtGdiSetBitmapBits", Test_NtGdiSetBitmapBits },
	{ L"NtGdiSetDIBitsToDeviceInternal", Test_NtGdiSetDIBitsToDeviceInternal },
	{ L"NtGdiSelectBitmap", Test_NtGdiSelectBitmap },
	{ L"NtGdiSelectBrush", Test_NtGdiSelectBrush },
	{ L"NtGdiSelectFont", Test_NtGdiSelectFont },
	{ L"NtGdiSelectPen", Test_NtGdiSelectPen },
//	{ L"NtGdiSTROBJ_vEnumStart", Test_NtGdiSTROBJ_vEnumStart },
	{ L"NtGdiGetDIBitsInternal", Test_NtGdiGetDIBitsInternal },
	{ L"NtGdiGetStockObject", Test_NtGdiGetStockObject },

	/* ntuser */
	{ L"NtUserCallHwnd", Test_NtUserCallHwnd },
	{ L"NtUserCallHwndLock", Test_NtUserCallHwndLock },
	{ L"NtUserCallHwndOpt", Test_NtUserCallHwndOpt },
	{ L"NtUserCallHwndParam", Test_NtUserCallHwndParam },
	{ L"NtUserCallHwndParamLock", Test_NtUserCallHwndParamLock },
	{ L"NtUserCallNoParam", Test_NtUserCallNoParam },
	{ L"NtUserCallOneParam", Test_NtUserCallOneParam },
	{ L"NtUserCountClipboardFormats", Test_NtUserCountClipboardFormats },
//	{ L"NtUserCreateWindowEx", Test_NtUserCreateWindowEx },
	{ L"NtUserEnumDisplayMonitors", Test_NtUserEnumDisplayMonitors },
	{ L"NtUserEnumDisplaySettings", TEST_NtUserEnumDisplaySettings },
	{ L"NtUserFindExistingCursorIcon", Test_NtUserFindExistingCursoricon },
	{ L"NtUserGetClassInfo", Test_NtUserGetClassInfo },
	{ L"NtUserGetTitleBarInfo", Test_NtUserGetTitleBarInfo },
	{ L"NtUserProcessConnect", Test_NtUserProcessConnect },
	{ L"NtUserRedrawWindow", Test_NtUserRedrawWindow },
	{ L"NtUserScrollDC", Test_NtUserScrollDC },
	{ L"NtUserSelectPalette", Test_NtUserSelectPalette },
	{ L"NtUserSetTimer", Test_NtUserSetTimer },
	{ L"NtUserSystemParametersInfo", Test_NtUserSystemParametersInfo },
	{ L"NtUserToUnicodeEx", Test_NtUserToUnicodeEx },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}


