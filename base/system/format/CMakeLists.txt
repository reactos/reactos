
add_executable(format format.c format.rc)
set_module_type(format win32cui)
set_target_properties(format PROPERTIES SUFFIX ".com")
add_importlibs(format user32 fmifs msvcrt kernel32 ntdll)
add_cd_file(TARGET format DESTINATION reactos/system32 FOR all)
