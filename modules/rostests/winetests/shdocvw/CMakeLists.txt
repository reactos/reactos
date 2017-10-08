
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    shdocvw.c
    shortcut.c
    testlist.c)

add_executable(shdocvw_winetest ${SOURCE})
target_link_libraries(shdocvw_winetest uuid)
set_module_type(shdocvw_winetest win32cui)
add_importlibs(shdocvw_winetest shell32 ole32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET shdocvw_winetest)
