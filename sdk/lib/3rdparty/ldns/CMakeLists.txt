
include_directories(
    ldns)

add_definitions(
    -D_DLL -D__USE_CRTIMP
    -DADNS_JGAA_WIN32
    -D_CRT_NO_POSIX_ERROR_CODES)

list(APPEND SOURCE
    buffer.c
    dname.c
    duration.c
    edns.c
    error.c
    higher.c
    host2str.c
    linktest.c
    net.c
    packet.c
    parse.c
    radix.c
    rbtree.c
    rdata.c
    resolver.c
    rr.c
    rr_functions.c
    str2host.c
    update.c
    wire2host.c
    zone.c)
        

add_library(ldns ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ldns PRIVATE -Wno-unused-but-set-variable)
endif()

add_dependencies(ldns psdk)
