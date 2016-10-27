
list(APPEND SOURCE
    roothub.c
    usbohci.c
    usbohci.h)

add_library(usbohci SHARED
    ${SOURCE}
    guid.c
    usbohci.rc)

set_module_type(usbohci kernelmodedriver)
add_importlibs(usbohci usbport ntoskrnl hal usbd)
add_pch(usbohci usbohci.h SOURCE)
add_cd_file(TARGET usbohci DESTINATION reactos/system32/drivers NO_CAB FOR all)
