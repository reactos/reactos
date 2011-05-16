
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

target_link_libraries(widl wpp)
