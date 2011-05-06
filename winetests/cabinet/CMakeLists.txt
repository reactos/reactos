
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(cabinet_winetest extract.c fdi.c testlist.c)
target_link_libraries(cabinet_winetest wine)
set_module_type(cabinet_winetest win32cui)
add_importlibs(cabinet_winetest cabinet msvcrt kernel32 ntdll)
add_cab_target(cabinet_winetest 7)
