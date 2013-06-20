
add_definitions(
    -D__ROS_LONG64__
    -D__WINESRC__)

add_executable(usp10_winetest usp10.c testlist.c)
target_link_libraries(usp10_winetest wine)
set_module_type(usp10_winetest win32cui)
add_importlibs(usp10_winetest usp10 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET usp10_winetest DESTINATION reactos/bin FOR all)
