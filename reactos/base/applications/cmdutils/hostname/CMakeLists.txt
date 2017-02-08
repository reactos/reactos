
add_executable(hostname hostname.c hostname.rc)
set_module_type(hostname win32cui UNICODE)
add_importlibs(hostname user32 msvcrt kernel32)
add_cd_file(TARGET hostname DESTINATION reactos/system32 FOR all)
