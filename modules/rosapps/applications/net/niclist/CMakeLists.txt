
add_executable(niclist niclist.c niclist.rc)
set_module_type(niclist win32cui)
add_importlibs(niclist user32 msvcrt kernel32)
add_cd_file(TARGET niclist DESTINATION reactos/system32 FOR all)
