
add_definitions(-DUSE_WINE_TODOS)
add_executable(spoolss_winetest spoolss.c testlist.c)
set_module_type(spoolss_winetest win32cui)
add_importlibs(spoolss_winetest msvcrt kernel32)
add_rostests_file(TARGET spoolss_winetest)
