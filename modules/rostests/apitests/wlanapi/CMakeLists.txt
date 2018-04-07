
add_executable(wlanapi_apitest wlanapi.c testlist.c)
set_module_type(wlanapi_apitest win32cui)
# wlanapi is available on WXPsp3 and WVista (and ReactOS), but not on WS03.
add_delay_importlibs(wlanapi_apitest wlanapi)
add_importlibs(wlanapi_apitest msvcrt kernel32)
add_rostests_file(TARGET wlanapi_apitest)
