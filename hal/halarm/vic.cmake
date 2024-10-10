

list(APPEND HAL_VIC_SOURCE
    vic/vic.c
    gic/halinit_up.c)

add_library(lib_hal_vic OBJECT ${HAL_VIC_SOURCE})
add_pch(lib_hal_vic include/hal.h ${HAL_VIC_SOURCE})
add_dependencies(lib_hal_vic bugcodes xdk)
