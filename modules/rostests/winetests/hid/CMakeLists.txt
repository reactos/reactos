
add_executable(hid_winetest device.c testlist.c)
set_module_type(hid_winetest win32cui)
add_importlibs(hid_winetest hid setupapi msvcrt kernel32)
add_rostests_file(TARGET hid_winetest)
