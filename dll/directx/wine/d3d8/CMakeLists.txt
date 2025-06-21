
add_definitions(
    -D__ROS_LONG64__
    -DUSE_WIN32_OPENGL)

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
    volume.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(d3d8 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

if(MSVC)
    # Disable C5286: implicit conversion from enum type 'x' to enum type 'y';
    target_compile_options(d3d8 PRIVATE /wd5286)
endif()

set_module_type(d3d8 win32dll UNICODE)
target_link_libraries(d3d8 uuid wine)
add_importlibs(d3d8 d3dwine msvcrt kernel32 ntdll)
add_pch(d3d8 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(d3d8) # CORE-5743: No CONST_VTABLE
