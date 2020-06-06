
include_directories(${REACTOS_SOURCE_DIR}/drivers/bus/acpi/acpica/include)

list(APPEND SOURCE
    Bus_PDO_QueryResourceRequirements.c
    testlist.c)

add_executable(acpi_apitest ${SOURCE})
set_module_type(acpi_apitest win32cui)
add_importlibs(acpi_apitest msvcrt kernel32 ntdll)

add_rostests_file(TARGET acpi_apitest)
