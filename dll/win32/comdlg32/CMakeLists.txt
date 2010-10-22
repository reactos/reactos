
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(comdlg32 ${CMAKE_CURRENT_SOURCE_DIR}/comdlg32.spec ${CMAKE_CURRENT_BINARY_DIR}/comdlg32.def)

add_library(comdlg32 SHARED
    cdlg32.c
    colordlg.c
    filedlg.c
    filedlg31.c
    filedlgbrowser.c
    finddlg.c
    fontdlg.c
    printdlg.c
    rsrc.rc)

set_module_type(comdlg32 win32dll)

target_link_libraries(comdlg32
    ${CMAKE_CURRENT_BINARY_DIR}/comdlg32.def
    uuid
    wine)

add_importlibs(comdlg32
    shell32
    shlwapi
    comctl32
    winspool
    user32
    gdi32
    advapi32
    ole32
    msvcrt
    kernel32
    ntdll)

add_dependencies(comdlg32 comdlg32_def)
add_livecd_target(comdlg32 reactos/system32)
