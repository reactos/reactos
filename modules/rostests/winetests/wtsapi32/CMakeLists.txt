
add_definitions(-D__WINESRC__ -DUSE_WINE_TODOS)
remove_definitions(-D__ROS_LONG64__)

add_executable(wtsapi32_winetest wtsapi.c testlist.c)
set_module_type(wtsapi32_winetest win32cui)
add_importlibs(wtsapi32_winetest wtsapi32 advapi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET wtsapi32_winetest)
