
add_executable(stats stats.c)
set_module_type(stats win32cui UNICODE)
add_importlibs(stats ntdll msvcrt kernel32)
add_cd_file(TARGET stats DESTINATION reactos/bin FOR all)
