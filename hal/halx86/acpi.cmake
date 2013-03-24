
list(APPEND HAL_ACPI_SOURCE
    acpi/halacpi.c
    acpi/halpnpdd.c
    acpi/busemul.c
    legacy/bus/pcibus.c)

add_object_library(lib_hal_acpi ${HAL_ACPI_SOURCE})

#add_pch(lib_hal_acpi include/hal.h)

if(MSVC OR (NOT CMAKE_VERSION VERSION_GREATER 2.8.7))
    target_link_libraries(lib_hal_acpi lib_hal_generic)
endif()
