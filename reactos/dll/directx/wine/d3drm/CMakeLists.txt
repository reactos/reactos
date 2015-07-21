
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
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
    d3drm_private.h)

add_library(d3drm SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm.def)

set_module_type(d3drm win32dll UNICODE)
target_link_libraries(d3drm dxguid uuid wine)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(d3drm mingwex)
endif()

add_importlibs(d3drm d3dxof msvcrt kernel32 ntdll)
add_pch(d3drm d3drm_private.h SOURCE)
add_cd_file(TARGET d3drm DESTINATION reactos/system32 FOR all)
