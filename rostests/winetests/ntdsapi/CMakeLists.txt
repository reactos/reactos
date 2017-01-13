
add_executable(ntdsapi_winetest ntdsapi.c testlist.c)
set_module_type(ntdsapi_winetest win32cui)
add_importlibs(ntdsapi_winetest ntdsapi msvcrt kernel32)
add_rostests_file(TARGET ntdsapi_winetest)
