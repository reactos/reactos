
list(APPEND SOURCE
    usbxhci.c
    roothub.c)

add_library(usbxhci MODULE
    ${SOURCE}
    usbxhci.rc)

set_module_type(usbxhci kernelmodedriver)
add_importlibs(usbxhci usbport usbd hal ntoskrnl)
add_pch(usbxhci usbxhci.h SOURCE)
add_cd_file(TARGET usbxhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
