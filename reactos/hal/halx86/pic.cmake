
list(APPEND HAL_PIC_ASM_SOURCE
    generic/systimer.S
    generic/trap.S)

list(APPEND HAL_PIC_SOURCE
    generic/profil.c
    generic/timer.c
    up/halinit_up.c
    up/pic.c)

add_object_library(lib_hal_pic ${HAL_PIC_SOURCE} ${HAL_PIC_ASM_SOURCE})
add_dependencies(lib_hal_pic asm)
