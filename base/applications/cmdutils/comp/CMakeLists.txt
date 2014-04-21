
add_executable(comp comp.c comp.rc)
set_module_type(comp win32cui)
add_importlibs(comp msvcrt kernel32)
add_cd_file(TARGET comp DESTINATION reactos/system32 FOR all)
