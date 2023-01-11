
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    adsp.cpp
    buspdo.cpp
    fdo.cpp
    hdac_controller.cpp
    hdac_stream.cpp
    hdaudbus.cpp
    hdaudio.cpp
    nhlt.cpp
    driver.h)

add_library(hdaudbus MODULE ${SOURCE})
set_module_type(hdaudbus kernelmodedriver)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hdaudbus PRIVATE -Wno-unused-variable -Wno-sign-compare -Wno-unknown-pragmas -Wno-switch)
endif()
target_link_libraries(hdaudbus wdf01000 libcntpr ${PSEH_LIB})
add_importlibs(hdaudbus ntoskrnl hal)
add_cd_file(TARGET hdaudbus DESTINATION reactos/system32/drivers FOR all)
add_pch(hdaudbus driver.h SOURCE)
add_driver_inf(hdaudbus hdaudbus.inf)
