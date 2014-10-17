
add_executable(wlanapi_winetest wlanapi.c testlist.c)
set_module_type(wlanapi_winetest win32cui)
add_importlibs(wlanapi_winetest wlanapi msvcrt kernel32)
add_cd_file(TARGET wlanapi_winetest DESTINATION reactos/bin FOR all)
