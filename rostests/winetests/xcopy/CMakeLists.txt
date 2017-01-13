
add_executable(xcopy_winetest xcopy.c testlist.c)
set_module_type(xcopy_winetest win32cui)
add_importlibs(xcopy_winetest msvcrt kernel32)
add_rostests_file(TARGET xcopy_winetest)
