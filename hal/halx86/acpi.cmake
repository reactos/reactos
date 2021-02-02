
list(APPEND HAL_ACPI_SOURCE
    acpi/halacpi.c
    acpi/halpnpdd.c
    acpi/busemul.c
    legacy/bus/pcibus.c)

add_library(lib_hal_acpi OBJECT ${HAL_ACPI_SOURCE})
add_dependencies(lib_hal_acpi bugcodes xdk)
#add_pch(lib_hal_acpi include/hal.h)

