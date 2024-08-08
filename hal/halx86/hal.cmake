include(up.cmake)
include(generic.cmake)
include(pcidata.cmake)
include(legacy.cmake)
include(pic.cmake)

add_library(lib_hal_hal OBJECT
            ${HAL_UP_SOURCE}
            ${HAL_GENERIC_SOURCE}
            ${lib_hal_generic_asm}
            ${HAL_LEGACY_SOURCE}
            ${HAL_PIC_SOURCE}
            ${lib_hal_pic_asm})

add_dependencies(lib_hal_hal bugcodes xdk asm)
target_compile_definitions(lib_hal_hal PRIVATE CONFIG_UP CONFIG_HAL)

