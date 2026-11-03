
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    bootdev.cpp
    childpdo.cpp
    desccheck.cpp
    devconfig.cpp
    devdesc.cpp
    devhcd.cpp
    devlpm.cpp
    devmisc.cpp
    devreq.cpp
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
    entry.cpp
    errreport.cpp
    fanout.cpp
    fdo.cpp
    guid.cpp
    hubchild.cpp
    hubdescval.cpp
    hubdrom.cpp
    hubid.cpp
    hubioctl.cpp
    hubmsos.cpp
    hubport.cpp
    hubregistry.cpp
    hubreq.cpp
    hubsha1.cpp
    hubsm.cpp
    hubtunnel.cpp
    hubutil.cpp
    hubwmi.cpp
    idlesm.cpp
    portacpi.cpp
    portconn.cpp
    portsm.cpp
    portsm_frames.cpp
    portsm_port.cpp
    portxfer.cpp
    suspend.cpp
    ucxlink.cpp
    upstream.cpp
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
add_driver_inf(usbhub3 usbhub3.inf)
add_registry_inf(usbhub3_reg.inf)
