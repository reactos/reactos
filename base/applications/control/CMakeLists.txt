
add_executable(control
    control.c
    control.rc)

set_unicode(control yes)

set_module_type(control win32gui)

add_importlibs(control advapi32 shell32 kernel32 msvcrt)