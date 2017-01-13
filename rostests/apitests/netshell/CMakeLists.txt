
add_executable(netshell_apitest NcIsValidConnectionName.c testlist.c)
set_module_type(netshell_apitest win32cui)
add_importlibs(netshell_apitest msvcrt kernel32)
add_rostests_file(TARGET netshell_apitest)
