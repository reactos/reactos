
include_directories(
    ldns)

add_definitions(
    -D_DLL -D__USE_CRTIMP
    -DADNS_JGAA_WIN32
    -D_CRT_NO_POSIX_ERROR_CODES)

list(APPEND SOURCE
    buffer.c
    compat/b64_ntop.c
    compat/b64_pton.c
    compat/gettimeofday.c
    compat/inet_ntop.c
    compat/inet_pton.c
    compat/strlcpy.c
    dname.c
    dnssec_sign.c
    dnssec_verify.c
    dnssec_zone.c
    dnssec.c
    duration.c
    edns.c
    error.c
    higher.c
    host2str.c
    host2wire.c
    keys.c
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
    sha1.c
    sha2.c
    str2host.c
    update.c
    util.c
    wire2host.c
    zone.c)
        

add_library(ldns ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ldns PRIVATE -Wno-unused-but-set-variable)
endif()

add_dependencies(ldns psdk)
