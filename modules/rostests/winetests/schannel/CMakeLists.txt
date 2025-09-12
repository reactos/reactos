
add_executable(schannel_winetest main.c testlist.c)
set_module_type(schannel_winetest win32cui)
add_importlibs(schannel_winetest msvcrt kernel32)
add_rostests_file(TARGET schannel_winetest)
