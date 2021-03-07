
list(APPEND HALACPI_ASM_SOURCE
#    generic/systimer.S
    generic/trap.S
    pic/pic.S)

list(APPEND HALACPI_SOURCE
    generic/clock.c
    generic/profil.c
    generic/spinlock.c
    generic/timer.c
    pic/acpi.c
    pic/halinit.c
    pic/irql.c
    pic/pic.c
    pic/processor.c)

add_asm_files(lib_hal_halacpi_asm ${HALACPI_ASM_SOURCE})
add_library(lib_hal_halacpi OBJECT ${HALACPI_SOURCE} ${lib_hal_halacpi_asm})
add_dependencies(lib_hal_halacpi asm bugcodes xdk)
