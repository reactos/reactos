
add_executable(wlanapi_apitest wlanapi.c testlist.c)
set_module_type(wlanapi_apitest win32cui)
add_importlibs(wlanapi_apitest wlanapi msvcrt kernel32)
add_rostests_file(TARGET wlanapi_apitest)
