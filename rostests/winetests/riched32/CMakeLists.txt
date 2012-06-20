
add_definitions(-D__ROS_LONG64__)

add_executable(riched32_winetest editor.c testlist.c)
target_link_libraries(riched32_winetest wine)
set_module_type(riched32_winetest win32cui)
add_importlibs(riched32_winetest ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET riched32_winetest DESTINATION reactos/bin FOR all)
