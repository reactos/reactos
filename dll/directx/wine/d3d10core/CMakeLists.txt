
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(d3d10core.dll d3d10core.spec ADD_IMPORTLIB)

add_library(d3d10core MODULE
    d3d10core_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10core_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10core.def)

set_module_type(d3d10core win32dll UNICODE)
target_link_libraries(d3d10core wine dxguid dx10guid uuid)
add_importlibs(d3d10core d3d11 dxgi msvcrt kernel32 ntdll)
add_dependencies(d3d10core wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d10core DESTINATION reactos/system32 FOR all)
set_wine_module(d3d10core)
