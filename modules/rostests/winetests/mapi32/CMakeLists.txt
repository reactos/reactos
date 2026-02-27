
list(APPEND SOURCE
    imalloc.c
    prop.c
    util.c
    testlist.c)

add_executable(mapi32_winetest ${SOURCE})
set_module_type(mapi32_winetest win32cui)
add_importlibs(mapi32_winetest advapi32 msvcrt kernel32)
add_rostests_file(TARGET mapi32_winetest)
