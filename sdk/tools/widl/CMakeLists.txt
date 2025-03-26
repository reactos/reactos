
if(MSVC)
    if(MSVC_VERSION LESS 1900)
        add_definitions(-Dsnprintf=_snprintf)

        # Add this definition for WDK only, VS 9 doesn't like that
        if(DEFINED ENV{DDKBUILDENV})
            add_definitions(-Dvsnprintf=_vsnprintf)
        endif()
    endif()
endif()

FLEX_TARGET(p_scanner parser.l ${CMAKE_CURRENT_BINARY_DIR}/parser.yy.c)
BISON_TARGET(p_parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c COMPILE_FLAGS "-p parser_")
ADD_FLEX_BISON_DEPENDENCY(p_scanner p_parser)

list(APPEND SOURCE
    client.c
    expr.c
    hash.c
    header.c
    proxy.c
    register.c
    server.c
    typegen.c
    typelib.c
    typetree.c
    utils.c
    widl.c
    write_msft.c
    write_sltg.c
    ${FLEX_p_scanner_OUTPUTS}
    ${BISON_p_parser_OUTPUTS}
    ../port/getopt.c
    ../port/getopt1.c
    ../port/mkstemps.c)

add_definitions(-DINT16=SHORT -DLANG_SCOTTISH_GAELIC=0x91)
add_host_tool(widl ${SOURCE})
target_link_libraries(widl PRIVATE host_includes wpphost)

if(MSVC)
    # Disable warning "'>': signed/unsigned mismatch"
    target_compile_options(widl PRIVATE "/wd4018")

    # Disable warning "unary minus operator applied to unsigned type, result still unsigned"
    target_compile_options(widl PRIVATE "/wd4146")

    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    target_compile_options(widl PRIVATE "/wd4244")

    target_compile_options(widl PRIVATE "-WX")
endif()
