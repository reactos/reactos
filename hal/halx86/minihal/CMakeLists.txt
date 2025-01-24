
# We need to define this here, because the target definitions are not
# passed to CL, when preprocessing the asm files on MSVC builds.
# See CORE-19847
add_definitions(-D_MINIHAL_)

list(APPEND MINI_HAL_SOURCE
    ../generic/portio.c
    ../legacy/bus/bushndlr.c
    ../legacy/bus/cmosbus.c
    ../legacy/bus/isabus.c
    ../legacy/bus/pcibus.c
    ../legacy/bussupp.c
    ../generic/dma.c
    ../generic/drive.c
    ../generic/misc.c
    ../generic/nmi.c
    ../generic/spinlock.c
    ../generic/timer.c
    ../generic/usage.c
    ../pic/pic.c
    ../include/hal.h
    halinit.c)

if(SARCH STREQUAL "xbox")
    list(APPEND MINI_HAL_SOURCE
        ../generic/beep.c
        ../generic/cmos.c
        ../xbox/clock.c
        ../xbox/reboot.c)
elseif(SARCH STREQUAL "pc98")
    list(APPEND MINI_HAL_SOURCE
        ../pc98/beep.c
        ../pc98/clock.c
        ../pc98/cmos.c
        ../pc98/reboot.c)
else()
    list(APPEND MINI_HAL_SOURCE
        ../generic/beep.c
        ../generic/clock.c
        ../generic/cmos.c
        ../generic/reboot.c)
endif()

add_asm_files(mini_hal_asm ../generic/systimer.S)
add_library(mini_hal ${MINI_HAL_SOURCE} ${mini_hal_asm})
target_compile_definitions(mini_hal PRIVATE _BLDR_ _NTSYSTEM_)
add_dependencies(mini_hal psdk bugcodes asm)
add_pch(mini_hal ../include/hal.h MINI_HAL_SOURCE)
