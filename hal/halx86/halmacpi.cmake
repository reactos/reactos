include(smp.cmake)
include(generic.cmake)
include(acpi.cmake)
include(apic.cmake)

add_library(lib_hal_halmacpi OBJECT
            ${HAL_SMP_SOURCE}
            ${lib_hal_smp_asm}
            ${HAL_GENERIC_SOURCE}
            ${lib_hal_generic_asm}
            ${HAL_ACPI_SOURCE}
            ${HAL_APIC_SOURCE}
            ${lib_hal_apic_asm})

add_dependencies(lib_hal_halmacpi bugcodes xdk asm)
target_compile_definitions(lib_hal_halmacpi PRIVATE CONFIG_SMP CONFIG_HALMACPI)

