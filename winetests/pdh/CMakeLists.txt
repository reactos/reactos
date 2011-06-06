
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(pdh_winetest pdh.c testlist.c)
target_link_libraries(pdh_winetest wine)
set_module_type(pdh_winetest win32cui)
add_importlibs(pdh_winetest pdh msvcrt kernel32 ntdll)
add_cd_file(TARGET pdh_winetest DESTINATION reactos/bin FOR all)
