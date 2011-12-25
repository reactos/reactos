
list(APPEND SOURCE
    hidusb.c
    usbhub.rc)

add_library(hidusb SHARED ${SOURCE})

set_module_type(hidusb kernelmodedriver)
add_importlibs(hidusb hidclass ntoskrnl usbd)

add_cab_target(usbhub 2)
