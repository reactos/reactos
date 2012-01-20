
add_library(epsapi
    enum/drivers.c
    enum/modules.c
    enum/processes.c)

add_pch(epsapi enum/precomp.h)
add_dependencies(epsapi psdk)
