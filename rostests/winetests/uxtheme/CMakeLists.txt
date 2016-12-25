
add_executable(uxtheme_winetest system.c testlist.c)
set_module_type(uxtheme_winetest win32cui)
add_importlibs(uxtheme_winetest uxtheme user32 msvcrt kernel32)
add_cd_file(TARGET uxtheme_winetest DESTINATION reactos/bin FOR all)
