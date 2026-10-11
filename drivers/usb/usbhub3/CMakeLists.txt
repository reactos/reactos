
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    desccheck.cpp
    devconfig.cpp
    devdesc.cpp
    devlpm.cpp
    devmisc.cpp
    devhcd.cpp
    devreq.cpp
    entry.cpp
    hubutil.cpp
    errreport.cpp
    devsm.cpp
    devsm_boot.cpp
    devsm_config.cpp
    devsm_desc.cpp
    devsm_enum.cpp
    devsm_hub.cpp
    devsm_lpm.cpp
    devsm_power.cpp
    devsm_reenum.cpp
    devsm_remove.cpp
    devsm_report.cpp
    devsm_stopped.cpp
    devsm_zero.cpp
    hubsm.cpp
    guid.cpp
    hubdrom.cpp
    portacpi.cpp
    bootdev.cpp
    hubchild.cpp
    portconn.cpp
    hubdescval.cpp
    fdo.cpp
    hubid.cpp
    suspend.cpp
    hubioctl.cpp
    fanout.cpp
    upstream.cpp
    childpdo.cpp
    hubmsos.cpp
    hubport.cpp
    hubregistry.cpp
    hubsha1.cpp
    hubtunnel.cpp
    ucxlink.cpp
    hubwmi.cpp
    hubreq.cpp
    portxfer.cpp
    idlesm.cpp
    portsm.cpp
    portsm_frames.cpp
    portsm_port.cpp
    usbhub3.h)

spec2def(usbhub3.sys usbhub3.spec)

add_library(usbhub3 MODULE ${SOURCE} usbhub3.rc ${CMAKE_CURRENT_BINARY_DIR}/usbhub3.def)
set_module_type(usbhub3 kmdfdriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # WDF_DECLARE_CONTEXT_TYPE passes a string literal as PCHAR
    target_compile_options(usbhub3 PRIVATE -Wno-write-strings)
endif()

target_compile_definitions(usbhub3 PRIVATE
    WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000)

target_include_directories(usbhub3 PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/ucx/1.6)

target_link_libraries(usbhub3 usbdex ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(usbhub3 wdfldr usbd ntoskrnl hal)
add_cd_file(TARGET usbhub3 DESTINATION reactos/system32/drivers NO_CAB FOR all)
