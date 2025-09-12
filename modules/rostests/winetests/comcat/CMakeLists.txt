
add_executable(comcat_winetest comcat.c testlist.c)
target_link_libraries(comcat_winetest uuid)
set_module_type(comcat_winetest win32cui)
add_importlibs(comcat_winetest ole32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET comcat_winetest)
