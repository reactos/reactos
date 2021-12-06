
list(APPEND HAL_SMP_SOURCE
    generic/buildtype.c
    generic/spinlock.c
    smp/smp.c
    smp/ipi.c)

if(ARCH STREQUAL "i386")
    list(APPEND HAL_SMP_ASM_SOURCE
        smp/i386/apentry.S)
endif()

add_asm_files(lib_hal_smp_asm ${HAL_SMP_ASM_SOURCE})
add_library(lib_hal_smp OBJECT ${HAL_SMP_SOURCE} ${lib_hal_smp_asm})
add_dependencies(lib_hal_smp bugcodes xdk)
target_compile_definitions(lib_hal_smp PRIVATE CONFIG_SMP)
