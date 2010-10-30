
set_unicode()

add_definitions(-D__WINESRC__)
add_definitions(-DUSE_WIN32_OPENGL)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3d8.dll d3d8.spec)

add_library(d3d8 SHARED
    cubetexture.c
    d3d8_main.c
    device.c
    directx.c
    indexbuffer.c
    pixelshader.c
    surface.c
    swapchain.c
    texture.c
    vertexbuffer.c
    vertexdeclaration.c
    vertexshader.c
    volume.c
    volumetexture.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

set_module_type(d3d8 win32dll)

target_link_libraries(d3d8
    uuid
    wine)

add_importlibs(d3d8
    user32
    gdi32
    advapi32
    wined3d)

add_dependencies(d3d8 wineheaders)
add_cab_target(d3d8 1)
