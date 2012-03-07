
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(comdlg32.dll comdlg32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cdlg32.c
    colordlg.c
    filedlg.c
    filedlg31.c
    filedlgbrowser.c
    finddlg.c
    fontdlg.c
    itemdlg.c
    printdlg.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comdlg32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/comdlg32.def)

add_library(comdlg32 SHARED ${SOURCE})
set_module_type(comdlg32 win32dll)
target_link_libraries(comdlg32 uuid wine)
add_delay_importlibs(comdlg32 ole32)

add_importlibs(comdlg32
    shell32
    shlwapi
    comctl32
    winspool
    user32
    gdi32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET comdlg32 DESTINATION reactos/system32 FOR all)

