
add_definitions(-D__ROS_LONG64__)

add_executable(cmd_winetest batch.c testlist.c rsrc.rc)
set_module_type(cmd_winetest win32cui)
add_importlibs(cmd_winetest msvcrt kernel32)
add_rostests_file(TARGET cmd_winetest)
