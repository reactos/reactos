
add_executable(pdh_winetest pdh.c testlist.c)
set_module_type(pdh_winetest win32cui)
add_importlibs(pdh_winetest pdh msvcrt kernel32 ntdll)
add_cd_file(TARGET pdh_winetest DESTINATION reactos/bin FOR all)
