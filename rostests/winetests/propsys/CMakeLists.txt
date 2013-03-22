
add_definitions(-D__ROS_LONG64__)
add_executable(propsys_winetest propstore.c propsys.c testlist.c)
target_link_libraries(propsys_winetest wine)
set_module_type(propsys_winetest win32cui)
add_importlibs(propsys_winetest propsys ole32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET propsys_winetest DESTINATION reactos/bin FOR all)
