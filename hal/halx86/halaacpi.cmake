
list(APPEND HALAACPI_ASM_SOURCE
    apic/apictrap.S)

list(APPEND HALAACPI_SOURCE
    apic/apic.c
    apic/apicacpi.c
    apic/apictimer.c
    apic/halinit.c
    apic/processor.c
    apic/rtctimer.c
    generic/spinlock.c)

add_asm_files(lib_hal_halaacpi_asm ${HALAACPI_ASM_SOURCE})
add_library(lib_hal_halaacpi OBJECT ${HALAACPI_SOURCE} ${lib_hal_halaacpi_asm})
add_dependencies(lib_hal_halaacpi asm bugcodes xdk)
