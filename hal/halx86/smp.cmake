
list(APPEND HAL_SMP_SOURCE
    generic/buildtype.c
    generic/spinlock.c
    smp/smp.c)

add_library(lib_hal_smp OBJECT ${HAL_SMP_SOURCE})
add_dependencies(lib_hal_smp bugcodes xdk)
target_compile_definitions(lib_hal_smp PRIVATE CONFIG_SMP)
