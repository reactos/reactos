
add_executable(msvfw32_winetest msvfw.c testlist.c)
set_module_type(msvfw32_winetest win32cui)
add_importlibs(msvfw32_winetest msvfw32 msvcrt kernel32)
add_cd_file(TARGET msvfw32_winetest DESTINATION reactos/bin FOR all)
