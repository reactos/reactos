
add_executable(mode mode.c mode.rc)
set_module_type(mode win32cui UNICODE)
set_target_properties(mode PROPERTIES SUFFIX ".com")
add_importlibs(mode user32 msvcrt kernel32)
add_cd_file(TARGET mode DESTINATION reactos/system32 FOR all)
