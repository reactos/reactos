
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mciavi32.dll mciavi32.spec)

list(APPEND SOURCE
    info.c
    mciavi.c
    mmoutput.c
    wnd.c
    precomp.h)

add_library(mciavi32 MODULE
    ${SOURCE}
    mciavi_res.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mciavi32.def)

set_module_type(mciavi32 win32dll)
target_link_libraries(mciavi32 wine)
add_importlibs(mciavi32 msvfw32 winmm user32 gdi32 msvcrt kernel32 ntdll)
add_pch(mciavi32 precomp.h SOURCE)
add_cd_file(TARGET mciavi32 DESTINATION reactos/system32 FOR all)
