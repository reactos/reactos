
add_definitions(-DINCLUDE_EHCI)
add_definitions(-D_MULTI_UHCI)
add_definitions(-D_MULTI_EHCI)
add_definitions(-D_X86)

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

set_target_properties(usbdriver PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(usbdriver ntoskrnl hal)
add_pch(usbdriver ${CMAKE_CURRENT_SOURCE_DIR}/usbdriver.h ${SOURCE})
add_dependencies(usbdriver psdk bugcodes)
add_cab_target(usbdriver 2)