
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DD3D_COMPILER_VERSION=0)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../d3dcompiler_43
    ${VKD3D_INCLUDE_DIRS})

spec2def(d3d10.dll d3d10.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3d10_main.c
    effect.c
    shader.c
    stateblock.c
    ../d3dcompiler_43/blob.c
    ../d3dcompiler_43/reflection.c
    ../d3dcompiler_43/utils.c)

add_library(d3d10 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10.def)

if(MSVC)
    # C5286: implicit conversion between enum types
    # C4267: effect.c narrows strlen(mask) into a BYTE ComponentCount.
    target_compile_options(d3d10 PRIVATE /wd5286 /wd4267)
endif()

set_module_type(d3d10 win32dll UNICODE)
target_link_libraries(d3d10 wine dxguid dx10guid uuid oldnames)
add_importlibs(d3d10 d3d10core d3dcompiler_39 dxgi d3dwine msvcrt kernel32 ntdll)
add_dependencies(d3d10 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d10 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d10)
