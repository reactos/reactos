
list(APPEND SOURCE
    descriptor.c
    disk.c
    fdo.c
    misc.c
    pdo.c
    queue.c
    error.c
    scsi.c
    usbstor.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(usbstor MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    usbstor.rc)

set_module_type(usbstor kernelmodedriver)
add_importlibs(usbstor ntoskrnl hal usbd)
add_pch(usbstor usbstor.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET usbstor DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(usbstor usbstor.inf)
