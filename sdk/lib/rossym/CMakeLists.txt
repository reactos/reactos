
add_definitions(-D_NTSYSTEM_)
list(APPEND SOURCE
    delete.c
    find.c
    fromfile.c
    frommem.c
    fromraw.c
    getraw.c
    init.c
    initkm.c
    initum.c
    zwfile.c)
add_library(rossym ${SOURCE})
add_dependencies(rossym psdk bugcodes)
target_compile_definitions(rossym INTERFACE __ROS_ROSSYM__)
