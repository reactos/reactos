set(CMAKE_C_STANDARD 11)

include_directories(
    uacpi/include
    uacpi/include/uacpi/
    uacpi/include/uacpi/platform
    uacpi/include/uacpi/internal)
    
list(APPEND UACPI_SOURCE
     uacpi/source/default_handlers.c
     uacpi/source/event.c
     uacpi/source/interpreter.c
     uacpi/source/io.c
     uacpi/source/mutex.c
     uacpi/source/namespace.c
     uacpi/source/notify.c
     uacpi/source/opcodes.c
     uacpi/source/opregion.c
     uacpi/source/osi.c
     uacpi/source/registers.c
     uacpi/source/resources.c
     uacpi/source/shareable.c
     uacpi/source/sleep.c
     uacpi/source/stdlib.c
     uacpi/source/tables.c
     uacpi/source/types.c
     uacpi/source/uacpi.c
     uacpi/source/utilities.c)

add_library(uacpi ${UACPI_SOURCE})
add_pch(uacpi uacpi/include/uacpi/acpi.h "${UACPI_SOURCE}")

list(APPEND ACPI_SOURCE
     uacpiosl.c
     main.c)

add_library(acpi MODULE
            ${ACPI_SOURCE}
            acpi.rc)

target_link_libraries(acpi wdmguid uacpi memcmp strtol ${PSEH_LIB})
set_module_type(acpi kernelmodedriver)
add_importlibs(acpi ntoskrnl hal)
add_pch(acpi precomp.h ACPI_SOURCE)
add_cd_file(TARGET acpi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(acpi acpi.inf)
