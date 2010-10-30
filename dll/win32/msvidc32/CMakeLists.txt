
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

spec2def(msvidc32.dll msvidc32.spec)

add_library(msvidc32 SHARED msvideo1.c rsrc.rc ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def)

set_module_type(msvidc32 win32dll)

target_link_libraries(msvidc32 wine)

add_importlibs(msvidc32 user32 kernel32 ntdll)

add_cab_target(msvidc32 1)
