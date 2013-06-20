
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(sxs_winetest
    cache.c
    name.c
    testlist.c)

target_link_libraries(sxs_winetest wine uuid)
set_module_type(sxs_winetest win32cui)
add_importlibs(sxs_winetest sxs msvcrt kernel32 ntdll)
add_cd_file(TARGET sxs_winetest DESTINATION reactos/bin FOR all)
