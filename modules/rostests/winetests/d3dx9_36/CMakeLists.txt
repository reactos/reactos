
add_executable(d3dx9_36_winetest
    asm.c
    core.c 
    effect.c 
    line.c
    math.c
    mesh.c
    shader.c
    surface.c
    texture.c
    volume.c
    xfile.c
    testlist.c
    rsrc.rc)
target_compile_definitions(d3dx9_36_winetest PRIVATE -DUSE_WINE_TODOS -D__WINESRC__ -Disnan=_isnan)
if(MSVC)
    # Disable warning C4477 (printf format warnings)
    target_compile_options(d3dx9_36_winetest PRIVATE "/wd4477")
endif()

target_link_libraries(d3dx9_36_winetest uuid dxguid)
set_module_type(d3dx9_36_winetest win32cui)
add_importlibs(d3dx9_36_winetest d3dx9_36 d3d9 user32 gdi32 msvcrt kernel32)
add_rostests_file(TARGET d3dx9_36_winetest)
