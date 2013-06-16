
add_definitions(-D__ROS_LONG64__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_executable(localspl_winetest localmon.c testlist.c)
target_link_libraries(localspl_winetest wine)
set_module_type(localspl_winetest win32cui)
add_importlibs(localspl_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET localspl_winetest DESTINATION reactos/bin FOR all)
