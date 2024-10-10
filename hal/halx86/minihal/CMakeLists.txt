
list(APPEND MINI_HAL_SOURCE
    ../generic/dma.c
    ../generic/drive.c
    ../generic/misc.c
    ../generic/nmi.c
    ../generic/spinlock.c
    ../generic/timer.c
    ../generic/usage.c
    ../legacy/bus/pcibus.c
    ../include/hal.h
    halinit.c)

if(ARCH STREQUAL "i386")
    list(APPEND MINI_HAL_SOURCE
        ../legacy/bussupp.c
        ../legacy/bus/bushndlr.c
        ../legacy/bus/cmosbus.c
        ../legacy/bus/isabus.c)
else()
    list(APPEND MINI_HAL_SOURCE
        ../acpi/busemul.c)
endif()

if(ARCH STREQUAL "i386")
    list(APPEND MINI_HAL_SOURCE
        ../generic/portio.c
        ../pic/pic.c)
    list(APPEND MINI_HAL_ASM_SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/i386/ioport.S)

    if(SARCH STREQUAL "xbox")
        list(APPEND MINI_HAL_SOURCE
            ../generic/beep.c
            ../generic/cmos.c
            ../xbox/clock.c
            ../xbox/reboot.c)
        list(APPEND MINI_HAL_ASM_SOURCE
            ../generic/systimer.S)
    elseif(SARCH STREQUAL "pc98")
        list(APPEND MINI_HAL_SOURCE
            ../pc98/beep.c
            ../pc98/clock.c
            ../pc98/cmos.c
            ../pc98/delay.c
            ../pc98/reboot.c)
    else()
        list(APPEND MINI_HAL_SOURCE
            ../generic/beep.c
            ../generic/clock.c
            ../generic/cmos.c
            ../generic/reboot.c)
        list(APPEND MINI_HAL_ASM_SOURCE
            ../generic/systimer.S)
    endif()
else()
    list(APPEND MINI_HAL_SOURCE
        ../generic/beep.c
        ../generic/clock.c
        ../generic/cmos.c
        ../generic/reboot.c)
endif()

add_asm_files(mini_hal_asm ${MINI_HAL_ASM_SOURCE})
add_library(mini_hal ${MINI_HAL_SOURCE} ${mini_hal_asm})
target_compile_definitions(mini_hal PRIVATE _MINIHAL_ _BLDR_ _NTSYSTEM_)
add_dependencies(mini_hal psdk bugcodes asm)
add_pch(mini_hal ../include/hal.h MINI_HAL_SOURCE)
