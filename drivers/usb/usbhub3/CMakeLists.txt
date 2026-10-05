
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    driver.cpp
    hub.cpp
    usbhub3.h)

add_library(usbhub3 MODULE ${SOURCE} usbhub3.rc)
set_module_type(usbhub3 kmdfdriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # WDF_DECLARE_CONTEXT_TYPE passes a string literal as PCHAR
    target_compile_options(usbhub3 PRIVATE -Wno-write-strings)
endif()

target_compile_definitions(usbhub3 PRIVATE
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_link_libraries(usbhub3 ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(usbhub3 wdfldr ntoskrnl hal)
add_cd_file(TARGET usbhub3 DESTINATION reactos/system32/drivers NO_CAB FOR all)
