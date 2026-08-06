
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600
    -D_USE_MATH_DEFINES
    -D__ROS_LONG64__
    -DWINE_NO_NAMELESS_EXTENSION)

include_directories(${VKD3D_INCLUDE_DIRS})

# We name this d3dwine.dll, because the Virtualbox additions ship with a custom
# wined3d.dll
spec2def(d3dwine.dll wined3d.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    adapter_gl.c
    adapter_vk.c
    buffer.c
    context.c
    context_gl.c
    context_vk.c
    cs.c
    device.c
    directx.c
    ffp_gl.c
    ffp_hlsl.c
    gl_compat.c
    glsl_shader.c
    palette.c
    query.c
    resource.c
    sampler.c
    shader.c
    shader_sm1.c
    shader_sm4.c
    shader_spirv.c
    stateblock.c
    surface.c
    swapchain.c
    texture.c
    utils.c
    vertexdeclaration.c
    view.c
    wined3d_main.c)

add_library(d3dwine MODULE
    ${SOURCE}
    resource.rc
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dwine.def)

if(MSVC)
    # C4098: 'void' function returning a void expression
    # C4319: context_vk.c masks a 64-bit VkDeviceSize with a size_t-wide
    target_compile_options(d3dwine PRIVATE /wd4312 /wd4098 /wd4319)
endif()

set_module_type(d3dwine win32dll)
target_link_libraries(d3dwine wine vkd3d dxguid oldnames)
add_importlibs(d3dwine opengl32 user32 gdi32_vista gdi32 advapi32 ucrtbase kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(d3dwine wineheaders d3d_idl_headers)
add_cd_file(TARGET d3dwine DESTINATION reactos/system32 FOR all)
set_wine_module(d3dwine)
