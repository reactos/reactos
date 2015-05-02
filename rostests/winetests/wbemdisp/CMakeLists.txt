
add_definitions(-DUSE_WINE_TODOS)
add_executable(wbemdisp_winetest wbemdisp.c testlist.c)
target_link_libraries(wbemdisp_winetest uuid)
set_module_type(wbemdisp_winetest win32cui)
add_importlibs(wbemdisp_winetest oleaut32 ole32 user32 msvcrt kernel32)
add_cd_file(TARGET wbemdisp_winetest DESTINATION reactos/bin FOR all)
