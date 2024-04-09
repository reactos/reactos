
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(d3d11.dll d3d11.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    async.c
    buffer.c
    d3d11_main.c
    device.c
    inputlayout.c
    shader.c
    state.c
    texture.c
    utils.c
    view.c)

add_library(d3d11 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/d3d11_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d11.def)

set_module_type(d3d11 win32dll)
target_link_libraries(d3d11 wine uuid dxguid)
add_importlibs(d3d11 d3dwine msvcrt dxgi kernel32 ntdll gdi32)
add_dependencies(d3d11 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d11 DESTINATION reactos/system32 FOR all)
