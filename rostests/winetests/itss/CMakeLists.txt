
add_definitions(-D__ROS_LONG64__)
add_executable(itss_winetest protocol.c testlist.c rsrc.rc)
target_link_libraries(itss_winetest wine)
set_module_type(itss_winetest win32cui)
add_importlibs(itss_winetest ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET itss_winetest DESTINATION reactos/bin FOR all)
