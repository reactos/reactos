
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DDIRECT3D_VERSION=0x0900)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3dcompiler_43.dll d3dcompiler_43.spec ADD_IMPORTLIB)

add_bison_files(asmshader.y hlsl.y ppy.y)
add_flex_files(asmshader.l hlsl.l ppl.l)

list(APPEND SOURCE
    asmparser.c
    blob.c
    bytecodewriter.c
    compiler.c
    main.c
    preproc.c
    reflection.c
    utils.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/asmshader.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/asmshader.yy.c
    ${CMAKE_CURRENT_BINARY_DIR}/hlsl.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/hlsl.yy.c
    ${CMAKE_CURRENT_BINARY_DIR}/ppy.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/ppl.yy.c)

add_library(d3dcompiler_43 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43.def)

set_module_type(d3dcompiler_43 win32dll)
target_link_libraries(d3dcompiler_43 dx10guid uuid wine wpp)
add_importlibs(d3dcompiler_43 msvcrt kernel32 ntdll)
add_dependencies(d3dcompiler_43 d3d_idl_headers)
add_pch(d3dcompiler_43 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET d3dcompiler_43 DESTINATION reactos/system32 FOR all)
