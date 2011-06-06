
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(browseui_winetest autocomplete.c testlist.c)
target_link_libraries(browseui_winetest wine uuid)
set_module_type(browseui_winetest win32cui)
add_importlibs(browseui_winetest user32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET browseui_winetest DESTINATION reactos/bin FOR all)
