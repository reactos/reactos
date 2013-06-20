
add_definitions(-D__ROS_LONG64__)

remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    assoc.c
    clist.c
    clsid.c
    generated.c
    istream.c
    ordinal.c
    path.c
    shreg.c
    string.c
    thread.c
    url.c
    testlist.c)

add_executable(shlwapi_winetest ${SOURCE})
target_link_libraries(shlwapi_winetest wine uuid)
set_module_type(shlwapi_winetest win32cui)
add_importlibs(shlwapi_winetest shlwapi ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET shlwapi_winetest DESTINATION reactos/bin FOR all)
