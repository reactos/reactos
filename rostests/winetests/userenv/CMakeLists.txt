
add_executable(userenv_winetest userenv.c testlist.c)
set_module_type(userenv_winetest win32cui)
add_importlibs(userenv_winetest userenv advapi32 msvcrt kernel32)
add_cd_file(TARGET userenv_winetest DESTINATION reactos/bin FOR all)
