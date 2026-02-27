
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    browse_ctx.c
    hlink.c
    testlist.c)

add_executable(hlink_winetest ${SOURCE})
set_module_type(hlink_winetest win32cui)
add_importlibs(hlink_winetest hlink ole32 msvcrt kernel32)
add_rostests_file(TARGET hlink_winetest)
