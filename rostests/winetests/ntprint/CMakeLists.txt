
add_executable(ntprint_winetest ntprint.c testlist.c)
set_module_type(ntprint_winetest win32cui)
add_importlibs(ntprint_winetest msvcrt kernel32)
add_cd_file(TARGET ntprint_winetest DESTINATION reactos/bin FOR all)
