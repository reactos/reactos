
add_executable(more more.c more.rc)

set_module_type(more win32cui)
add_importlibs(more user32 msvcrt)
