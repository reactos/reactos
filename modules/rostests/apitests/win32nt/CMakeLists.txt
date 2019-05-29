
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
    ntgdi/NtGdiExtSelectClipRgn.c
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
    ntgdi/NtGdiTransformPoints.c

#    ntuser/NtUserCallHwnd.c
#    ntuser/NtUserCallHwndLock.c
#    ntuser/NtUserCallHwndOpt.c
#    ntuser/NtUserCallHwndParam.c
#    ntuser/NtUserCallHwndParamLock.c
#    ntuser/NtUserCallNoParam.c
#    ntuser/NtUserCallOneParam.c
    ntuser/NtUserCountClipboardFormats.c
    ntuser/NtUserCreateWindowEx.c
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
    win32nt.h)

add_library(win32knt_static STATIC ${SOURCE} w32knapi.rc)
add_dependencies(win32knt_static xdk)
add_pch(win32knt_static win32nt.h SOURCE)

set(WIN32KNT_IMPORTLIBS gdi32 user32 shell32 advapi32 msvcrt kernel32 ntdll)

if(1)  # ros
    add_executable(win32knt_apitest testlist.c)
    target_link_libraries(win32knt_apitest ${PSEH_LIB} win32knt_static gditools)
    set_module_type(win32knt_apitest win32cui)
    add_importlibs(win32knt_apitest win32u ${WIN32KNT_IMPORTLIBS})

    add_rostests_file(TARGET win32knt_apitest)
endif()

if(0)  # Specify 1 if you want 2ksp4 version
    # See also ../win32u/CMakeLists.txt
    add_executable(win32knt_2ksp4_apitest testlist.c)
    target_link_libraries(win32knt_2ksp4_apitest ${PSEH_LIB} win32knt_static gditools)
    set_module_type(win32knt_2ksp4_apitest win32cui)
    add_importlibs(win32knt_2ksp4_apitest win32u_2ksp4 ${WIN32KNT_IMPORTLIBS})
endif()

if(1) # xpsp2
    add_executable(win32knt_xpsp2_apitest testlist.c)
    target_link_libraries(win32knt_xpsp2_apitest ${PSEH_LIB} win32knt_static gditools)
    set_module_type(win32knt_xpsp2_apitest win32cui)
    add_importlibs(win32knt_xpsp2_apitest win32u_xpsp2 ${WIN32KNT_IMPORTLIBS})
endif()

if(1) # 2k3sp2
    add_executable(win32knt_2k3sp2_apitest testlist.c)
    target_link_libraries(win32knt_2k3sp2_apitest ${PSEH_LIB} win32knt_static gditools)
    set_module_type(win32knt_2k3sp2_apitest win32cui)
    add_importlibs(win32knt_2k3sp2_apitest win32u_2k3sp2 ${WIN32KNT_IMPORTLIBS})
endif()

if(0)  # Specify 1 if you want vista version
    # See also ../win32u/CMakeLists.txt
    add_executable(win32knt_vista_apitest testlist.c)
    target_link_libraries(win32knt_vista_apitest ${PSEH_LIB} win32knt_static gditools)
    set_module_type(win32knt_vista_apitest win32cui)
    add_importlibs(win32knt_vista_apitest win32u_vista ${WIN32KNT_IMPORTLIBS})
endif()
