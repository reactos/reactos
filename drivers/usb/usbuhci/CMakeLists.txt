
list(APPEND SOURCE
    roothub.c
    usbuhci.c
    usbuhci.h)

add_library(usbuhci MODULE
    ${SOURCE}
    guid.c
    usbuhci.rc)

target_compile_definitions(usbuhci
    # HACK: Disabled until USB Stack is reinvestigated.
    PRIVATE NO_KERNEL_LIST_ENTRY_CHECKS)

set_module_type(usbuhci kernelmodedriver)
add_importlibs(usbuhci usbport usbd hal ntoskrnl)
add_pch(usbuhci usbuhci.h SOURCE)
add_cd_file(TARGET usbuhci DESTINATION reactos/system32/drivers NO_CAB FOR all)
