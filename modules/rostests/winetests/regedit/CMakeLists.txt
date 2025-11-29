
add_executable(regedit_winetest regedit.c testlist.c)
target_link_libraries(regedit_winetest wine)
set_module_type(regedit_winetest win32cui)
add_importlibs(regedit_winetest advapi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET regedit_winetest)
