add_executable(np_enum np_enum.c)
set_module_type(np_enum win32cui UNICODE)
add_importlibs(np_enum msvcrt kernel32 mpr)
add_rostests_file(TARGET np_enum)
