
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    ftp.c
    generated.c
    http.c
    internet.c
    url.c
    urlcache.c
    testlist.c)

add_executable(wininet_winetest ${SOURCE})
set_module_type(wininet_winetest win32cui)
add_importlibs(wininet_winetest wininet crypt32 ws2_32 user32 advapi32 shell32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET wininet_winetest)
