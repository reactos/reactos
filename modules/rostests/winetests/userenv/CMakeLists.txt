
add_executable(userenv_winetest userenv.c testlist.c)
set_module_type(userenv_winetest win32cui)
target_link_libraries(userenv_winetest oldnames)
add_importlibs(userenv_winetest userenv advapi32 msvcrt kernel32)
add_rostests_file(TARGET userenv_winetest)
