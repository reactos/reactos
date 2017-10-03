
add_executable(objdir objdir.c)
set_module_type(objdir win32cui)
add_importlibs(objdir ntdll msvcrt kernel32)
add_cd_file(TARGET objdir DESTINATION reactos/bin FOR all)
