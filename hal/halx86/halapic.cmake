
list(APPEND HALAPIC_ASM_SOURCE
    apic/apictrap.S
    apic/tsccal.S)

list(APPEND HALAPIC_SOURCE
    apic/apic.c
    apic/apicmps.c
    apic/apictimer.c
    apic/halinit.c
    apic/processor.c
    apic/rtctimer.c
    apic/tsc.c
    generic/spinlock.c)

add_asm_files(lib_hal_halapic_asm ${HALAPIC_ASM_SOURCE})
add_library(lib_hal_halapic OBJECT ${HALAPIC_SOURCE} ${lib_hal_halapic_asm})
add_dependencies(lib_hal_halapic asm bugcodes xdk)
