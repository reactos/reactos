
list(APPEND SOURCE
    roothub.c
    usbohci.c
    usbohci.h)

add_library(usbohci MODULE
    ${SOURCE}
    guid.c
    usbohci.rc)

set_module_type(usbohci kernelmodedriver)
add_importlibs(usbohci usbport usbd hal ntoskrnl)
add_pch(usbohci usbohci.h SOURCE)
add_cd_file(TARGET usbohci DESTINATION reactos/system32/drivers NO_CAB FOR all)
