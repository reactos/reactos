
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    reader.c
	writer.c
	testlist.c)

add_executable(xmllite_winetest ${SOURCE})
target_link_libraries(xmllite_winetest wine uuid)

set_module_type(xmllite_winetest win32cui)
add_importlibs(xmllite_winetest xmllite ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET xmllite_winetest DESTINATION reactos/bin FOR all)
