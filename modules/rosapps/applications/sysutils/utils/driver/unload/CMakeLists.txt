
add_executable(unload unload.c)
set_module_type(unload win32cui UNICODE)
add_importlibs(unload ntdll user32 msvcrt kernel32)
add_cd_file(TARGET unload DESTINATION reactos/bin FOR all)
