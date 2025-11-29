
add_definitions(-DUSE_WINE_TODOS)
add_executable(cabinet_winetest extract.c fdi.c testlist.c)
set_module_type(cabinet_winetest win32cui)
add_importlibs(cabinet_winetest cabinet msvcrt kernel32)
add_rostests_file(TARGET cabinet_winetest)
