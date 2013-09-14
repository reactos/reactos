
add_definitions(-D__ROS_LONG64__)
add_executable(credui_winetest credui.c testlist.c)
set_module_type(credui_winetest win32cui)
add_importlibs(credui_winetest credui msvcrt kernel32)
add_cd_file(TARGET credui_winetest DESTINATION reactos/bin FOR all)
