
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    notification.c
    url.c
    winhttp.c
    testlist.c)

add_executable(winhttp_winetest ${SOURCE})
target_link_libraries(winhttp_winetest uuid)
set_module_type(winhttp_winetest win32cui)
add_importlibs(winhttp_winetest winhttp oleaut32 ole32 crypt32 advapi32 ws2_32 msvcrt kernel32)
add_rostests_file(TARGET winhttp_winetest)
