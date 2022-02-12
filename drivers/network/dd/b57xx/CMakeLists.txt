
add_definitions(
    -DNDIS_MINIPORT_DRIVER
    -DNDIS51_MINIPORT)

list(APPEND SOURCE
    ndis.c
    hardware.c
    info.c
    interrupt.c
    nic.h)

add_library(b57xx MODULE ${SOURCE} b57xx.rc)
add_pch(b57xx nic.h SOURCE)
set_module_type(b57xx kernelmodedriver)
add_importlibs(b57xx ndis ntoskrnl hal)
add_cd_file(TARGET b57xx DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(b57xx netb57xx.inf)
