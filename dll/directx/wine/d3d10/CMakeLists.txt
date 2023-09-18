
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(d3d10.dll d3d10.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3d10_main.c
    effect.c
    shader.c
    stateblock.c
    utils.c
    d3d10_private.h)

add_library(d3d10 SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3d10 PRIVATE -Wno-incompatible-pointer-types -Wno-unused-but-set-variable -Wno-switch -Wno-error) # Our favourite compiler :)
endif()

set_module_type(d3d10 win32dll)
target_link_libraries(d3d10 wine dxguid uuid)
add_importlibs(d3d10 d3d11 d3dwine msvcrt d3dcompiler_43 d3d10core dxgi kernel32 ntdll)
add_pch(d3d10 d3d10_private.h SOURCE)
add_dependencies(d3d10 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d10 DESTINATION reactos/system32 FOR all)
