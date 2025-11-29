
add_executable(riched32_winetest editor.c testlist.c)
set_module_type(riched32_winetest win32cui)
add_importlibs(riched32_winetest ole32 user32 msvcrt kernel32)
add_rostests_file(TARGET riched32_winetest)
