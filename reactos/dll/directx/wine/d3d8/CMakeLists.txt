
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3d8.dll d3d8.spec)

list(APPEND SOURCE
    buffer.c
    d3d8_main.c
    device.c
    directx.c
    shader.c
    surface.c
    swapchain.c
    texture.c
    vertexdeclaration.c
    volume.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

add_library(d3d8 SHARED ${SOURCE})
set_module_type(d3d8 win32dll UNICODE)
target_link_libraries(d3d8 uuid wine)

add_importlibs(d3d8
    user32
    gdi32
    advapi32
    wined3d
    msvcrt
    kernel32
    ntdll)

add_dependencies(d3d8 wineheaders)
add_pch(d3d8 d3d8_private.h)
add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)
