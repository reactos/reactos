
if(MSVC)
    if(MSVC_VERSION LESS 1900)
        add_definitions(-Dsnprintf=_snprintf)

        # Add this definition for WDK only, VS 9 doesn't like that
        if(DEFINED ENV{DDKBUILDENV})
            add_definitions(-Dvsnprintf=_vsnprintf)
        endif()
    endif()

    list(APPEND SOURCE getopt.c)
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
    parser.yy.c
    parser.tab.c
    port/mkstemps.c)

# Taken from widl.rbuild
add_definitions(-DINT16=SHORT)
add_executable(widl ${SOURCE})
target_link_libraries(widl wpphost)
