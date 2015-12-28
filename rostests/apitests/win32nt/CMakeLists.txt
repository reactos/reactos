
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/include ../gditools)

list(APPEND SOURCE
    ntdd/NtGdiDdCreateDirectDrawObject.c
    ntdd/NtGdiDdDeleteDirectDrawObject.c
    ntdd/NtGdiDdQueryDirectDrawObject.c

    ntgdi/NtGdiArcInternal.c
    ntgdi/NtGdiBitBlt.c
    ntgdi/NtGdiCombineRgn.c
    ntgdi/NtGdiCreateBitmap.c
    ntgdi/NtGdiCreateCompatibleBitmap.c
    ntgdi/NtGdiCreateCompatibleDC.c
    ntgdi/NtGdiCreateDIBSection.c
    ntgdi/NtGdiDeleteObjectApp.c
    ntgdi/NtGdiDoPalette.c
    ntgdi/NtGdiEngCreatePalette.c
    ntgdi/NtGdiEnumFontOpen.c
    #ntgdi/NtGdiExtSelectClipRgn.c
    ntgdi/NtGdiExtTextOutW.c
    #ntgdi/NtGdiFlushUserBatch.c
    ntgdi/NtGdiGetBitmapBits.c
    ntgdi/NtGdiGetDIBits.c
    ntgdi/NtGdiGetFontResourceInfoInternalW.c
    ntgdi/NtGdiGetRandomRgn.c
    ntgdi/NtGdiGetStockObject.c
    ntgdi/NtGdiPolyPolyDraw.c
    ntgdi/NtGdiRestoreDC.c
    ntgdi/NtGdiSaveDC.c
    ntgdi/NtGdiSelectBitmap.c
    ntgdi/NtGdiSelectBrush.c
    ntgdi/NtGdiSelectFont.c
    ntgdi/NtGdiSelectPen.c
    ntgdi/NtGdiSetBitmapBits.c
    ntgdi/NtGdiSetDIBitsToDeviceInternal.c

#    ntuser/NtUserCallHwnd.c
#    ntuser/NtUserCallHwndLock.c
#    ntuser/NtUserCallHwndOpt.c
#    ntuser/NtUserCallHwndParam.c
#    ntuser/NtUserCallHwndParamLock.c
#    ntuser/NtUserCallNoParam.c
#    ntuser/NtUserCallOneParam.c
    ntuser/NtUserCountClipboardFormats.c
#    ntuser/NtUserEnumDisplayMonitors.c
    ntuser/NtUserEnumDisplaySettings.c
    ntuser/NtUserFindExistingCursorIcon.c
    ntuser/NtUserGetClassInfo.c
#    ntuser/NtUserGetIconInfo.c
    ntuser/NtUserGetTitleBarInfo.c
    ntuser/NtUserProcessConnect.c
    ntuser/NtUserRedrawWindow.c
    ntuser/NtUserScrollDC.c
    ntuser/NtUserSelectPalette.c
    ntuser/NtUserSetTimer.c
    ntuser/NtUserSystemParametersInfo.c
    ntuser/NtUserToUnicodeEx.c
    ntuser/NtUserUpdatePerUserSystemParameters.c

    #osver.c
    testlist.c
    w32knapi.rc)

add_executable(win32knt_apitest ${SOURCE})
target_link_libraries(win32knt_apitest ${PSEH_LIB} gditools)
set_module_type(win32knt_apitest win32cui)
add_importlibs(win32knt_apitest
    win32kdll # win32kdll_2ksp4 win32kdll_xpsp2 win32kdll_2k3sp2 win32kdll_vista
    gdi32
    user32
    shell32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET win32knt_apitest DESTINATION reactos/bin FOR all)
