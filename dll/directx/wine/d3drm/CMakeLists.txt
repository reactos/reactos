
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3drm.dll d3drm.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3drm.c
    d3drm_main.c
    device.c
    face.c
    frame.c
    light.c
    material.c
    math.c
    meshbuilder.c
    texture.c
    viewport.c
    precomp.h)

add_library(d3drm MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm.def)

set_module_type(d3drm win32dll UNICODE)
target_link_libraries(d3drm dxguid uuid wine)
add_importlibs(d3drm ddraw d3dxof msvcrt kernel32 ntdll)
add_pch(d3drm precomp.h SOURCE)
add_cd_file(TARGET d3drm DESTINATION reactos/system32 FOR all)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(d3drm PRIVATE -Wno-incompatible-function-pointer-types)
endif()
