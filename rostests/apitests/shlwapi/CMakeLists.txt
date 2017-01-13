
add_executable(shlwapi_apitest PathIsUNC.c testlist.c)
set_module_type(shlwapi_apitest win32cui)
add_importlibs(shlwapi_apitest msvcrt kernel32)
add_cd_file(TARGET shlwapi_apitest DESTINATION reactos/bin FOR all)
