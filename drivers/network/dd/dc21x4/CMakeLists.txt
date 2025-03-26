
add_definitions(
    -DNDIS_MINIPORT_DRIVER
    -DNDIS51_MINIPORT=1)

list(APPEND SOURCE
    dc21x4.c
    dc21x4.h
    dc21x4hw.h
    debug.h
    eeprom.c
    eeprom.h
    eeprom_data.c
    hardware.c
    init.c
    interrupt.c
    media.c
    media040.c
    media041.c
    media140.c
    media143.c
    phy.c
    power.c
    requests.c
    send.c
    util.h)

if(DBG)
    list(APPEND SOURCE debug.c)
endif()

add_library(dc21x4 MODULE ${SOURCE} dc21x4.rc)
if(DBG)
    target_link_libraries(dc21x4 memcmp)
endif()
add_pch(dc21x4 dc21x4.h SOURCE)
set_module_type(dc21x4 kernelmodedriver)
add_importlibs(dc21x4 ndis ntoskrnl hal)
add_cd_file(TARGET dc21x4 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(dc21x4 net21x4.inf)
