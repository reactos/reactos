
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3dx9_36.dll d3dx9_36.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    core.c
    d3dx9_36_main.c
    effect.c
    font.c
    line.c
    math.c
    mesh.c
    render.c
    shader.c
    skin.c
    sprite.c
    surface.c
    texture.c
    util.c
    volume.c
    xfile.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_36_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_36.def)

add_library(d3dx9_36 SHARED ${SOURCE} version.rc)
set_module_type(d3dx9_36 win32dll)
add_dependencies(d3dx9_36 d3d_idl_headers)
target_link_libraries(d3dx9_36 dxguid wine)
add_importlibs(d3dx9_36 d3dcompiler_43 d3dxof user32 ole32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dx9_36 DESTINATION reactos/system32 FOR all)
