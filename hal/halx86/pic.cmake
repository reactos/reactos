
list(APPEND HAL_PIC_ASM_SOURCE
    pic/pic.S
    pic/systimer.S
    pic/trap.S)

list(APPEND HAL_PIC_SOURCE
    pic/clock.c
    pic/halinit.c
    pic/irql.c
    pic/pic.c
    pic/processor.c
    pic/profil.c
    pic/timer.c)

add_asm_files(lib_hal_pic_asm ${HAL_PIC_ASM_SOURCE})
add_library(lib_hal_pic OBJECT ${HAL_PIC_SOURCE} ${lib_hal_pic_asm})
add_dependencies(lib_hal_pic asm)
