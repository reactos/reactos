
add_executable(browseui_winetest autocomplete.c progressdlg.c testlist.c)
target_link_libraries(browseui_winetest uuid)
set_module_type(browseui_winetest win32cui)
add_importlibs(browseui_winetest ole32 msvcrt kernel32)
add_rostests_file(TARGET browseui_winetest)
