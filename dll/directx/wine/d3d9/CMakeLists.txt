
set_unicode()

add_definitions(-D__WINESRC__)
add_definitions(-DUSE_WIN32_OPENGL)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3d9 ${CMAKE_CURRENT_SOURCE_DIR}/d3d9.spec ${CMAKE_CURRENT_BINARY_DIR}/d3d9.def)

add_library(d3d9 SHARED
    cubetexture.c
    d3d9_main.c
    device.c
    directx.c
    indexbuffer.c
    pixelshader.c
    query.c
    stateblock.c
    surface.c
    swapchain.c
    texture.c
    vertexbuffer.c
    vertexdeclaration.c
    vertexshader.c
    volume.c
    volumetexture.c
    version.rc)

set_module_type(d3d9 win32dll)

target_link_libraries(d3d9
    ${CMAKE_CURRENT_BINARY_DIR}/d3d9.def
    uuid
    wine)

add_importlibs(d3d9
    user32
    gdi32
    advapi32
    wined3d)

add_dependencies(d3d9 d3d9_def wineheaders)
add_livecd_target(d3d9 reactos/system32)
