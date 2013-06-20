
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(snmpapi_winetest util.c testlist.c)
target_link_libraries(snmpapi_winetest wine)
set_module_type(snmpapi_winetest win32cui)
add_importlibs(snmpapi_winetest snmpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET snmpapi_winetest DESTINATION reactos/bin FOR all)
