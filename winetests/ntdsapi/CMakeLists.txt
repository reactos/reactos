
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(ntdsapi_winetest ntdsapi.c testlist.c)
target_link_libraries(ntdsapi_winetest wine)
set_module_type(ntdsapi_winetest win32cui)
add_importlibs(ntdsapi_winetest ntdsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET ntdsapi_winetest DESTINATION reactos/bin FOR all)
