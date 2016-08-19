
add_definitions(-DDEBUG_MODE)
add_definitions(-DNTDDI_VERSION=0x05020400)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND SOURCE
    fdo.c
    hub_fdo.c
    misc.c
    pdo.c
    usbhub.c
    usbhub.h)

add_library(usbhub SHARED
    ${SOURCE}
    guid.c
    usbhub.rc)

target_link_libraries(usbhub ${PSEH_LIB})
set_module_type(usbhub kernelmodedriver)
add_importlibs(usbhub ntoskrnl hal usbd)
add_pch(usbhub usbhub.h SOURCE)
add_cd_file(TARGET usbhub DESTINATION reactos/system32/drivers NO_CAB FOR all)
