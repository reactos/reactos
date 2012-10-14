
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    compses.c
    connect.c
    execpgm.c
    lookupss.c
    smclient.c)
    
add_library(smlib ${SOURCE})
add_pch(smlib precomp.h)
add_dependencies(smlib psdk)
