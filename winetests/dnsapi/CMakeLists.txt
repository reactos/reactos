
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(dnsapi_winetest name.c record.c testlist.c)
target_link_libraries(dnsapi_winetest wine)
set_module_type(dnsapi_winetest win32cui)
add_importlibs(dnsapi_winetest dnsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET dnsapi_winetest DESTINATION reactos/bin FOR all)
