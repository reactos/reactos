
add_executable(tcat cat.c)
set_module_type(tcat win32cui)
add_importlibs(tcat user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET tcat DESTINATION reactos/system32 FOR all)
