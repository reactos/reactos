
set_unicode()

spec2def(ntlanman ${CMAKE_CURRENT_SOURCE_DIR}/ntlanman.spec ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def)

add_library(ntlanman SHARED ntlanman.c ntlanman.rc)

set_module_type(ntlanman win32dll)

target_link_libraries(ntlanman
    ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def
    wine)

add_importlibs(ntlanman netapi32 kernel32 ntdll)
add_dependencies(ntlanman ntlanman_def)
add_livecd_target(ntlanman reactos/system32)
