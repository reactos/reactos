
add_executable(apisets_apitest apisets.c testlist.c)
set_module_type(apisets_apitest win32cui)
target_link_libraries(apisets_apitest apisets)
add_importlibs(apisets_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET apisets_apitest)
