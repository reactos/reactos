
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    mimeintl.c
    mimeole.c
    testlist.c)

add_executable(inetcomm_winetest ${SOURCE})
target_link_libraries(inetcomm_winetest wine)
set_module_type(inetcomm_winetest win32cui)
add_importlibs(inetcomm_winetest inetcomm oleaut32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET inetcomm_winetest DESTINATION reactos/bin FOR all)
