
list(APPEND HAL_APIC_ASM_SOURCE
    apic/apictrap.S
    apic/tsccal.S)

list(APPEND HAL_APIC_SOURCE
    apic/apic.c
    apic/apictimer.c
    apic/halinit_apic.c
    apic/rtctimer.c
    apic/tsc.c)

add_object_library(lib_hal_apic ${HAL_APIC_SOURCE} ${HAL_APIC_ASM_SOURCE})
add_dependencies(lib_hal_apic asm)
