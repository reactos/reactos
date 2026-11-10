
add_definitions(
    -DNDIS51_MINIPORT
    -DNDIS_MINIPORT_DRIVER)

list(APPEND SOURCE
    debug.h
    e100.c
    e100.h
    e100hw.h
    eeprom.c
    hardware.c
    init.c
    interrupt.c
    phy.c
    power.c
    requests.c
    send.c)

add_library(e100 MODULE ${SOURCE} e100.rc)
target_link_libraries(e100 memcmp)
add_pch(e100 e100.h SOURCE)
set_module_type(e100 kernelmodedriver)
add_importlibs(e100 ndis ntoskrnl hal)
add_cd_file(TARGET e100 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(e100 nete100.inf)
