
add_executable(netcfgx_winetest netcfgx.c testlist.c)
target_link_libraries(netcfgx_winetest uuid)
set_module_type(netcfgx_winetest win32cui)
add_importlibs(netcfgx_winetest ole32 msvcrt kernel32)
add_rostests_file(TARGET netcfgx_winetest)
