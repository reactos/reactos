
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${VKD3D_INCLUDE_DIRS})

spec2def(d3d12.dll d3d12.spec ADD_IMPORTLIB)

add_library(d3d12 MODULE
    d3d12_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12.def)

set_module_type(d3d12 win32dll)
target_link_libraries(d3d12 wine vkd3d dxguid dx10guid uuid)
add_importlibs(d3d12 dxgi d3dwine gdi32 user32 msvcrt kernel32_vista kernel32 ntdll)
add_dependencies(d3d12 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d12 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d12)
