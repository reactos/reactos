
add_executable(wldap32_winetest parse.c testlist.c)
set_module_type(wldap32_winetest win32cui)
add_importlibs(wldap32_winetest wldap32 msvcrt kernel32)
add_rostests_file(TARGET wldap32_winetest)
