
add_definitions(-D__ROS_LONG64__)
add_executable(cmd_winetest batch.c testlist.c rsrc.rc)
target_link_libraries(cmd_winetest wine)
set_module_type(cmd_winetest win32cui)
add_importlibs(cmd_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET cmd_winetest DESTINATION reactos/bin FOR all)
