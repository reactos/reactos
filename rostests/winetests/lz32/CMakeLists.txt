
add_definitions(-D__ROS_LONG64__)

add_executable(lz32_winetest lzexpand_main.c testlist.c)
target_link_libraries(lz32_winetest wine)
set_module_type(lz32_winetest win32cui)
add_importlibs(lz32_winetest lz32 msvcrt kernel32 ntdll)
add_cd_file(TARGET lz32_winetest DESTINATION reactos/bin FOR all)
