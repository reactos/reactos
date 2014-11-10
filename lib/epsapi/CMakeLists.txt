
list(APPEND SOURCE
    enum/drivers.c
    enum/modules.c
    enum/processes.c
    enum/precomp.h)

add_library(epsapi ${SOURCE})
add_pch(epsapi enum/precomp.h SOURCE)
add_dependencies(epsapi psdk)
