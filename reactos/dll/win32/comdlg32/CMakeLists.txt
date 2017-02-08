
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(comdlg32.dll comdlg32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cdlg32.c
    colordlg.c
    filedlg.c
    filedlg31.c
    filedlgbrowser.c
    finddlg.c
    fontdlg.c
    #itemdlg.c /* Win 7 */
    printdlg.c
    cdlg.h
    ${CMAKE_CURRENT_BINARY_DIR}/comdlg32_stubs.c)

add_library(comdlg32 SHARED
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comdlg32.def)

set_module_type(comdlg32 win32dll)
target_link_libraries(comdlg32 uuid wine)
add_delay_importlibs(comdlg32 ole32)
add_importlibs(comdlg32 shell32 shlwapi comctl32 winspool user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(comdlg32 cdlg.h SOURCE)
add_cd_file(TARGET comdlg32 DESTINATION reactos/system32 FOR all)
