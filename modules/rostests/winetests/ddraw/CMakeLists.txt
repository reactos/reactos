
add_definitions(-DUSE_WINE_TODOS)
add_executable(ddraw_winetest d3d.c ddraw1.c ddraw2.c ddraw4.c ddraw7.c ddrawmodes.c dsurface.c refcount.c visual.c testlist.c)
target_link_libraries(ddraw_winetest uuid dxguid)
set_module_type(ddraw_winetest win32cui)
add_importlibs(ddraw_winetest ddraw ole32 gdi32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET ddraw_winetest)
