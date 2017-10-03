
add_executable(notevil notevil.c notevil.rc)
set_module_type(notevil win32cui UNICODE)
add_importlibs(notevil user32 msvcrt kernel32)
add_cd_file(TARGET notevil DESTINATION reactos/system32 FOR all)
