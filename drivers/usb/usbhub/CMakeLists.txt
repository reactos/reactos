
list(APPEND SOURCE
    createclose.c
    fdo.c
    misc.c
    pdo.c
    usbhub.c
    usbhub.rc)

add_library(usbhub SHARED ${SOURCE})

set_module_type(usbhub kernelmodedriver)
add_importlibs(usbhub ntoskrnl hal usbd)

add_cd_file(TARGET usbhub DESTINATION reactos/system32/drivers NO_CAB FOR all)
