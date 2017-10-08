
add_executable(y y.c y.rc)
set_module_type(y win32cui)
add_importlibs(y msvcrt kernel32)
add_cd_file(TARGET y DESTINATION reactos/system32 FOR all)
