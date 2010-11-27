
set_unicode()

spec2def(ntlanman.dll ntlanman.spec)

add_library(ntlanman SHARED ntlanman.c ntlanman.rc ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def)

set_module_type(ntlanman win32dll)

target_link_libraries(ntlanman wine)

add_importlibs(ntlanman netapi32 msvcrt kernel32 ntdll)

add_cab_target(ntlanman 1)
