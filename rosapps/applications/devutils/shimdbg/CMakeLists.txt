
add_executable(shimdbg shimdbg.c)

set_module_type(shimdbg win32cui)
add_importlibs(shimdbg ntdll msvcrt kernel32)
add_cd_file(TARGET shimdbg DESTINATION reactos/system32 FOR all)
