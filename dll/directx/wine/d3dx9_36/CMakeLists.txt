
add_definitions(-D__WINESRC__ -Dcopysignf=_copysignf -DD3DX_SDK_VERSION=36 -D_D3DX9_VER=36)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3dx9_36.dll d3dx9_36.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    animation.c
    core.c
    effect.c
    font.c
    line.c
    main.c
    math.c
    mesh.c
    preshader.c
    render.c
    shader.c
    skin.c
    sprite.c
    surface.c
    texture.c
    util.c
    volume.c
    xfile.c
    precomp.h)

add_library(d3dx9_36 MODULE
    ${SOURCE}
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_36_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_36.def)

set_module_type(d3dx9_36 win32dll)
add_dependencies(d3dx9_36 d3d_idl_headers)
target_link_libraries(d3dx9_36 dxguid wine)
add_importlibs(d3dx9_36 d3dcompiler_43 d3dxof d3dwine user32 ole32 gdi32 msvcrt kernel32 ntdll)
add_pch(d3dx9_36 precomp.h SOURCE)
add_cd_file(TARGET d3dx9_36 DESTINATION reactos/system32 FOR all)
