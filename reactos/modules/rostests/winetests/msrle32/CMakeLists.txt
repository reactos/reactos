
add_definitions(-DUSE_WINE_TODOS)
add_executable(msrle32_winetest msrle.c testlist.c)
set_module_type(msrle32_winetest win32cui)
add_importlibs(msrle32_winetest msvfw32 msvcrt kernel32)
add_rostests_file(TARGET msrle32_winetest)
