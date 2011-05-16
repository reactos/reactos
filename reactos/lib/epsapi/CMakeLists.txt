
add_library(epsapi
    enum/drivers.c
    enum/modules.c
    enum/processes.c)

add_dependencies(epsapi psdk)

