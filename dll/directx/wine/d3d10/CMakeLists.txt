
add_definitions(
    -D__WINESRC__
    -Dstricmp=_stricmp
    -DD3D_COMPILER_VERSION=0
    -Dstrdup=_strdup
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)

include_directories(${REACTOS_SOURCE_DIR}/dll/directx/wine/d3dcompiler_43/)

list(APPEND SOURCE
    ../d3dcompiler_43/asmparser.c
    ../d3dcompiler_43/blob.c
    ../d3dcompiler_43/bytecodewriter.c
    ../d3dcompiler_43/compiler.c
    ../d3dcompiler_43/reflection.c
    ../d3dcompiler_43/utils.c)

FLEX_TARGET(asmshader_scanner ../d3dcompiler_43/asmshader.l ${CMAKE_CURRENT_BINARY_DIR}/asmshader.yy.c)
BISON_TARGET(asmshader_parser ../d3dcompiler_43/asmshader.y ${CMAKE_CURRENT_BINARY_DIR}/asmshader.tab.c)
ADD_FLEX_BISON_DEPENDENCY(asmshader_scanner asmshader_parser)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_asmshader_scanner_OUTPUTS}
    ${BISON_asmshader_parser_OUTPUTS})

spec2def(d3d10.dll d3d10.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3d10_main.c
    effect.c
    shader.c
    stateblock.c
    d3d10_private.h)

add_library(d3d10 SHARED
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3d10 PRIVATE -Wno-incompatible-pointer-types -Wno-unused-but-set-variable -Wno-switch -Wno-error) # Our favourite compiler :)
endif()
if(MSVC)
    target_compile_options(d3d10 PRIVATE /wd4267)
endif()

set_module_type(d3d10 win32dll)
target_link_libraries(d3d10 wine dx10guid dxguid uuid)
add_importlibs(d3d10 d3d11 d3dwine msvcrt d3dcompiler_39 d3d10core dxgi kernel32 ntdll)
add_pch(d3d10 d3d10_private.h SOURCE)
add_dependencies(d3d10 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d10 DESTINATION reactos/system32 FOR all)