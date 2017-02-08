
list(APPEND SOURCE
    driverfuncs.c
    meta.c)

add_library(mesa_drv_common STATIC ${SOURCE})
add_dependencies(mesa_drv_common xdk)
