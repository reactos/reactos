add_executable(parttest parttest.c)
set_module_type(parttest win32cui)
add_importlibs(parttest ntdll msvcrt kernel32)
add_cd_file(TARGET parttest DESTINATION reactos/bin FOR all)
