
add_executable(xinput1_3_winetest xinput.c testlist.c)
set_module_type(xinput1_3_winetest win32cui)
add_importlibs(xinput1_3_winetest user32 msvcrt kernel32)
add_cd_file(TARGET xinput1_3_winetest DESTINATION reactos/bin FOR all)
