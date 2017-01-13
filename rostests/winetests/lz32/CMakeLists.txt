
add_definitions(-DUSE_WINE_TODOS)
add_executable(lz32_winetest lzexpand_main.c testlist.c)
set_module_type(lz32_winetest win32cui)
add_importlibs(lz32_winetest lz32 msvcrt kernel32)
add_rostests_file(TARGET lz32_winetest)
