
add_executable(devenum_winetest devenum.c testlist.c)
set_module_type(devenum_winetest win32cui)
add_importlibs(devenum_winetest oleaut32 ole32 msvcrt kernel32)
add_cd_file(TARGET devenum_winetest DESTINATION reactos/bin FOR all)
