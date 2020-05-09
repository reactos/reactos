
spec2def(usbport.sys usbport.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    debug.c
    device.c
    endpoint.c
    iface.c
    ioctl.c
    iso.c
    pnp.c
    power.c
    queue.c
    roothub.c
    trfsplit.c
    urb.c
    usb2.c
    usbport.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(usbport MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    usbport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usbport.def)

add_pch(usbport usbport.h "${PCH_SKIP_SOURCE}")
set_module_type(usbport kernelmodedriver)
add_importlibs(usbport ntoskrnl hal)
add_cd_file(TARGET usbport DESTINATION reactos/system32/drivers NO_CAB FOR all)
