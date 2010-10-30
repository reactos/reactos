
if(ARCH MATCHES i386)

add_definitions(-D_NTHALDLL_)
add_definitions(-D_NTHAL_)

include_directories(halx86/include)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_GENERIC_SOURCE
    halx86/generic/beep.c
    halx86/generic/cmos.c
    halx86/generic/display.c
    halx86/generic/dma.c
    halx86/generic/drive.c
    halx86/generic/memory.c
    halx86/generic/misc.c
    halx86/generic/profil.c
    halx86/generic/reboot.c
    halx86/generic/sysinfo.c
    halx86/generic/timer.c
    halx86/generic/usage.c)

list(APPEND HAL_GENERIC_SOURCE
    halx86/generic/bios.c
    halx86/generic/halinit.c
    halx86/generic/i386/portio.c
    halx86/generic/i386/systimer.S
    halx86/generic/i386/trap.S)

list(APPEND HAL_GENERIC_PCAT_SOURCE
    halx86/generic/legacy/bus/bushndlr.c
    halx86/generic/legacy/bus/cmosbus.c
    halx86/generic/legacy/bus/isabus.c
    halx86/generic/legacy/bus/pcibus.c
    halx86/generic/legacy/bus/pcidata.c
    halx86/generic/legacy/bus/sysbus.c
    halx86/generic/legacy/bussupp.c
    halx86/generic/legacy/halpcat.c)

list(APPEND HAL_GENERIC_UP_SOURCE
    halx86/generic/spinlock.c
    halx86/up/pic.c
    halx86/up/processor.c)



list(APPEND HAL_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/hal_hal.h.gch
    ${HAL_GENERIC_SOURCE}
    ${HAL_GENERIC_PCAT_SOURCE}
    ${HAL_GENERIC_UP_SOURCE}
    halx86/up/halinit_up.c
    halx86/up/halup.rc)

add_library(hal SHARED ${HAL_SOURCE})
add_pch(hal ${CMAKE_CURRENT_SOURCE_DIR}/halx86/include/hal.h ${HAL_SOURCE})
add_dependencies(hal psdk bugcodes)

set_target_properties(hal PROPERTIES LINK_FLAGS "-Wl,-entry,_HalInitSystem@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native")

target_link_libraries(hal
    ${CMAKE_CURRENT_SOURCE_DIR}/halx86/hal_i386.def
    -lntoskrnl
    libcntpr)

list(APPEND MINI_HAL_SOURCE
    halx86/generic/legacy/bus/bushndlr.c
    halx86/generic/legacy/bus/cmosbus.c
    halx86/generic/legacy/bus/isabus.c
    halx86/generic/legacy/bus/pcibus.c
    halx86/generic/legacy/bus/pcidata.c
    halx86/generic/legacy/bus/sysbus.c
    halx86/generic/legacy/bussupp.c
    halx86/generic/beep.c
    halx86/generic/bios.c
    halx86/generic/cmos.c
    halx86/generic/dma.c
    halx86/generic/display.c
    halx86/generic/drive.c
    halx86/generic/misc.c
    halx86/generic/profil.c
    halx86/generic/reboot.c
    halx86/generic/spinlock.c
    halx86/generic/sysinfo.c
    halx86/generic/timer.c
    halx86/generic/usage.c
    halx86/up/halinit_up.c
    halx86/up/pic.c
    halx86/up/processor.c)

if(ARCH MATCHES i386)
list(APPEND MINI_HAL_SOURCE
    halx86/generic/i386/portio.c
    halx86/generic/i386/systimer.S)
endif(ARCH MATCHES i386)

add_library(mini_hal ${MINI_HAL_SOURCE})
set_property(TARGET mini_hal PROPERTY COMPILE_DEFINITIONS "_BLDR_;_MINIHAL_")
add_dependencies(mini_hal psdk bugcodes)
add_minicd_target(hal reactos hal.dll)
add_livecd_target(hal reactos/system32)
else()
#TBD
endif(ARCH MATCHES i386)