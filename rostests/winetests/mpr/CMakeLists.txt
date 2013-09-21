
add_executable(mpr_winetest mpr.c testlist.c)
set_module_type(mpr_winetest win32cui)
add_importlibs(mpr_winetest mpr msvcrt kernel32)
add_cd_file(TARGET mpr_winetest DESTINATION reactos/bin FOR all)
