
add_definitions(-DUSE_WINE_TODOS)

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
target_link_libraries(urlmon_winetest uuid)
set_module_type(urlmon_winetest win32cui)
add_importlibs(urlmon_winetest urlmon wininet ole32 oleaut32 shlwapi user32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET urlmon_winetest)
