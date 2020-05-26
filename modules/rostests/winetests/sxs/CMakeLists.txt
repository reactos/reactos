
add_definitions(-DUSE_WINE_TODOS)
add_idl_headers(sxs_winetest_idlheaders interfaces.idl)
add_executable(sxs_winetest cache.c name.c sxs.c testlist.c resource.rc)
set_module_type(sxs_winetest win32cui)
add_importlibs(sxs_winetest sxs shlwapi msvcrt kernel32)
add_rostests_file(TARGET sxs_winetest)
add_dependencies(sxs_winetest sxs_winetest_idlheaders)
