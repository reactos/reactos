
set_unicode()

add_executable(route route.c route.rc)

set_module_type(route win32cui)
add_importlibs(route ws2_32 iphlpapi msvcrt)
