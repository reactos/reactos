

list(APPEND MINI_HAL_SOURCE
    generic/portio.c
    generic/systimer.S
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    generic/beep.c
    generic/bios.c
    generic/cmos.c
    generic/dma.c
    generic/display.c
    generic/drive.c
    generic/misc.c
    generic/profil.c
    generic/reboot.c
    generic/spinlock.c
    generic/sysinfo.c
    generic/timer.c
    generic/usage.c
    up/halinit_mini.c
    up/pic.c
    up/processor.c)

# mini_hal
add_library(mini_hal ${MINI_HAL_SOURCE})
add_target_compile_definitions(mini_hal _BLDR_ _MINIHAL_)
add_dependencies(mini_hal psdk bugcodes asm)
