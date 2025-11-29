
add_executable(avifil32_winetest api.c testlist.c)
set_module_type(avifil32_winetest win32cui)
add_importlibs(avifil32_winetest avifil32 ole32 msvcrt kernel32)
add_rostests_file(TARGET avifil32_winetest)
