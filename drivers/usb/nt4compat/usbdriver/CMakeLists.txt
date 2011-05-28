
add_definitions(
    -DINCLUDE_EHCI
    -D_MULTI_UHCI
    -D_MULTI_EHCI
    -D_X86)

list(APPEND SOURCE
    ehci.c
    ohci.c
    uhci.c
    roothub.c
    hub.c
    td.c
    usb.c
    umss.c
    bulkonly.c
    cbi.c
    devmgr.c
    dmgrdisp.c
    compdrv.c
    etd.c
    gendrv.c
    mouse.c
    keyboard.c
    usbdriver.rc)

add_library(usbdriver SHARED ${CMAKE_CURRENT_BINARY_DIR}/usbdriver_usbdriver.h.gch ${SOURCE})

set_module_type(usbdriver kernelmodedriver)
add_importlibs(usbdriver ntoskrnl hal)

add_pch(usbdriver ${CMAKE_CURRENT_SOURCE_DIR}/usbdriver.h ${SOURCE})

add_cd_file(TARGET usbdriver DESTINATION reactos/system32/drivers FOR all)
