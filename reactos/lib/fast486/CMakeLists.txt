
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fast486)

list(APPEND SOURCE
    debug.c
    fast486.c
    opcodes.c
    opgroups.c
    extraops.c
    common.c
    fpu.c)

add_library(fast486 ${SOURCE})
add_dependencies(fast486 xdk)
