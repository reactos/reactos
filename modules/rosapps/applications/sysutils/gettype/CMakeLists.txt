
add_executable(gettype gettype.c)
set_module_type(gettype win32cui UNICODE)
add_importlibs(gettype shell32 mpr netapi32 msvcrt kernel32)
add_cd_file(TARGET gettype DESTINATION reactos/system32 FOR all)
