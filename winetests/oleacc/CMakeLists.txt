
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(oleacc_winetest main.c testlist.c)
target_link_libraries(oleacc_winetest wine)
set_module_type(oleacc_winetest win32cui)
add_importlibs(oleacc_winetest oleacc msvcrt kernel32 ntdll)
add_cd_file(TARGET oleacc_winetest DESTINATION reactos/bin FOR all)
