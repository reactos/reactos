
add_definitions(-DDEBUG_MODE)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND SOURCE
    debug.c
    ioctl.c
    pnp.c
    power.c
    usbhub.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(usbhub MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    usbhub.rc)

target_link_libraries(usbhub ${PSEH_LIB})
set_module_type(usbhub kernelmodedriver)
add_importlibs(usbhub ntoskrnl hal wmilib usbd)
add_pch(usbhub usbhub.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET usbhub DESTINATION reactos/system32/drivers NO_CAB FOR all)
