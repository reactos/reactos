
add_definitions(-DUSE_WINE_TODOS)
add_executable(d3drm_winetest d3drm.c vector.c testlist.c)
target_link_libraries(d3drm_winetest uuid dxguid)
set_module_type(d3drm_winetest win32cui)
add_importlibs(d3drm_winetest ddraw d3drm user32 msvcrt kernel32)
add_cd_file(TARGET d3drm_winetest DESTINATION reactos/bin FOR all)
