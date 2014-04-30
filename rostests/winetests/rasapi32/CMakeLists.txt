
add_executable(rasapi32_winetest rasapi.c testlist.c)
set_module_type(rasapi32_winetest win32cui)
add_importlibs(rasapi32_winetest msvcrt kernel32)
add_cd_file(TARGET rasapi32_winetest DESTINATION reactos/bin FOR all)
