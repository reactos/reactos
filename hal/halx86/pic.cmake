
list(APPEND HAL_PIC_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    pic/pic.S)

list(APPEND HAL_PIC_SOURCE
    generic/clock.c
    generic/profil.c
    generic/timer.c
    pic/halinit.c
    pic/irql.c
    pic/pic.c
    pic/processor.c)

add_asm_files(lib_hal_pic_asm ${HAL_PIC_ASM_SOURCE})
add_library(lib_hal_pic OBJECT ${HAL_PIC_SOURCE} ${lib_hal_pic_asm})
add_dependencies(lib_hal_pic asm)
