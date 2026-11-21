
add_definitions(
    -DNDIS51_MINIPORT
    -DNDIS_MINIPORT_DRIVER
    -DNDIS_LEGACY_MINIPORT=1)

list(APPEND SOURCE
    lance.c
    send.c
    transfer.c
    keywords.h
    lancehrd.h
    lancesft.h)

add_library(pcntn5m MODULE ${SOURCE} pcntn5m.rc)
set_module_type(pcntn5m kernelmodedriver)
add_importlibs(pcntn5m ndis ntoskrnl hal)
add_cd_file(TARGET pcntn5m DESTINATION reactos/system32/drivers FOR all)
