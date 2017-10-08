
add_executable(pefile_apitest ntoskrnl.c testlist.c)
set_module_type(pefile_apitest win32cui)
add_importlibs(pefile_apitest msvcrt kernel32)
add_rostests_file(TARGET pefile_apitest)
