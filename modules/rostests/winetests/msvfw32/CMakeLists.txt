
add_definitions(-DUSE_WINE_TODOS)
add_executable(msvfw32_winetest drawdib.c mciwnd.c msvfw.c testlist.c msvfw32.rc)
set_module_type(msvfw32_winetest win32cui)
add_importlibs(msvfw32_winetest user32 gdi32 advapi32 msvfw32 msvcrt kernel32)
add_rostests_file(TARGET msvfw32_winetest)
