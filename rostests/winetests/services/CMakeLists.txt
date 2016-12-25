
add_executable(services_winetest service.c testlist.c)
set_module_type(services_winetest win32cui)
add_importlibs(services_winetest advapi32 msvcrt kernel32)
add_cd_file(TARGET services_winetest DESTINATION reactos/bin FOR all)
