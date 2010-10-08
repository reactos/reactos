
add_executable(hostname hostname.c hostname.rc)

set_module_type(hostname win32cui)
add_importlibs(hostname msvcrt kernel32)
