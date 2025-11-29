
add_definitions(-DUSE_WINE_TODOS)
add_executable(rsaenh_winetest rsaenh.c testlist.c)
set_module_type(rsaenh_winetest win32cui)
add_importlibs(rsaenh_winetest advapi32 msvcrt kernel32)
add_rostests_file(TARGET rsaenh_winetest)
