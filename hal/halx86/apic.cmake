
list(APPEND HAL_APIC_ASM_SOURCE
    apic/apictrap.S
    apic/tsccal.S)

list(APPEND HAL_APIC_SOURCE
    apic/apic.c
    apic/apictimer.c
    apic/halinit_apic.c
    apic/rtctimer.c
    apic/tsc.c
    include/hal.h)

add_asm_files(lib_hal_apic_asm ${HAL_APIC_ASM_SOURCE})
add_object_library(lib_hal_apic ${HAL_APIC_SOURCE} ${lib_hal_apic_asm})
add_pch(lib_hal_apic include/hal.h HAL_APIC_SOURCE UNITY_BUILD)
add_dependencies(lib_hal_apic asm)
