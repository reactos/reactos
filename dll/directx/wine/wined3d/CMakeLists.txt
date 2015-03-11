
add_definitions(
    -D__WINESRC__
    -D_USE_MATH_DEFINES
    -DUSE_WIN32_OPENGL)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

# We name this d3dwine.dll, because the Virtualbox additions ship with a custom wined3d.dll
# and it breaks everything if it is installed.
spec2def(d3dwine.dll wined3d.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    arb_program_shader.c
    ati_fragment_shader.c
    buffer.c
    context.c
    cs.c
    device.c
    directx.c
    drawprim.c
    dxtn.c
    gl_compat.c
    glsl_shader.c
    nvidia_texture_shader.c
    palette.c
    query.c
    resource.c
    sampler.c
    shader.c
    shader_sm1.c
    shader_sm4.c
    state.c
    stateblock.c
    surface.c
    swapchain.c
    texture.c
    utils.c
    vertexdeclaration.c
    view.c
    volume.c
    wined3d_main.c
    wined3d_private.h)

add_library(d3dwine SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dwine.def)

set_module_type(d3dwine win32dll)
target_link_libraries(d3dwine wine)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(d3dwine mingwex)
endif()

add_importlibs(d3dwine user32 opengl32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(d3dwine wined3d_private.h SOURCE)
add_cd_file(TARGET d3dwine DESTINATION reactos/system32 FOR all)
