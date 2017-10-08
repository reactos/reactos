
add_executable(snmpapi_winetest util.c testlist.c)
set_module_type(snmpapi_winetest win32cui)
add_importlibs(snmpapi_winetest snmpapi msvcrt kernel32)
add_rostests_file(TARGET snmpapi_winetest)
