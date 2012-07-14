
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    notification.c
    url.c
    winhttp.c
    testlist.c)

add_executable(winhttp_winetest ${SOURCE})
target_link_libraries(winhttp_winetest wine uuid)
set_module_type(winhttp_winetest win32cui)
add_importlibs(winhttp_winetest winhttp oleaut32 ole32 crypt32 advapi32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET winhttp_winetest DESTINATION reactos/bin FOR all)
