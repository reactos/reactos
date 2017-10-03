
add_executable(nts2w32err nts2w32err.c)
set_module_type(nts2w32err win32cui)
add_importlibs(nts2w32err ntdll msvcrt kernel32)
add_cd_file(TARGET nts2w32err DESTINATION reactos/bin FOR all)
