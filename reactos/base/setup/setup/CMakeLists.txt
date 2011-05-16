
set_unicode()

add_executable(setup setup.c setup.rc)

set_module_type(setup win32gui)
add_importlibs(setup userenv msvcrt kernel32 ntdll)
add_cd_file(TARGET setup DESTINATION reactos/system32 FOR all)
