
remove_definitions(-D_WIN32_WINNT=0x502)

add_library(ucxstub STATIC ucxstub.c ucxstub.h)
add_dependencies(ucxstub xdk)

target_compile_definitions(ucxstub
    PUBLIC WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_include_directories(ucxstub
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${REACTOS_SOURCE_DIR}/sdk/include/ucx/1.6
    ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.17
    PRIVATE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/shared/inc/private/common
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/kmdf/inc/private)
