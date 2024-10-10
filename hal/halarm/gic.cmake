

list(APPEND HAL_GIC_SOURCE
    gic/gic.c
    gic/halinit_up.c)

# Needed to compile while using ACPICA
if(ARCH STREQUAL "arm64")
    add_definitions(-DWIN64)
endif()

add_library(lib_hal_gic OBJECT ${HAL_GIC_SOURCE})
add_pch(lib_hal_gic include/hal.h ${HAL_GIC_SOURCE})
add_dependencies(lib_hal_gic bugcodes xdk)
