
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    ftp.c
    generated.c
    http.c
    internet.c
    url.c
    urlcache.c
    testlist.c)

add_executable(wininet_winetest ${SOURCE})
target_link_libraries(wininet_winetest wine)
set_module_type(wininet_winetest win32cui)
add_importlibs(wininet_winetest wininet crypt32 ws2_32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wininet_winetest DESTINATION reactos/bin FOR all)
