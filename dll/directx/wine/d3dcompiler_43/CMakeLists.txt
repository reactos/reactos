
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -Dstrcasecmp=_stricmp
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
    utils.c)

FLEX_TARGET(asmshader_scanner asmshader.l ${CMAKE_CURRENT_BINARY_DIR}/asmshader.yy.c)
BISON_TARGET(asmshader_parser asmshader.y ${CMAKE_CURRENT_BINARY_DIR}/asmshader.tab.c COMPILE_FLAGS "-p asmshader_")
ADD_FLEX_BISON_DEPENDENCY(asmshader_scanner asmshader_parser)

FLEX_TARGET(hlsl_scanner hlsl.l ${CMAKE_CURRENT_BINARY_DIR}/hlsl.yy.c)
BISON_TARGET(hlsl_parser hlsl.y ${CMAKE_CURRENT_BINARY_DIR}/hlsl.tab.c COMPILE_FLAGS "-p hlsl_")
ADD_FLEX_BISON_DEPENDENCY(hlsl_scanner hlsl_parser)

FLEX_TARGET(pp_scanner ppl.l ${CMAKE_CURRENT_BINARY_DIR}/ppl.yy.c)
BISON_TARGET(pp_parser ppy.y ${CMAKE_CURRENT_BINARY_DIR}/ppy.tab.c COMPILE_FLAGS "-p ppy_")
ADD_FLEX_BISON_DEPENDENCY(pp_scanner pp_parser)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_asmshader_scanner_OUTPUTS}
    ${BISON_asmshader_parser_OUTPUTS}
    ${FLEX_hlsl_scanner_OUTPUTS}
    ${BISON_hlsl_parser_OUTPUTS}
    ${FLEX_pp_scanner_OUTPUTS}
    ${BISON_pp_parser_OUTPUTS})

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
