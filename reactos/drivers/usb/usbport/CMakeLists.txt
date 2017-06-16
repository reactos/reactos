
spec2def(usbport.sys usbport.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    debug.c
    device.c
    endpoint.c
    iface.c
    ioctl.c
    pnp.c
    power.c
    queue.c
    roothub.c
    urb.c
    usb2.c
    usbport.c
    usbport.h)

add_library(usbport SHARED
    ${SOURCE}
    guid.c
    usbport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usbport.def)

add_pch(usbport usbport.h SOURCE)
set_module_type(usbport kernelmodedriver)
add_importlibs(usbport ntoskrnl hal)
add_cd_file(TARGET usbport DESTINATION reactos/system32/drivers NO_CAB FOR all)
