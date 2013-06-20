
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(serialui_winetest confdlg.c testlist.c)
target_link_libraries(serialui_winetest wine)
set_module_type(serialui_winetest win32cui)
add_importlibs(serialui_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET serialui_winetest DESTINATION reactos/bin FOR all)
