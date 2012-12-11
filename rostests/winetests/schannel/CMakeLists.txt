
add_definitions(-D__ROS_LONG64__)
add_executable(schannel_winetest main.c testlist.c)
target_link_libraries(schannel_winetest wine)
set_module_type(schannel_winetest win32cui)
add_importlibs(schannel_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET schannel_winetest DESTINATION reactos/bin FOR all)
