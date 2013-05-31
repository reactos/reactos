
add_definitions(-D__ROS_LONG64__)
add_executable(avifil32_winetest api.c testlist.c)
target_link_libraries(avifil32_winetest wine)
set_module_type(avifil32_winetest win32cui)
add_importlibs(avifil32_winetest avifil32 ole32 msvcrt kernel32)
add_cd_file(TARGET avifil32_winetest DESTINATION reactos/bin FOR all)
