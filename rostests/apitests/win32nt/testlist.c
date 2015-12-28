#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_NtGdiDdCreateDirectDrawObject(void);
extern void func_NtGdiDdCreateDirectDrawObject(void);
extern void func_NtGdiDdDeleteDirectDrawObject(void);
extern void func_NtGdiDdQueryDirectDrawObject(void);

extern void func_NtGdiArcInternal(void);
extern void func_NtGdiBitBlt(void);
extern void func_NtGdiCombineRgn(void);
extern void func_NtGdiCreateBitmap(void);
extern void func_NtGdiCreateCompatibleBitmap(void);
extern void func_NtGdiCreateCompatibleDC(void);
extern void func_NtGdiCreateDIBSection(void);
extern void func_NtGdiDeleteObjectApp(void);
extern void func_NtGdiDoPalette(void);
extern void func_NtGdiEngCreatePalette(void);
extern void func_NtGdiEnumFontOpen(void);
//extern void func_NtGdiExtSelectClipRgn(void);
extern void func_NtGdiExtTextOutW(void);
//extern void func_NtGdiFlushUserBatch(void);
extern void func_NtGdiGetBitmapBits(void);
extern void func_NtGdiGetDIBitsInternal(void);
extern void func_NtGdiGetFontResourceInfoInternalW(void);
extern void func_NtGdiGetRandomRgn(void);
extern void func_NtGdiGetStockObject(void);
extern void func_NtGdiPolyPolyDraw(void);
extern void func_NtGdiRestoreDC(void);
extern void func_NtGdiSaveDC(void);
extern void func_NtGdiSelectBitmap(void);
extern void func_NtGdiSelectBrush(void);
extern void func_NtGdiSelectFont(void);
extern void func_NtGdiSelectPen(void);
extern void func_NtGdiSetBitmapBits(void);
extern void func_NtGdiSetDIBitsToDeviceInternal(void);
//extern void func_NtUserCallHwnd(void);
//extern void func_NtUserCallHwndLock(void);
//extern void func_NtUserCallHwndOpt(void);
//extern void func_NtUserCallHwndParam(void);
//extern void func_NtUserCallHwndParamLock(void);
//extern void func_NtUserCallNoParam(void);
//extern void func_NtUserCallOneParam(void);
extern void func_NtUserCountClipboardFormats(void);
//extern void func_NtUserEnumDisplayMonitors(void);
extern void func_NtUserEnumDisplaySettings(void);
extern void func_NtUserFindExistingCursorIcon(void);
extern void func_NtUserGetClassInfo(void);
//extern void func_NtUserGetIconInfo(void);
extern void func_NtUserGetTitleBarInfo(void);
extern void func_NtUserProcessConnect(void);
extern void func_NtUserRedrawWindow(void);
extern void func_NtUserScrollDC(void);
extern void func_NtUserSelectPalette(void);
extern void func_NtUserSetTimer(void);
extern void func_NtUserSystemParametersInfo(void);
extern void func_NtUserToUnicodeEx(void);
extern void func_NtUserUpdatePerUserSystemParameters(void);

const struct test winetest_testlist[] =
{
    /* ntdd*/
    { "NtGdiDdCreateDirectDrawObject", func_NtGdiDdCreateDirectDrawObject },
    { "NtGdiDdDeleteDirectDrawObject", func_NtGdiDdDeleteDirectDrawObject },
    { "NtGdiDdQueryDirectDrawObject", func_NtGdiDdQueryDirectDrawObject },
    { "NtGdiArcInternal", func_NtGdiArcInternal },

    /* ntgdi */
    { "NtGdiBitBlt", func_NtGdiBitBlt },
    { "NtGdiCombineRgn", func_NtGdiCombineRgn },
    { "NtGdiCreateBitmap", func_NtGdiCreateBitmap },
    { "NtGdiCreateCompatibleBitmap", func_NtGdiCreateCompatibleBitmap },
    { "NtGdiCreateCompatibleDC", func_NtGdiCreateCompatibleDC },
    { "NtGdiCreateDIBSection", func_NtGdiCreateDIBSection },
    { "NtGdiDeleteObjectApp", func_NtGdiDeleteObjectApp },
    { "NtGdiDoPalette", func_NtGdiDoPalette },
    { "NtGdiEngCreatePalette", func_NtGdiEngCreatePalette },
    { "NtGdiEnumFontOpen", func_NtGdiEnumFontOpen },
    //{ "NtGdiExtSelectClipRgn", func_NtGdiExtSelectClipRgn },
    { "NtGdiExtTextOutW", func_NtGdiExtTextOutW },
    //{ "NtGdiFlushUserBatch", func_NtGdiFlushUserBatch },
    { "NtGdiGetBitmapBits", func_NtGdiGetBitmapBits },
    { "NtGdiGetDIBitsInternal", func_NtGdiGetDIBitsInternal },
    { "NtGdiGetFontResourceInfoInternalW", func_NtGdiGetFontResourceInfoInternalW },
    { "NtGdiGetRandomRgn", func_NtGdiGetRandomRgn },
    { "NtGdiGetStockObject", func_NtGdiGetStockObject },
    { "NtGdiPolyPolyDraw", func_NtGdiPolyPolyDraw },
    { "NtGdiRestoreDC", func_NtGdiRestoreDC },
    { "NtGdiSaveDC", func_NtGdiSaveDC },
    { "NtGdiSelectBitmap", func_NtGdiSelectBitmap },
    { "NtGdiSelectBrush", func_NtGdiSelectBrush },
    { "NtGdiSelectFont", func_NtGdiSelectFont },
    { "NtGdiSelectPen", func_NtGdiSelectPen },
    { "NtGdiSetBitmapBits", func_NtGdiSetBitmapBits },
    { "NtGdiSetDIBitsToDeviceInternal", func_NtGdiSetDIBitsToDeviceInternal },

    /* ntuser */
    //{ "NtUserCallHwnd", func_NtUserCallHwnd },
    //{ "NtUserCallHwndLock", func_NtUserCallHwndLock },
    //{ "NtUserCallHwndOpt", func_NtUserCallHwndOpt },
    //{ "NtUserCallHwndParam", func_NtUserCallHwndParam },
    //{ "NtUserCallHwndParamLock", func_NtUserCallHwndParamLock },
    //{ "NtUserCallNoParam", func_NtUserCallNoParam },
    //{ "NtUserCallOneParam", func_NtUserCallOneParam },
    { "NtUserCountClipboardFormats", func_NtUserCountClipboardFormats },
    //{ "NtUserEnumDisplayMonitors", func_NtUserEnumDisplayMonitors },
    { "NtUserEnumDisplaySettings", func_NtUserEnumDisplaySettings },
    { "NtUserFindExistingCursorIcon", func_NtUserFindExistingCursorIcon },
    { "NtUserGetClassInfo", func_NtUserGetClassInfo },
    //{ "NtUserGetIconInfo", func_NtUserGetIconInfo },
    { "NtUserGetTitleBarInfo", func_NtUserGetTitleBarInfo },
    { "NtUserProcessConnect", func_NtUserProcessConnect },
    { "NtUserRedrawWindow", func_NtUserRedrawWindow },
    { "NtUserScrollDC", func_NtUserScrollDC },
    { "NtUserSelectPalette", func_NtUserSelectPalette },
    { "NtUserSetTimer", func_NtUserSetTimer },
    { "NtUserSystemParametersInfo", func_NtUserSystemParametersInfo },
    { "NtUserToUnicodeEx", func_NtUserToUnicodeEx },
    { "NtUserUpdatePerUserSystemParameters", func_NtUserUpdatePerUserSystemParameters },

    { 0, 0 }
};
