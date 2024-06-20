
include_directories(${REACTOS_SOURCE_DIR}/drivers/bus/acpi/acpica/include)

generate_whitelist_code(acpi_apitest_whitelist.txt)
add_definitions(-DUSE_WHITELIST)

list(APPEND SOURCE
    Bus_PDO_EvalMethod.c
    Bus_PDO_QueryResourceRequirements.c
    testlist.c
    ${CMAKE_CURRENT_BINARY_DIR}/whitelist.c)

add_executable(acpi_apitest ${SOURCE})
set_module_type(acpi_apitest win32cui)
add_importlibs(acpi_apitest msvcrt kernel32 ntdll)

add_rostests_file(TARGET acpi_apitest)
