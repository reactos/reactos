
add_executable(itss_winetest protocol.c testlist.c rsrc.rc)
set_module_type(itss_winetest win32cui)
add_importlibs(itss_winetest ole32 msvcrt kernel32)
add_cd_file(TARGET itss_winetest DESTINATION reactos/bin FOR all)
