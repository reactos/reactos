
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    notification.c
    testlist.c
    url.c
    winhttp.c)

add_executable(winhttp_winetest ${SOURCE})
target_link_libraries(winhttp_winetest wine)
set_module_type(winhttp_winetest win32cui)
add_importlibs(winhttp_winetest winhttp crypt32 ws2_32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET winhttp_winetest DESTINATION reactos/bin FOR all)
