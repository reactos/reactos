
include_directories(BEFORE include)

list(APPEND SOURCE
    ne2000/8390.c
    ne2000/main.c
    ne2000.rc)

add_library(ne2000 SHARED ${SOURCE})

set_module_type(ne2000 kernelmodedriver)

target_link_libraries(ne2000
    -lndis
    -lntoskrnl
    -lhal)

add_dependencies(ne2000 psdk bugcodes buildno_header)

add_livecd_target(ne2000 reactos/system32/drivers)