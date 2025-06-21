
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -Dstrdup=_strdup
    -Dstricmp=_stricmp
    -DDIRECT3D_VERSION=0x0900)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3dcompiler_43.dll d3dcompiler_43.spec ADD_IMPORTLIB)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)

list(APPEND SOURCE
    asmparser.c
    blob.c
    bytecodewriter.c
    compiler.c
    reflection.c
    utils.c)

FLEX_TARGET(asmshader_scanner asmshader.l ${CMAKE_CURRENT_BINARY_DIR}/asmshader.yy.c)
BISON_TARGET(asmshader_parser asmshader.y ${CMAKE_CURRENT_BINARY_DIR}/asmshader.tab.c)
ADD_FLEX_BISON_DEPENDENCY(asmshader_scanner asmshader_parser)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_asmshader_scanner_OUTPUTS}
    ${BISON_asmshader_parser_OUTPUTS})

add_library(d3dcompiler_43 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_43.def)

set_module_type(d3dcompiler_43 win32dll)
target_link_libraries(d3dcompiler_43 vkd3d-shader vkd3d vkd3d-common dx10guid uuid wine wpp)
add_importlibs(d3dcompiler_43 msvcrt ucrtbase kernel32_vista kernel32 ntdll)
add_dependencies(d3dcompiler_43 d3d_idl_headers)
add_cd_file(TARGET d3dcompiler_43 DESTINATION reactos/system32 FOR all)
