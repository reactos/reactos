
add_executable(sort sort.c sort.rc)
set_module_type(sort win32cui)
add_importlibs(sort msvcrt kernel32)
add_cd_file(TARGET sort DESTINATION reactos/system32 FOR all)
