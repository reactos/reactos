
list(APPEND HAL_XBOX_ASM_SOURCE
    generic/systimer.S
    generic/trap.S
    generic/v86.S
    up/pic.S)

list(APPEND HAL_XBOX_SOURCE
    generic/beep.c
    generic/cmos.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/halinit.c
    generic/memory.c
    generic/misc.c
    generic/nmi.c
    generic/pic.c
    generic/sysinfo.c
    generic/usage.c
    generic/bios.c
    generic/portio.c
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_classes.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_vendors.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    legacy/halpnpdd.c
    legacy/halpcat.c
    generic/profil.c
    generic/timer.c
    xbox/clock.c
    xbox/part_xbox.c
    xbox/halinit_xbox.c
    xbox/reboot.c
    up/irql.c
    up/pic.c)

add_asm_files(lib_hal_xbox_asm ${HAL_XBOX_ASM_SOURCE})
add_library(lib_hal_xbox OBJECT ${HAL_XBOX_SOURCE} ${lib_hal_xbox_asm})
add_dependencies(lib_hal_xbox bugcodes xdk asm)
#add_pch(lib_hal_xbox xbox/halxbox.h)
target_compile_definitions(lib_hal_xbox PRIVATE SARCH_XBOX)
