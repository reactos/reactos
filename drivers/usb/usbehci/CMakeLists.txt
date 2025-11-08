
list(APPEND SOURCE
    debug.c
    roothub.c
    usbehci.c
    usbehci.h)

add_library(usbehci MODULE
    ${SOURCE}
    guid.c
    usbehci.rc)

target_compile_definitions(usbehci
    # HACK: Disabled until USB Stack is reinvestigated.
    PRIVATE NO_KERNEL_LIST_ENTRY_CHECKS)

set_module_type(usbehci kernelmodedriver)
add_importlibs(usbehci usbport usbd hal ntoskrnl)
add_pch(usbehci usbehci.h SOURCE)
add_cd_file(TARGET usbehci DESTINATION reactos/system32/drivers NO_CAB FOR all)
