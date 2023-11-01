include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_ARMTIMER_SOURCE
     timers/generic/generic.c)

add_library(lib_hal_armtimer OBJECT ${HAL_ARMTIMER_SOURCE})

