
add_definitions(-D__ROS_LONG64__)
add_executable(services_winetest service.c testlist.c)
target_link_libraries(services_winetest wine)
set_module_type(services_winetest win32cui)
add_importlibs(services_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET services_winetest DESTINATION reactos/bin FOR all)
