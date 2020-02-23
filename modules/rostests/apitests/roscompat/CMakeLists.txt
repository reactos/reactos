
add_executable(roscompat_apitest export_rewriting.c testlist.c)
set_module_type(roscompat_apitest win32cui)
add_importlibs(roscompat_apitest msvcrt kernel32)
add_rostests_file(TARGET roscompat_apitest)
