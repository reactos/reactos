
add_definitions(
    -DNDIS50_MINIPORT
    -DNDIS_MINIPORT_DRIVER
    -DNDIS_LEGACY_MINIPORT
    -DNDIS51_MINIPORT)

list(APPEND SOURCE
    pcnet.c
    requests.c
    pcnet.rc)

add_library(pcnet SHARED ${SOURCE})
add_pch(pcnet pcnet.h)
set_module_type(pcnet kernelmodedriver)
add_importlibs(pcnet ndis ntoskrnl hal)
add_cd_file(TARGET pcnet DESTINATION reactos/system32/drivers FOR all)
