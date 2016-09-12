
list(APPEND HAL_UP_SOURCE
    generic/spinlock.c
    up/processor.c)

add_object_library(lib_hal_up ${HAL_UP_SOURCE})
add_dependencies(lib_hal_up bugcodes xdk)
