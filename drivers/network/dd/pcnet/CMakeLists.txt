
add_definitions(-DNDIS50_MINIPORT)
add_definitions(-DNDIS_MINIPORT_DRIVER)
add_definitions(-DNDIS_LEGACY_MINIPORT)
add_definitions(-DNDIS51_MINIPORT)

list(APPEND SOURCE
    pcnet.c
    requests.c
    pcnet.rc)

add_library(pcnet SHARED ${SOURCE})

set_target_properties(pcnet PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(pcnet
    -lndis
    -lntoskrnl
    -lhal)

add_dependencies(pcnet psdk bugcodes buildno_header)
