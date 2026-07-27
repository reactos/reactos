
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(d3d10_1.dll d3d10_1.spec ADD_IMPORTLIB)

add_library(d3d10_1 MODULE
    d3d10_1_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_1_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_1.def)

set_module_type(d3d10_1 win32dll UNICODE)
target_link_libraries(d3d10_1 wine dxguid dx10guid uuid)
add_importlibs(d3d10_1 d3d10core dxgi d3d10 msvcrt kernel32 ntdll)
add_dependencies(d3d10_1 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d10_1 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d10_1)
