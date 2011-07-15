
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    compses.c
    connect.c
    execpgm.c
    lookupss.c)
    
add_library(smlib ${SOURCE})
add_dependencies(smlib psdk)
