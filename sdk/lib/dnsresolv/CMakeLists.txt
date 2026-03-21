
list(APPEND SOURCE
    dnsresolv.c
    dnsresolv.h)

add_library(dnsresolv ${SOURCE})
add_dependencies(dnsresolv psdk)
