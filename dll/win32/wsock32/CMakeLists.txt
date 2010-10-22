
set_unicode()

add_library(wsock32 SHARED stubs.c wsock32.rc)

set_module_type(wsock32 win32dll)

target_link_libraries(wsock32 ${CMAKE_CURRENT_SOURCE_DIR}/wsock32.def)
add_importlibs(wsock32 ws2_32 kernel32 ntdll)
add_livecd_target(wsock32 reactos/system32)
