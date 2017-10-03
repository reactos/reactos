add_executable(parttest parttest.c)
set_module_type(parttest win32cui)
add_importlibs(parttest ntdll msvcrt kernel32)
add_rostests_file(TARGET parttest)
