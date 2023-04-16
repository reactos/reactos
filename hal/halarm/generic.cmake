include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_GENERIC_ASM_SOURCE
    generic/arm/cache.S)


list(APPEND HAL_GENERIC_SOURCE
    generic/beep.c
    generic/bus.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/fmutex.c
    generic/halinit.c
    generic/misc.c
    generic/portio.c
    generic/processor.c
    generic/profil.c
    generic/reboot.c
    generic/rtc.c
    generic/spinlock.c
    generic/sysinfo.c
    generic/timer.c)

add_asm_files(lib_hal_generic_asm ${HAL_GENERIC_ASM_SOURCE})
add_library(lib_hal_generic OBJECT ${HAL_GENERIC_SOURCE} ${lib_hal_generic_asm})
add_dependencies(lib_hal_generic asm)
