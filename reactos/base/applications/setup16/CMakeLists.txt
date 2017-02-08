
add_executable(setup16 main.c)
set_module_type(setup16 win32gui UNICODE)
add_importlibs(setup16 user32 gdi32 advapi32 shell32 setupapi msvcrt kernel32)
add_cd_file(TARGET setup16 DESTINATION reactos/system32 FOR all)
