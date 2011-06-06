
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(psapi_winetest psapi_main.c testlist.c)
target_link_libraries(psapi_winetest wine)
set_module_type(psapi_winetest win32cui)
add_importlibs(psapi_winetest psapi msvcrt kernel32 ntdll)
add_cd_file(TARGET psapi_winetest DESTINATION reactos/bin FOR all)
