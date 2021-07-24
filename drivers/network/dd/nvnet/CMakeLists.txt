
add_definitions(
    -DNDIS_MINIPORT_DRIVER
    -DNDIS51_MINIPORT=1)

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
add_pch(nvnet nvnet.h SOURCE)
set_module_type(nvnet kernelmodedriver)
add_importlibs(nvnet ndis ntoskrnl hal)
add_cd_file(TARGET nvnet DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(nvnet netnv.inf)
