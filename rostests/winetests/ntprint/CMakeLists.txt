
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(ntprint_winetest ntprint.c testlist.c)
target_link_libraries(ntprint_winetest wine)
set_module_type(ntprint_winetest win32cui)
add_importlibs(ntprint_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET ntprint_winetest DESTINATION reactos/bin FOR all)
