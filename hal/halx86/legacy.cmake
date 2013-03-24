
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
    legacy/halpcat.c)

add_object_library(lib_hal_legacy ${HAL_LEGACY_SOURCE})

#add_pch(lib_hal_legacy include/hal.h)

if(MSVC OR (NOT CMAKE_VERSION VERSION_GREATER 2.8.7))
    target_link_libraries(lib_hal_legacy lib_hal_generic)
endif()
