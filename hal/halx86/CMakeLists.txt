
add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_GENERIC_SOURCE
    generic/beep.c
    generic/cmos.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/memory.c
    generic/misc.c
    generic/profil.c
    generic/reboot.c
    generic/sysinfo.c
    generic/timer.c
    generic/usage.c)

if(ARCH MATCHES i386)
    list(APPEND HAL_GENERIC_SOURCE
        generic/bios.c
        generic/halinit.c
        generic/i386/portio.c
        generic/i386/systimer.S
        generic/i386/trap.S)
endif()

list(APPEND HAL_GENERIC_PCAT_SOURCE
    generic/legacy/bus/bushndlr.c
    generic/legacy/bus/cmosbus.c
    generic/legacy/bus/isabus.c
    generic/legacy/bus/pcibus.c
    generic/legacy/bus/pcidata.c
    generic/legacy/bus/sysbus.c
    generic/legacy/bussupp.c
    generic/legacy/halpcat.c)

list(APPEND HAL_GENERIC_ACPI_SOURCE
    generic/acpi/halacpi.c
    generic/acpi/halpnpdd.c
    generic/acpi/busemul.c
    generic/legacy/bus/pcibus.c)

if(ARCH MATCHES i386)

    list(APPEND HAL_GENERIC_UP_SOURCE
        generic/spinlock.c
        up/pic.c
        up/processor.c)

    list(APPEND MINI_HAL_SOURCE
        generic/i386/portio.c
        generic/i386/systimer.S
        generic/legacy/bus/bushndlr.c
        generic/legacy/bus/cmosbus.c
        generic/legacy/bus/isabus.c
        generic/legacy/bus/pcibus.c
        generic/legacy/bus/sysbus.c
        generic/legacy/bussupp.c
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
        up/halinit_up.c
        up/pic.c
        up/processor.c)

    spec2def(hal.dll ../hal.spec)

    list(APPEND HAL_SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/hal_hal.h.gch
        ${HAL_GENERIC_SOURCE}
        ${HAL_GENERIC_PCAT_SOURCE}
        ${HAL_GENERIC_UP_SOURCE}
        up/halinit_up.c
        up/halup.rc
        ${CMAKE_CURRENT_BINARY_DIR}/hal.def)

    # hal
    add_library(hal SHARED ${HAL_SOURCE})
    add_pch(hal ${CMAKE_CURRENT_SOURCE_DIR}/include/hal.h ${HAL_SOURCE})
    add_dependencies(hal psdk bugcodes)

    set_entrypoint(hal HalInitSystem@8)
    set_subsystem(hal native)
    set_image_base(hal 0x00010000)
    add_importlibs(hal ntoskrnl)
    target_link_libraries(hal libcntpr)

    add_cd_file(TARGET hal DESTINATION reactos/system32 NO_CAB FOR all)

    # mini_hal
    add_library(mini_hal ${MINI_HAL_SOURCE})
    set_property(TARGET mini_hal PROPERTY COMPILE_DEFINITIONS "_BLDR_;_MINIHAL_")
    add_dependencies(mini_hal psdk bugcodes asm)

elseif(ARCH MATCHES amd64)
#TBD
endif()

add_importlib_target(../hal.spec)
