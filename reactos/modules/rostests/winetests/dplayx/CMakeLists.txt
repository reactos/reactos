
add_executable(dplayx_winetest dplayx.c testlist.c)
set_module_type(dplayx_winetest win32cui)
add_importlibs(dplayx_winetest dplayx ole32 oleaut32 version advapi32 msvcrt kernel32)
add_rostests_file(TARGET dplayx_winetest)
