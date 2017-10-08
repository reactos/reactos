
list(APPEND SOURCE
    hidusb.c
    hidusb.rc)

add_library(hidusb SHARED ${SOURCE})
set_module_type(hidusb kernelmodedriver)
add_importlibs(hidusb hidclass ntoskrnl usbd hal)
add_cd_file(TARGET hidusb DESTINATION reactos/system32/drivers NO_CAB FOR all)
