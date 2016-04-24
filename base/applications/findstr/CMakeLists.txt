
add_executable(findstr findstr.c findstr.rc)
set_module_type(findstr win32cui)
add_importlibs(findstr user32 msvcrt kernel32)
add_cd_file(TARGET findstr DESTINATION reactos/system32 FOR all)
