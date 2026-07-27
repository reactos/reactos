
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${VKD3D_INCLUDE_DIRS})

spec2def(d3d9.dll d3d9.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    buffer.c
    d3d9_main.c
    device.c
    directx.c
    query.c
    shader.c
    stateblock.c
    surface.c
    swapchain.c
    texture.c
    vertexdeclaration.c
    volume.c)

add_library(d3d9 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d9_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d9.def)

if(MSVC)
    # C5286: implicit conversion between enum types
    target_compile_options(d3d9 PRIVATE /wd5286)
endif()

set_module_type(d3d9 win32dll UNICODE)
target_link_libraries(d3d9 wine dxguid uuid)
add_importlibs(d3d9 d3dwine user32 msvcrt kernel32 ntdll)
add_dependencies(d3d9 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d9 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d9)
