
list(APPEND HAL_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    pic/pic.S)

list(APPEND HAL_SOURCE
    generic/clock.c
    generic/profil.c
    generic/timer.c
    pic/halinit.c
    pic/irql.c
    pic/legacy.c
    pic/pic.c
    pic/processor.c)

add_asm_files(lib_hal_hal_asm ${HAL_ASM_SOURCE})
add_library(lib_hal_hal OBJECT ${HAL_SOURCE} ${lib_hal_hal_asm})
add_dependencies(lib_hal_hal asm bugcodes xdk)
