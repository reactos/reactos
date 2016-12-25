
add_executable(reg_winetest reg.c testlist.c)
set_module_type(reg_winetest win32cui)
add_importlibs(reg_winetest advapi32 msvcrt kernel32)
add_cd_file(TARGET reg_winetest DESTINATION reactos/bin FOR all)
