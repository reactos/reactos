
set_unicode()
set_rc_compiler()

add_executable(control control.c control.rc)

set_module_type(control win32gui)
add_importlibs(control advapi32 shell32 msvcrt kernel32)
add_cab_target(control 1)
