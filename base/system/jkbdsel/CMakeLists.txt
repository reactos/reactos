add_executable(jkbdsel jkbdsel.c)
set_module_type(jkbdsel nativecui)
add_importlibs(jkbdsel ntdll)
add_cd_file(TARGET jkbdsel DESTINATION reactos/system32 FOR all)
