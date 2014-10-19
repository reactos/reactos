
include_directories(inc)
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    addr.c
    debug.c
    dnsaddr.c
    dnsutil.c
    flatbuf.c
    hostent.c
    ip6.c
    memory.c
    name.c
    print.c
    record.c
    rrprint.c
    sablob.c
    straddr.c
    string.c
    table.c
    utf8.c)

add_library(dnslib ${SOURCE})
add_pch(dnslib inc/precomp.h)
add_dependencies(dnslib psdk)
