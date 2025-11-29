
add_executable(serialui_winetest confdlg.c testlist.c)
set_module_type(serialui_winetest win32cui)
add_importlibs(serialui_winetest msvcrt kernel32)
add_rostests_file(TARGET serialui_winetest)
