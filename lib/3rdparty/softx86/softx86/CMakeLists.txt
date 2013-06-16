
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/softx86)

list(APPEND SOURCE
    aaa.c
    add.c
    binops.c
    cbw.c
    clc.c
    drooling_duh.c
    fpu.c
    groupies.c
    inc.c
    interrupts.c
    ioport.c
    jumpy.c
    mov.c
    optable.c
    prefixes.c
    procframe.c
    pushpop.c
    shovel.c
    softx86.c
### Uncomment the following files for dll compilation:
#   softx86_dll.def
#   softx86_dll.rc
)

add_library(softx86 ${SOURCE})
allow_warnings(softx86)
