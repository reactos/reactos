
add_definitions(-DUSE_WINE_TODOS)
add_executable(mlang_winetest mlang.c testlist.c)
set_module_type(mlang_winetest win32cui)
add_importlibs(mlang_winetest mlang oleaut32 ole32 user32 gdi32 msvcrt kernel32)
add_rostests_file(TARGET mlang_winetest)
