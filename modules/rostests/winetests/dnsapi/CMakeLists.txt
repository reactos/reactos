
add_executable(dnsapi_winetest name.c record.c testlist.c)
set_module_type(dnsapi_winetest win32cui)
add_importlibs(dnsapi_winetest dnsapi msvcrt kernel32)
add_rostests_file(TARGET dnsapi_winetest)
