
add_definitions(
    -DNDIS50_MINIPORT
    -DNDIS_MINIPORT_DRIVER
    -DNDIS_LEGACY_MINIPORT)

list(APPEND SOURCE
    ndis.c
    hardware.c
    info.c
    interrupt.c
    nic.h
    e1000hw.h
    debug.c
    debug.h
    send.c)

add_library(e1000 MODULE ${SOURCE} e1000.rc)
add_pch(e1000 nic.h SOURCE)
set_module_type(e1000 kernelmodedriver)
add_importlibs(e1000 ndis ntoskrnl hal)
add_cd_file(TARGET e1000 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(e1000 nete1000.inf)
