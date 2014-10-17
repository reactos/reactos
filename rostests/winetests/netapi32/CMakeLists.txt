
list(APPEND SOURCE
    access.c
    apibuf.c
    ds.c
    wksta.c
    testlist.c)

add_executable(netapi32_winetest ${SOURCE})
target_link_libraries(netapi32_winetest)
set_module_type(netapi32_winetest win32cui)
add_importlibs(netapi32_winetest advapi32 msvcrt kernel32)
add_cd_file(TARGET netapi32_winetest DESTINATION reactos/bin FOR all)
