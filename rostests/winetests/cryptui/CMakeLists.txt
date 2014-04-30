
add_executable(cryptui_winetest cryptui.c testlist.c)
set_module_type(cryptui_winetest win32cui)
add_importlibs(cryptui_winetest cryptui crypt32 user32 msvcrt kernel32)
add_cd_file(TARGET cryptui_winetest DESTINATION reactos/bin FOR all)
