
add_definitions(-DUSE_WINE_TODOS)

add_executable(mscoree_winetest
    debugging.c
    metahost.c
    mscoree.c
    testlist.c)

target_link_libraries(mscoree_winetest wine uuid)
set_module_type(mscoree_winetest win32cui)
add_importlibs(mscoree_winetest ole32 shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET mscoree_winetest)
