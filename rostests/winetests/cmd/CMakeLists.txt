
add_executable(cmd_winetest batch.c testlist.c rsrc.rc)
set_module_type(cmd_winetest win32cui)
add_importlibs(cmd_winetest msvcrt kernel32)
add_cd_file(TARGET cmd_winetest DESTINATION reactos/bin FOR all)
