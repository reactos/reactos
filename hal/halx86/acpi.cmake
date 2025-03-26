
include_directories(include ${REACTOS_SOURCE_DIR}/drivers/bus/acpi/acpica/include)

list(APPEND HAL_ACPI_SOURCE
    acpi/halacpi.c
    acpi/halpnpdd.c
    acpi/busemul.c
    acpi/madt.c
    legacy/bus/pcibus.c)

# Needed to compile while using ACPICA
if(ARCH STREQUAL "amd64")
    add_definitions(-DWIN64)
endif()

add_library(lib_hal_acpi OBJECT ${HAL_ACPI_SOURCE})
add_pch(lib_hal_acpi include/hal.h ${HAL_ACPI_SOURCE})
add_dependencies(lib_hal_acpi bugcodes xdk)
