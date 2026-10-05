
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    classlib.cpp
    driver.cpp
    exports.cpp
    ucx01000.h)

add_library(ucx01000 MODULE ${SOURCE} ucx01000.rc)
set_module_type(ucx01000 kmdfdriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # WDF_DECLARE_CONTEXT_TYPE passes a string literal as PCHAR
    target_compile_options(ucx01000 PRIVATE -Wno-write-strings)
endif()

target_compile_definitions(ucx01000 PRIVATE
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_include_directories(ucx01000 PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include/ucx/1.6
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/shared/inc/private/common
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/kmdf/inc/private)

target_link_libraries(ucx01000 ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(ucx01000 wdfldr ntoskrnl hal)
add_cd_file(TARGET ucx01000 DESTINATION reactos/system32/drivers NO_CAB FOR all)
