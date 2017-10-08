
add_definitions(-DUSE_WINE_TODOS)
add_executable(sxs_winetest cache.c name.c testlist.c)
set_module_type(sxs_winetest win32cui)
add_importlibs(sxs_winetest sxs msvcrt kernel32)
add_rostests_file(TARGET sxs_winetest)
