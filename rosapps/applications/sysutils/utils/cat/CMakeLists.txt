
add_executable(cat cat.c)
set_module_type(cat win32cui)
add_importlibs(cat ntdll user32 msvcrt kernel32)
add_cd_file(TARGET cat DESTINATION reactos/bin FOR all)
