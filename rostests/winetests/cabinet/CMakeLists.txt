
add_definitions(-D__ROS_LONG64__)
add_executable(cabinet_winetest extract.c fdi.c testlist.c)
target_link_libraries(cabinet_winetest wine)
set_module_type(cabinet_winetest win32cui)
add_importlibs(cabinet_winetest cabinet msvcrt kernel32 ntdll)
add_cd_file(TARGET cabinet_winetest DESTINATION reactos/bin FOR all)
