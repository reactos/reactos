
set_rc_compiler()

add_executable(format format.c format.rc)

set_module_type(format win32cui)
add_importlibs(format user32 fmifs msvcrt kernel32 ntdll)
add_cd_file(TARGET format DESTINATION reactos/system32 FOR all)
