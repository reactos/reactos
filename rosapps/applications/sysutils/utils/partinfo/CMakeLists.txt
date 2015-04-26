
add_executable(partinfo partinfo.c)
set_module_type(partinfo win32cui)
add_importlibs(partinfo ntdll msvcrt kernel32)
add_cd_file(TARGET partinfo DESTINATION reactos/bin FOR all)
