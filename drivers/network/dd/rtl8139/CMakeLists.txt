
add_definitions(
    -DNDIS50_MINIPORT
    -DNDIS_MINIPORT_DRIVER
    -DNDIS_LEGACY_MINIPORT)

list(APPEND SOURCE
    ndis.c
    hardware.c
    info.c
    interrupt.c
    nic.h)

add_library(rtl8139 MODULE ${SOURCE} rtl8139.rc)
add_pch(rtl8139 nic.h SOURCE)
set_module_type(rtl8139 kernelmodedriver)
add_importlibs(rtl8139 ndis ntoskrnl hal)
add_cd_file(TARGET rtl8139 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(rtl8139 netrtl8139.inf)
