
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
