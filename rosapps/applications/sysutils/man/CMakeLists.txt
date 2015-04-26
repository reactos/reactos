
add_executable(man man.c)
set_module_type(man win32cui)
add_importlibs(man user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET man DESTINATION reactos/system32 FOR all)
