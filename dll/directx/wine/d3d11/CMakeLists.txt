
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600)

include_directories(${VKD3D_INCLUDE_DIRS})

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

add_library(d3d11 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d11_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d11.def)

if(MSVC)
    # C5286: implicit conversion between enum types
    # C4133: device.c casts a D3D_FEATURE_LEVEL* to an enum wined3d_feature_level*.
    # C4098: 'void' function returning a void expression
    target_compile_options(d3d11 PRIVATE /wd5286 /wd4133 /wd4098)
endif()

set_module_type(d3d11 win32dll UNICODE)
target_link_libraries(d3d11 wine dxguid dx10guid uuid oldnames)
add_importlibs(d3d11 dxgi d3dwine gdi32 msvcrt kernel32 ntdll)
add_dependencies(d3d11 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d11 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d11)
