
add_definitions(-D__ROS_LONG64__)

add_executable(xmllite_winetest reader.c testlist.c)
target_link_libraries(xmllite_winetest wine)

if(MSVC)
    target_link_libraries(xmllite_winetest uuid)
endif()

set_module_type(xmllite_winetest win32cui)
add_importlibs(xmllite_winetest xmllite ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET xmllite_winetest DESTINATION reactos/bin FOR all)
