
add_executable(odbccp32_winetest misc.c testlist.c)
set_module_type(odbccp32_winetest win32cui)
add_importlibs(odbccp32_winetest odbccp32 advapi32 msvcrt kernel32)
add_cd_file(TARGET odbccp32_winetest DESTINATION reactos/bin FOR all)
