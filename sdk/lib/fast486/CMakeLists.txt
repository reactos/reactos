
list(APPEND SOURCE
    debug.c
    fast486.c
    opcodes.c
    opgroups.c
    extraops.c
    common.c
    fpu.c)

add_library(fast486 STATIC ${SOURCE})
add_dependencies(fast486 xdk)
target_include_directories(fast486 PUBLIC ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/fast486)
