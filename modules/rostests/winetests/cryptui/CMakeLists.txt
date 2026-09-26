
add_executable(cryptui_winetest cryptui.c)
set_module_type(cryptui_winetest win32cui)
target_link_libraries(cryptui_winetest winetestlib)
add_importlibs(cryptui_winetest cryptui crypt32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET cryptui_winetest)
