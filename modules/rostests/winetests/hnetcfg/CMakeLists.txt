
add_definitions(-DUSE_WINE_TODOS)
add_executable(hnetcfg_winetest policy.c testlist.c)
target_link_libraries(hnetcfg_winetest uuid)
set_module_type(hnetcfg_winetest win32cui)
add_importlibs(hnetcfg_winetest ole32 oleaut32 mpr msvcrt kernel32)
add_rostests_file(TARGET hnetcfg_winetest)
