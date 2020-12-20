
list(APPEND HALMPS_ASM_SOURCE
    apic/apictrap.S
    apic/tsccal.S)

list(APPEND HALMPS_SOURCE
    apic/apic.c
    apic/apicmps.c
    apic/apictimer.c
    apic/halinit.c
    apic/processor.c
    apic/rtctimer.c
    apic/tsc.c
    generic/spinlock.c)

add_asm_files(lib_hal_halmps_asm ${HALMPS_ASM_SOURCE})
add_object_library(lib_hal_halmps ${HALMPS_SOURCE} ${lib_hal_halmps_asm})
add_dependencies(lib_hal_halmps asm bugcodes xdk)
target_compile_definitions(lib_hal_halmps PRIVATE CONFIG_SMP)
