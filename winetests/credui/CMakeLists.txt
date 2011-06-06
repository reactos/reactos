
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(credui_winetest credui.c testlist.c)
target_link_libraries(credui_winetest wine)
set_module_type(credui_winetest win32cui)
add_importlibs(credui_winetest credui msvcrt kernel32 ntdll)
add_cd_file(TARGET credui_winetest DESTINATION reactos/bin FOR all)
