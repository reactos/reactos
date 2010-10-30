
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciavi32.dll mciavi32.spec)

add_library(mciavi32 SHARED
    info.c
    mciavi.c
    mmoutput.c
    wnd.c
    mciavi_res.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mciavi32.def)

set_module_type(mciavi32 win32dll)

target_link_libraries(mciavi32 wine)

add_importlibs(mciavi32 msvfw32 winmm user32 gdi32 kernel32 ntdll)

add_cab_target(mciavi32 1)
