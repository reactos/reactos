
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    afd/bind.c
    afd/connect.c
    afd/context.c
    afd/info.c
    afd/listen.c
    afd/lock.c
    afd/main.c
    afd/read.c
    afd/select.c
    afd/tdi.c
    afd/tdiconn.c
    afd/write.c
    afd.rc)

add_library(afd SHARED ${CMAKE_CURRENT_BINARY_DIR}/afd_afd.h.gch ${SOURCE})

set_module_type(afd kernelmodedriver)

target_link_libraries(afd
    pseh
    -lntoskrnl
    -lhal)

add_pch(afd ${CMAKE_CURRENT_SOURCE_DIR}/include/afd.h ${SOURCE})
add_dependencies(afd psdk bugcodes buildno_header)

add_livecd_target(afd reactos/system32/drivers)