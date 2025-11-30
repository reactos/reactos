
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)

spec2def(d3d11.dll d3d11.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    async.c
    buffer.c
    d3d11_main.c
    decoder.c
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
if(MSVC)
    # Disable warning C4133: 'function': incompatible types - from 'WICPixelFormatNumericRepresentation *' to 'DWORD *'
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    target_compile_options(d3d11 PRIVATE /wd4133 /wd4146)
 
endif()
set_module_type(d3d11 win32dll)
target_link_libraries(d3d11 wine uuid dxguid dx10guid)
add_importlibs(d3d11 d3dwine msvcrt dxgi kernel32 ntdll gdi32_vista gdi32)
add_dependencies(d3d11 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d11 DESTINATION reactos/system32 FOR all)
