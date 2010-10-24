
add_definitions(-DNDIS50_MINIPORT)
add_definitions(-DNDIS_MINIPORT_DRIVER)
add_definitions(-DNDIS_LEGACY_MINIPORT)
add_definitions(-DNDIS51_MINIPORT)

list(APPEND SOURCE
    pcnet.c
    requests.c
    pcnet.rc)

add_library(pcnet SHARED ${SOURCE})

set_module_type(pcnet kernelmodedriver)

target_link_libraries(pcnet
    -lndis
    -lntoskrnl
    -lhal)

add_dependencies(pcnet psdk bugcodes buildno_header)

add_livecd_target(pcnet reactos/system32/drivers)