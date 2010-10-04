
include_directories(.)
set_rc_compiler()

add_executable(format format.c format.rc)

set_module_type(format win32cui)

add_importlibs(format ntdll user32 fmifs msvcrt)
