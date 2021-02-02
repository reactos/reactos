
list(APPEND HAL_PIC_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    up/pic.S)

list(APPEND HAL_PIC_SOURCE
    generic/clock.c
    generic/profil.c
    generic/timer.c
    up/halinit_up.c
    up/irql.c
    up/pic.c)

add_asm_files(lib_hal_pic_asm ${HAL_PIC_ASM_SOURCE})
add_library(lib_hal_pic OBJECT ${HAL_PIC_SOURCE} ${lib_hal_pic_asm})
add_dependencies(lib_hal_pic asm)
