
list(APPEND HALACPI_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    up/pic.S)

list(APPEND HALACPI_SOURCE
    generic/clock.c
    generic/profil.c
    generic/spinlock.c
    generic/timer.c
    up/halinit.c
    up/irql.c
    up/pic.c
    up/processor.c)

add_asm_files(lib_hal_halacpi_asm ${HALACPI_ASM_SOURCE})
add_object_library(lib_hal_halacpi ${HALACPI_SOURCE} ${lib_hal_halacpi_asm})
add_dependencies(lib_hal_halacpi asm bugcodes xdk)
