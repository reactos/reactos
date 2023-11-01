include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_SP804_SOURCE
     timers/sp804/timer.c)

add_library(lib_hal_sp804 OBJECT ${HAL_SP804_SOURCE})

