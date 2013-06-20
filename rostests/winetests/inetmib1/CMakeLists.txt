
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(inetmib1_winetest main.c testlist.c)
target_link_libraries(inetmib1_winetest wine)
set_module_type(inetmib1_winetest win32cui)
add_importlibs(inetmib1_winetest snmpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET inetmib1_winetest DESTINATION reactos/bin FOR all)
