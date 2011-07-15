
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(wlanapi_winetest wlanapi.c testlist.c)
target_link_libraries(wlanapi_winetest wine)
set_module_type(wlanapi_winetest win32cui)
add_importlibs(wlanapi_winetest wlanapi msvcrt kernel32 ntdll)
add_cd_file(TARGET wlanapi_winetest DESTINATION reactos/bin FOR all)
