
if(NOT ARCH STREQUAL "arm")
add_subdirectory(acpi)
add_subdirectory(acpi_new)
endif()
add_subdirectory(isapnp)
add_subdirectory(pci)
add_subdirectory(pcix)
add_subdirectory(pcmcia)
