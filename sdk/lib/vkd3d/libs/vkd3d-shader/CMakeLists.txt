remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x60)
include_directories(../../include)
include_directories(../../include/private)
include_directories(../../include/vulkan)

FLEX_TARGET(hlsl_scanner hlsl.l ${CMAKE_CURRENT_BINARY_DIR}/hlsl.yy.c)
BISON_TARGET(hlsl_parser hlsl.y ${CMAKE_CURRENT_BINARY_DIR}/hlsl.tab.c)
ADD_FLEX_BISON_DEPENDENCY(hlsl_scanner hlsl_parser)

FLEX_TARGET(preproc_scanner preproc.l ${CMAKE_CURRENT_BINARY_DIR}/preproc.yy.c)
BISON_TARGET(preproc_parser preproc.y ${CMAKE_CURRENT_BINARY_DIR}/preproc.tab.c)
ADD_FLEX_BISON_DEPENDENCY(preproc_scanner preproc_parser)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_hlsl_scanner_OUTPUTS}
    ${BISON_hlsl_parser_OUTPUTS}
    ${FLEX_preproc_scanner_OUTPUTS}
    ${BISON_preproc_parser_OUTPUTS})

list(APPEND VKD3DSHADER_SOURCE
     checksum.c
     d3d_asm.c
     d3dbc.c
     dxbc.c
     dxil.c
     fx.c
     glsl.c
     hlsl_codegen.c
     hlsl_constant_ops.c
     hlsl.c
     ir.c
     msl.c
     spirv.c
     tpf.c
     vkd3d_shader_main.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/string/strnlen.c) #NT5 hacks

add_library(vkd3d-shader ${PCH_SKIP_SOURCE}
                         ${VKD3DSHADER_SOURCE})
target_link_libraries(vkd3d-shader uuid wine)                   
add_dependencies(vkd3d-shader psdk d3d_idl_headers)

if(MSVC)
    target_compile_options(vkd3d-shader PRIVATE /wd4146 /wd4090 /wd4267 /wd4309 /wd4305 /wd4715 /wd4098)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vkd3d-shader PRIVATE -Wno-implicit-function-declaration -Wno-format -Wno-write-strings -Wno-unknown-pragmas -Wno-switch -Wno-format-extra-args -Wno-format-overflow)
endif()
