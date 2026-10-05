
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    guid.c
    usbdex.c
    usbdex.h)

add_library(usbdex ${SOURCE})
add_dependencies(usbdex xdk)

target_compile_definitions(usbdex PRIVATE
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_include_directories(usbdex PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.17
    ${REACTOS_SOURCE_DIR}/sdk/include/ucx/1.6)
