
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${VKD3D_INCLUDE_DIRS})

spec2def(d3d8.dll d3d8.spec ADD_IMPORTLIB)

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
    volume.c)

add_library(d3d8 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

if(MSVC)
    # C5286: implicit conversion between enum types
    target_compile_options(d3d8 PRIVATE /wd5286)
endif()

set_module_type(d3d8 win32dll UNICODE)
target_link_libraries(d3d8 wine dxguid uuid)
add_importlibs(d3d8 d3dwine user32 msvcrt kernel32 ntdll)
add_dependencies(d3d8 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d8)
