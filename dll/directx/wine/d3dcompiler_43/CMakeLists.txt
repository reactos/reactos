
add_definitions(
    -D__WINESRC__
    -DDIRECT3D_VERSION=0x0900)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3dcompiler_43.dll d3dcompiler_43.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    asmparser.c
    blob.c
    bytecodewriter.c
    compiler.c
    main.c
    preproc.c
    reflection.c
    utils.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43_stubs.c)

add_library(d3dcompiler_43 MODULE
    ${SOURCE}
    asmshader.tab.c
    asmshader.yy.c
    hlsl.tab.c
    hlsl.yy.c
    ppy.tab.c
    ppl.yy.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43.def)

# some files have been generated with relative file paths...
set_source_files_properties(
    asmshader.tab.c
    asmshader.yy.c
    hlsl.tab.c
    hlsl.yy.c
    ppy.tab.c
    ppl.yy.c
    PROPERTIES COMPILE_FLAGS "-UREACTOS_SOURCE_DIR -DREACTOS_SOURCE_DIR=\"\\\".\\\"\"")

set_module_type(d3dcompiler_43 win32dll)
target_link_libraries(d3dcompiler_43 dx10guid uuid wine wpp)
add_importlibs(d3dcompiler_43 msvcrt kernel32 ntdll)
add_dependencies(d3dcompiler_43 d3d_idl_headers)
add_pch(d3dcompiler_43 precomp.h SOURCE)
add_cd_file(TARGET d3dcompiler_43 DESTINATION reactos/system32 FOR all)
