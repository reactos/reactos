
add_definitions(-DUSE_WINE_TODOS)
remove_definitions(-D__ROS_LONG64__)
add_executable(cabinet_winetest extract.c fdi.c testlist.c)
set_module_type(cabinet_winetest win32cui)
add_importlibs(cabinet_winetest cabinet msvcrt kernel32)
add_rostests_file(TARGET cabinet_winetest)
