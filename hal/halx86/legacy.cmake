
list(APPEND HAL_LEGACY_SOURCE
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_classes.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_vendors.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    legacy/halpnpdd.c
    legacy/halpcat.c
    smp/mps/mps.c)

add_library(lib_hal_legacy OBJECT ${HAL_LEGACY_SOURCE})
add_dependencies(lib_hal_legacy bugcodes xdk)
#add_pch(lib_hal_legacy include/hal.h)
