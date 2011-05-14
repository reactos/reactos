
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(rsaenh_winetest rsaenh.c testlist.c)
target_link_libraries(rsaenh_winetest wine)
set_module_type(rsaenh_winetest win32cui)
add_importlibs(rsaenh_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET rsaenh_winetest DESTINATION reactos/bin FOR all)
