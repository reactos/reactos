
if(NOT ARCH STREQUAL "arm")
add_subdirectory(acpi)
endif()
add_subdirectory(isapnp)
add_subdirectory(pci)
add_subdirectory(pcix)
add_subdirectory(pcmcia)
