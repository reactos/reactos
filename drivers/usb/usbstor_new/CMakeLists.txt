
add_definitions(-DDEBUG_MODE)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

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

add_library(usbstor SHARED
    ${SOURCE}
    guid.c
    usbstor.rc)

set_module_type(usbstor kernelmodedriver)
add_importlibs(usbstor usbd hal ntoskrnl)
add_pch(usbstor usbstor.h SOURCE)
add_cd_file(TARGET usbstor DESTINATION reactos/system32/drivers NO_CAB FOR all)
