
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(iccvid.dll iccvid.spec)

add_library(iccvid SHARED iccvid.c rsrc.rc ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

set_module_type(iccvid win32dll)

target_link_libraries(iccvid wine)

add_importlibs(iccvid user32 kernel32 ntdll)

add_cab_target(iccvid 1)
