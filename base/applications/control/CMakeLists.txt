
set_unicode()

add_executable(control control.c control.rc)

set_module_type(control win32gui)
add_importlibs(control advapi32 shell32 msvcrt)
