
add_executable(opengl32_winetest opengl.c testlist.c)
set_module_type(opengl32_winetest win32cui)
add_importlibs(opengl32_winetest opengl32 gdi32 user32 msvcrt kernel32)
add_rostests_file(TARGET opengl32_winetest)
