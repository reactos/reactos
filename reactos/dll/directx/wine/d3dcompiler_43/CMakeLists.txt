
add_definitions(
    -D__WINESRC__
    -DDIRECT3D_VERSION=0x0900)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(d3dcompiler_43.dll d3dcompiler_43.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    asmparser.c
    asmshader.tab.c
    asmshader.yy.c
    blob.c
    bytecodewriter.c
    compiler.c
    hlsl.tab.c
    hlsl.yy.c
    main.c
    reflection.c
    utils.c
    d3dcompiler_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43_stubs.c)

add_library(d3dcompiler_43 SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43.def)

set_module_type(d3dcompiler_43 win32dll)
target_link_libraries(d3dcompiler_43 dx10guid uuid wine wpp)
add_importlibs(d3dcompiler_43 msvcrt kernel32 ntdll)
add_dependencies(d3dcompiler_43 d3d_idl_headers)
add_pch(d3dcompiler_43 d3dcompiler_private.h SOURCE)
add_cd_file(TARGET d3dcompiler_43 DESTINATION reactos/system32 FOR all)
