
add_executable(oleacc_winetest main.c testlist.c)
set_module_type(oleacc_winetest win32cui)
add_importlibs(oleacc_winetest oleacc msvcrt kernel32)
add_cd_file(TARGET oleacc_winetest DESTINATION reactos/bin FOR all)
