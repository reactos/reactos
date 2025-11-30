remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(
    -D_USE_MATH_DEFINES
    -DUSE_WIN32_OPENGL
    -D__ROS_LONG64__
    -Dcopysignf=_copysignf
    -Dputenv=_putenv
    -Dstricmp=_stricmp)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)

# We name this d3dwine.dll, because the Virtualbox additions ship with a custom wined3d.dll
# and it breaks everything if it is installed.
spec2def(d3dwine.dll wined3d.spec ADD_IMPORTLIB)
list(APPEND MAIN_SOURCE wined3d_main.c)
list(APPEND SOURCE
    adapter_gl.c
    adapter_vk.c
    buffer.c
    context_gl.c
    context_vk.c
    context.c
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
    shader_sm1.c
    shader_sm4.c
    shader_spirv.c
    shader.c
    stateblock.c
    surface.c
    swapchain.c
    texture.c
    utils.c
    vertexdeclaration.c
    view.c
    precomp.h)

add_library(d3dwine_common ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3dwine_common PRIVATE -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-write-strings -Wno-unknown-pragmas -Wno-switch -Wno-format-extra-args -Wno-format-overflow)
endif()

if(MSVC)
    target_compile_options(d3dwine_common PRIVATE /wd4028 /wd4090 /wd4312)
endif()
add_dependencies(d3dwine_common psdk xdk wineheaders d3d_idl_headers)

# FULL WINED3D
add_library(d3dwine MODULE
    ${MAIN_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dwine.def)

set_module_type(d3dwine win32dll)
target_link_libraries(d3dwine d3dwine_common vkd3d vkd3d-shader vkd3d-common wine wpp uuid )
add_importlibs(d3dwine user32 opengl32 gdi32_vista gdi32 advapi32 msvcrt kernel32 kernel32_vista ntdll)
add_pch(d3dwine precomp.h SOURCE)
set_wine_module(d3dwine)
add_cd_file(TARGET d3dwine DESTINATION reactos/system32 FOR all)
add_dependencies(d3dwine psdk xdk wineheaders d3d_idl_headers)

# DDRAW WINED3D
spec2def(ddraw_d3dwine.dll wined3d.spec ADD_IMPORTLIB)
add_library(ddraw_d3dwine MODULE
    ${MAIN_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw_d3dwine.def)
target_compile_definitions(ddraw_d3dwine PRIVATE
    -DWINE_DDRAW_SW)
set_module_type(ddraw_d3dwine win32dll)
target_link_libraries(ddraw_d3dwine d3dwine_common vkd3d vkd3d-shader vkd3d-common wine wpp uuid )
add_importlibs(ddraw_d3dwine user32 opengl32 gdi32_vista gdi32 advapi32 msvcrt kernel32 kernel32_vista ntdll)
add_pch(ddraw_d3dwine precomp.h SOURCE)
set_wine_module(ddraw_d3dwine)
add_cd_file(TARGET ddraw_d3dwine DESTINATION reactos/system32 FOR all)
add_dependencies(ddraw_d3dwine psdk xdk wineheaders d3d_idl_headers)
