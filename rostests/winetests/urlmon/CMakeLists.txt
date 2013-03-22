
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    generated.c
    misc.c
    protocol.c
    sec_mgr.c
    stream.c
    uri.c
    url.c
    testlist.c)

add_executable(urlmon_winetest ${SOURCE})
target_link_libraries(urlmon_winetest wine uuid)
set_module_type(urlmon_winetest win32cui)
add_importlibs(urlmon_winetest urlmon wininet ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET urlmon_winetest DESTINATION reactos/bin FOR all)
