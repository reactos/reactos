
add_executable(amstream_winetest amstream.c testlist.c)
target_link_libraries(amstream_winetest uuid)
set_module_type(amstream_winetest win32cui)
add_importlibs(amstream_winetest ole32 user32 ddraw msvcrt kernel32)
add_rostests_file(TARGET amstream_winetest)
