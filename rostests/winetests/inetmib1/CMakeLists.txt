
add_executable(inetmib1_winetest main.c testlist.c)
set_module_type(inetmib1_winetest win32cui)
add_importlibs(inetmib1_winetest snmpapi msvcrt kernel32)
add_cd_file(TARGET inetmib1_winetest DESTINATION reactos/bin FOR all)
