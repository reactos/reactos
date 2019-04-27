
if(MSVC)
    if(MSVC_VERSION LESS 1900)
        add_definitions(-Dsnprintf=_snprintf)

        # Add this definition for WDK only, VS 9 doesn't like that
        if(DEFINED ENV{DDKBUILDENV})
            add_definitions(-Dvsnprintf=_vsnprintf)
        endif()
    endif()
endif()

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
    parser.yy.c
    parser.tab.c
    ../port/getopt.c
    ../port/getopt1.c
    ../port/mkstemps.c)

# Taken from widl.rbuild
add_definitions(-DINT16=SHORT)
add_host_tool(widl ${SOURCE})
target_link_libraries(widl wpphost)

if(MSVC)
    # Disable warning "'>': signed/unsigned mismatch"
    add_target_compile_flags(widl "/wd4018")

    # Disable warning "unary minus operator applied to unsigned type, result still unsigned"
    add_target_compile_flags(widl "/wd4146")

    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    add_target_compile_flags(widl "/wd4244")
endif()
