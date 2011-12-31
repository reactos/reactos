
list(APPEND SOURCE
    hidusb.c
    hidusb.rc)

add_library(hidusb SHARED ${SOURCE})

set_module_type(hidusb kernelmodedriver)
add_importlibs(hidusb hidclass ntoskrnl usbd hal)

add_cab_target(hidusb 2)
