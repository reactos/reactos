
add_executable(useridletime UserIdleTime.c)
set_module_type(useridletime win32cui UNICODE)
add_importlibs(useridletime user32 msvcrt kernel32)
add_rostests_file(TARGET useridletime SUBDIR suppl)
