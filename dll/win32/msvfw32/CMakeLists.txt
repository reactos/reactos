
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

spec2def(msvfw32 ${CMAKE_CURRENT_SOURCE_DIR}/msvfw32.spec ${CMAKE_CURRENT_BINARY_DIR}/msvfw32.def)

add_library(msvfw32 SHARED
    mciwnd.c
    msvideo_main.c
    drawdib.c
    rsrc.rc)

set_module_type(msvfw32 win32dll)

target_link_libraries(msvfw32
    ${CMAKE_CURRENT_BINARY_DIR}/msvfw32.def
    wine)

add_importlibs(msvfw32 winmm version comctl32 user32 gdi32 advapi32 kernel32 ntdll)
add_dependencies(msvfw32 msvfw32_def)
add_cab_target(msvfw32 1)
