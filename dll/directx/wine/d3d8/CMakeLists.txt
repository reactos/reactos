
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
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
    precomp.h)

add_library(d3d8 MODULE
    ${SOURCE}
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

set_module_type(d3d8 win32dll UNICODE)
target_link_libraries(d3d8 uuid wine)
add_importlibs(d3d8 d3dwine msvcrt kernel32 ntdll)
add_pch(d3d8 precomp.h SOURCE)
add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)
