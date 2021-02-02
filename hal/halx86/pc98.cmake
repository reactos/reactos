
# Generic
list(APPEND HAL_PC98_SOURCE
    pc98/beep.c
    pc98/clock.c
    pc98/cmos.c
    pc98/delay.c
    pc98/pic.c
    pc98/profil.c
    pc98/reboot.c
    generic/bios.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/halinit.c
    generic/memory.c
    generic/misc.c
    generic/nmi.c
    generic/portio.c
    generic/sysinfo.c
    generic/usage.c)

list(APPEND HAL_PC98_ASM_SOURCE
    generic/v86.S)

# PIC
list(APPEND HAL_PC98_SOURCE
    pc98/irql.c
    generic/timer.c
    up/halinit_up.c
    up/pic.c)

list(APPEND HAL_PC98_ASM_SOURCE
    generic/trap.S
    up/pic.S)

# Legacy
list(APPEND HAL_PC98_SOURCE
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_classes.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_vendors.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    legacy/halpnpdd.c
    legacy/halpcat.c)

add_asm_files(lib_hal_pc98_asm ${HAL_PC98_ASM_SOURCE})
add_library(lib_hal_pc98 OBJECT ${HAL_PC98_SOURCE} ${lib_hal_pc98_asm})
add_dependencies(lib_hal_pc98 bugcodes xdk asm)
#add_pch(lib_hal_pc98 pc98/halpc98.h)

target_compile_definitions(lib_hal_pc98 PRIVATE SARCH_PC98)
