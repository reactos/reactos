
add_definitions(-DUSE_WINE_TODOS)
add_executable(usp10_winetest usp10.c testlist.c)
set_module_type(usp10_winetest win32cui)
add_importlibs(usp10_winetest usp10 user32 gdi32 msvcrt kernel32)
add_rostests_file(TARGET usp10_winetest)
