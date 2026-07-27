
add_definitions(-DUSE_WINE_TODOS -D_USE_MATH_DEFINES -DWINETEST_USE_DBGSTR_LONGLONG -DCONST_VTABLE)
add_executable(d2d1_winetest d2d1.c testlist.c)
target_link_libraries(d2d1_winetest uuid dxguid dx10guid)
set_module_type(d2d1_winetest win32cui)
add_importlibs(d2d1_winetest d2d1 d3d10_1 d3d11 dwrite d3dcompiler_47 ole32 advapi32 gdi32 user32 msvcrt kernel32)
add_rostests_file(TARGET d2d1_winetest)
