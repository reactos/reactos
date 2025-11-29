
add_executable(uxtheme_winetest system.c testlist.c)
set_module_type(uxtheme_winetest win32cui)
add_importlibs(uxtheme_winetest user32 gdi32 uxtheme msvcrt kernel32)
add_rostests_file(TARGET uxtheme_winetest)
