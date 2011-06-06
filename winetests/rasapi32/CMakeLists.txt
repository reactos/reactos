
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(rasapi32_winetest rasapi.c testlist.c)
target_link_libraries(rasapi32_winetest wine)
set_module_type(rasapi32_winetest win32cui)
add_importlibs(rasapi32_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET rasapi32_winetest DESTINATION reactos/bin FOR all)
