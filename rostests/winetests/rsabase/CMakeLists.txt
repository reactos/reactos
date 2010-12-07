
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(rsabase_winetest rsabase.c testlist.c)
target_link_libraries(rsabase_winetest wine)
set_module_type(rsabase_winetest win32cui)
add_importlibs(rsabase_winetest advapi32 msvcrt kernel32 ntdll)
