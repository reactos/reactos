include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_ARMTIMER_ASM_SOURCE
    timers/generic/timer.S)

list(APPEND HAL_ARMTIMER_SOURCE
     timers/generic/generic.c
     timers/generic/armhw.c)

add_asm_files(lib_hal_armtimer_asm ${HAL_ARMTIMER_ASM_SOURCE})
add_library(lib_hal_armtimer OBJECT ${HAL_ARMTIMER_SOURCE} ${lib_hal_armtimer_asm})
add_dependencies(lib_hal_armtimer asm)
