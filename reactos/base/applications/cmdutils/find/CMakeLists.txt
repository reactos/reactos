
add_executable(find find.c find.rc)
set_module_type(find win32cui)
add_importlibs(find user32 msvcrt kernel32)
add_cd_file(TARGET find DESTINATION reactos/system32 FOR all)
