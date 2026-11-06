
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    bulkio.cpp
    cmdring.cpp
    ctrlxfer.cpp
    dmapool.cpp
    epctx.cpp
    epstate.cpp
    hc.cpp
    intr.cpp
    isoxfer.cpp
    mmio.cpp
    rootports.cpp
    slotdev.cpp
    slots.cpp
    xferring.cpp
    xhcidrv.h
    xhcientry.cpp)

add_library(usbxhci MODULE ${SOURCE} usbxhci.rc)
set_module_type(usbxhci kmdfdriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # WDF_DECLARE_CONTEXT_TYPE passes a string literal as PCHAR
    target_compile_options(usbxhci PRIVATE -Wno-write-strings)
endif()

target_compile_definitions(usbxhci PRIVATE
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_link_libraries(usbxhci ucxstub ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(usbxhci wdfldr ntoskrnl hal)
add_cd_file(TARGET usbxhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(usbxhci usbxhci.inf)
add_registry_inf(usbxhci_reg.inf)
