
add_definitions(-DDEBUG_MODE)

list(APPEND SOURCE
    descriptor.c
    disk.c
    fdo.c
    misc.c
    pdo.c
    queue.c
    error.c
    scsi.c
    usbstor.c
    usbstor.h)

add_library(usbstor MODULE
    ${SOURCE}
    guid.c
    usbstor.rc)
add_pch(usbstor usbstor.h SOURCE)

set_module_type(usbstor kernelmodedriver)
add_importlibs(usbstor ntoskrnl hal usbd)
add_cd_file(TARGET usbstor DESTINATION reactos/system32/drivers NO_CAB FOR all)
