
add_executable(cryptnet_winetest cryptnet.c testlist.c)
set_module_type(cryptnet_winetest win32cui)
add_importlibs(cryptnet_winetest cryptnet crypt32 msvcrt kernel32)
add_rostests_file(TARGET cryptnet_winetest)
