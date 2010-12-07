
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(xmllite_winetest reader.c testlist.c)
target_link_libraries(xmllite_winetest wine)
set_module_type(xmllite_winetest win32cui)
add_importlibs(xmllite_winetest xmllite ole32 msvcrt kernel32 ntdll)
