
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(uxtheme_winetest system.c testlist.c)
target_link_libraries(uxtheme_winetest wine)
set_module_type(uxtheme_winetest win32cui)
add_importlibs(uxtheme_winetest user32 msvcrt kernel32 ntdll)
add_cab_target(uxtheme_winetest 7)
