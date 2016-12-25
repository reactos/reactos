
add_definitions(-D__WINESRC__)
add_executable(psapi_winetest psapi_main.c testlist.c)
set_module_type(psapi_winetest win32cui)
add_importlibs(psapi_winetest msvcrt kernel32)
add_cd_file(TARGET psapi_winetest DESTINATION reactos/bin FOR all)
