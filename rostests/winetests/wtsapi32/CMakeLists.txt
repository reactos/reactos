
add_definitions(-DUSE_WINE_TODOS)
add_executable(wtsapi32_winetest wtsapi.c testlist.c)
set_module_type(wtsapi32_winetest win32cui)
add_importlibs(wtsapi32_winetest wtsapi32 msvcrt kernel32)
add_cd_file(TARGET wtsapi32_winetest DESTINATION reactos/bin FOR all)
