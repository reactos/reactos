
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fast486)

list(APPEND SOURCE
    fast486.c
    opcodes.c
    opgroups.c
    extraops.c
    common.c)

add_library(fast486 ${SOURCE})
