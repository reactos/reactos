
list(APPEND HALMACPI_ASM_SOURCE
    apic/apictrap.S)

list(APPEND HALMACPI_SOURCE
    apic/apic.c
    apic/apicacpi.c
    apic/apictimer.c
    apic/halinit.c
    apic/processor.c
    apic/rtctimer.c
    generic/spinlock.c)

add_asm_files(lib_hal_halmacpi_asm ${HALMACPI_ASM_SOURCE})
add_library(lib_hal_halmacpi OBJECT ${HALMACPI_SOURCE} ${lib_hal_halmacpi_asm})
add_dependencies(lib_hal_halmacpi asm bugcodes xdk)
target_compile_definitions(lib_hal_halmacpi PRIVATE CONFIG_SMP)

