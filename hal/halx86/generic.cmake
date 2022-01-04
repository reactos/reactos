
list(APPEND HAL_GENERIC_SOURCE
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
    generic/reboot.c
    generic/sysinfo.c
    generic/usage.c)

if(ARCH STREQUAL "i386")
    list(APPEND HAL_GENERIC_SOURCE
        generic/bios.c
        generic/portio.c)

    list(APPEND HAL_GENERIC_ASM_SOURCE
        generic/v86.S)
endif()

add_asm_files(lib_hal_generic_asm ${HAL_GENERIC_ASM_SOURCE})
add_library(lib_hal_generic OBJECT ${HAL_GENERIC_SOURCE} ${lib_hal_generic_asm})
add_dependencies(lib_hal_generic asm)
