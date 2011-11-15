
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

add_library(usbdrv SHARED ${SOURCE})

set_module_type(usbdrv kernelmodedriver)
add_importlibs(usbdrv ntoskrnl hal)

add_pch(usbdrv usbdriver.h)

add_cd_file(TARGET usbdrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
