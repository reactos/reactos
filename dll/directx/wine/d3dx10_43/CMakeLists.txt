
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600)

include_directories(
    BEFORE
    ${REACTOS_SOURCE_DIR}/sdk/include/wine
    ${REACTOS_BINARY_DIR}/sdk/include/wine)

spec2def(d3dx10_43.dll d3dx10_43.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    async.c
    compiler.c
    d3dx10_43_main.c
    font.c
    mesh.c
    sprite.c
    texture.c
    guid.c)

add_library(d3dx10_43 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx10_43_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx10_43.def)

set_module_type(d3dx10_43 win32dll)
target_link_libraries(d3dx10_43 wine dxguid dx10guid uuid)
add_importlibs(d3dx10_43
    d3d10_1 d3dcompiler_47 d3dx9_36 gdi32
    kernel32_vista ntdll_vista msvcrt kernel32 ntdll)
add_delay_importlibs(d3dx10_43 windowscodecs)
add_dependencies(d3dx10_43 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3dx10_43 DESTINATION reactos/system32 FOR all)
