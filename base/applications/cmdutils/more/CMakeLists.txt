
add_executable(more more.c more.rc)
set_module_type(more win32cui)
set_target_properties(more PROPERTIES SUFFIX ".com")
add_importlibs(more user32 msvcrt kernel32)
add_cd_file(TARGET more DESTINATION reactos/system32 FOR all)
