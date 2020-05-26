
add_definitions(-DUSE_WINE_TODOS -DD3D_COMPILER_VERSION=43)

add_executable(d3dcompiler_43_winetest
    asm.c
    blob.c
    hlsl.c
    reflection.c
    testlist.c)

set_module_type(d3dcompiler_43_winetest win32cui)
add_dependencies(d3dcompiler_43_winetest d3d_idl_headers)
add_importlibs(d3dcompiler_43_winetest d3d9 d3dx9_43 user32 msvcrt kernel32)
add_rostests_file(TARGET d3dcompiler_43_winetest)
