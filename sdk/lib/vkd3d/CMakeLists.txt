
set(VKD3D_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/private
    CACHE INTERNAL "vkd3d public and private include directories")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/private
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/vkd3d
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/vkd3d-shader
    ${CMAKE_CURRENT_BINARY_DIR})

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600
    -D_USE_MATH_DEFINES
    -D_STDSTREAM_DEFINED
    "-Dstdin=(&__iob_func()[0])"
    "-Dstdout=(&__iob_func()[1])"
    "-Dstderr=(&__iob_func()[2])"
    -DWINE_NO_NAMELESS_EXTENSION
    -DLIBVKD3D_SOURCE
    -DLIBVKD3D_SHADER_SOURCE
    -DLIBVKD3D_UTILS_SOURCE
    -DHAVE_CONFIG_H)

list(APPEND SOURCE
    libs/vkd3d-common/blob.c
    libs/vkd3d-common/debug.c
    libs/vkd3d-common/error.c
    libs/vkd3d-common/memory.c
    libs/vkd3d-common/utf8.c
    libs/vkd3d-shader/checksum.c
    libs/vkd3d-shader/d3d_asm.c
    libs/vkd3d-shader/d3dbc.c
    libs/vkd3d-shader/dxbc.c
    libs/vkd3d-shader/dxil.c
    libs/vkd3d-shader/fx.c
    libs/vkd3d-shader/glsl.c
    libs/vkd3d-shader/hlsl.c
    libs/vkd3d-shader/hlsl_codegen.c
    libs/vkd3d-shader/hlsl_constant_ops.c
    libs/vkd3d-shader/ir.c
    libs/vkd3d-shader/msl.c
    libs/vkd3d-shader/spirv.c
    libs/vkd3d-shader/tpf.c
    libs/vkd3d-shader/vkd3d_shader_main.c
    libs/vkd3d/cache.c
    libs/vkd3d/command.c
    libs/vkd3d/device.c
    libs/vkd3d/resource.c
    libs/vkd3d/state.c
    libs/vkd3d/utils.c
    libs/vkd3d/vkd3d_main.c)

# The scanners/parsers carry their own api.prefix (hlsl_yy, preproc_yy) in the
# .l/.y sources, so no -p flag is needed here.
FLEX_TARGET(vkd3d_hlsl_scanner libs/vkd3d-shader/hlsl.l ${CMAKE_CURRENT_BINARY_DIR}/hlsl.yy.c)
BISON_TARGET(vkd3d_hlsl_parser libs/vkd3d-shader/hlsl.y ${CMAKE_CURRENT_BINARY_DIR}/hlsl.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/hlsl.tab.h)
ADD_FLEX_BISON_DEPENDENCY(vkd3d_hlsl_scanner vkd3d_hlsl_parser)

FLEX_TARGET(vkd3d_preproc_scanner libs/vkd3d-shader/preproc.l ${CMAKE_CURRENT_BINARY_DIR}/preproc.yy.c)
BISON_TARGET(vkd3d_preproc_parser libs/vkd3d-shader/preproc.y ${CMAKE_CURRENT_BINARY_DIR}/preproc.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/preproc.tab.h)
ADD_FLEX_BISON_DEPENDENCY(vkd3d_preproc_scanner vkd3d_preproc_parser)

add_library(vkd3d
    ${SOURCE}
    ${FLEX_vkd3d_hlsl_scanner_OUTPUTS}
    ${BISON_vkd3d_hlsl_parser_OUTPUTS}
    ${FLEX_vkd3d_preproc_scanner_OUTPUTS}
    ${BISON_vkd3d_preproc_parser_OUTPUTS})

# Generated scanner/parser sources need the vkd3d-shader source dir on the
# include path to find hlsl.h / preproc.h.
target_include_directories(vkd3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/vkd3d-shader)

if(MSVC)
    # C4090: differing 'const' qualifiers
    # C4200: zero-sized array in struct
    # C4098: 'void' function returning a void expression
    # C4146: unary minus on unsigned, intentional in the bit-twiddling paths.
    # C4245/C4305/C4309: signed/unsigned and float literal conversions in the
    #        shader constant tables.
    # C4267/C4319: align() masks with a size_t-wide value and the SM1 encoder
    #        narrows a token count to 16 bits.
    target_compile_options(vkd3d PRIVATE
        /wd4090 /wd4098 /wd4146 /wd4200 /wd4245 /wd4267 /wd4305 /wd4309 /wd4319)
else()
    target_compile_options(vkd3d PRIVATE -Wno-unused-but-set-variable)
endif()

target_link_libraries(vkd3d oldnames)

add_dependencies(vkd3d psdk d3d_idl_headers)
