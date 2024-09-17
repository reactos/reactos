
add_executable(roscompat_apitest roscompat.c testlist.c)
set_module_type(roscompat_apitest win32cui)
add_importlibs(roscompat_apitest msvcrt advapi32 kernel32)
add_rostests_file(TARGET roscompat_apitest)
