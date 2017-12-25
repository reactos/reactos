
list(APPEND HAL_UP_SOURCE
    generic/spinlock.c
    up/processor.c
    include/hal.h)

add_object_library(lib_hal_up ${HAL_UP_SOURCE})
add_pch(lib_hal_up include/hal.h HAL_UP_SOURCE UNITY_BUILD)
add_dependencies(lib_hal_up bugcodes xdk)
