
add_definitions(
    -DNDIS51_MINIPORT
    -DNDIS_MINIPORT_DRIVER)

list(APPEND SOURCE
    backoff.c
    debug.h
    init.c
    interrupt.c
    nic.c
    nic.h
    nvnet.c
    nvnet.h
    phy.c
    phyreg.h
    requests.c
    send.c)

add_library(nvnet MODULE ${SOURCE} nvnet.rc)
target_link_libraries(nvnet memcmp)
add_pch(nvnet nvnet.h SOURCE)
set_module_type(nvnet kernelmodedriver)
add_importlibs(nvnet ndis ntoskrnl hal)
add_cd_file(TARGET nvnet DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(nvnet netnv.inf)
