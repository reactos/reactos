
list(APPEND HAL_PIC_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    up/pic.S)

list(APPEND HAL_PIC_SOURCE
    generic/profil.c
    generic/timer.c
    up/halinit_up.c
    up/pic.c)

add_asm_files(lib_hal_pic_asm ${HAL_PIC_ASM_SOURCE})
add_object_library(lib_hal_pic ${HAL_PIC_SOURCE} ${lib_hal_pic_asm})
add_dependencies(lib_hal_pic asm)
