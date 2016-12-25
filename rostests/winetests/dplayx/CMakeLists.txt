
add_executable(dplayx_winetest dplayx.c testlist.c)
set_module_type(dplayx_winetest win32cui)
add_importlibs(dplayx_winetest dplayx ole32 msvcrt kernel32)
add_cd_file(TARGET dplayx_winetest DESTINATION reactos/bin FOR all)
