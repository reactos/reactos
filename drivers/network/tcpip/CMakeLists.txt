
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

add_definitions(-DNDIS40)
add_definitions(-D_NTDRIVER_)

spec2def(tcpip ${CMAKE_CURRENT_SOURCE_DIR}/tcpip.spec ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def)

list(APPEND SOURCE
    datalink/lan.c
    tcpip/ainfo.c
    tcpip/buffer.c
    tcpip/dispatch.c
    tcpip/fileobjs.c
    tcpip/iinfo.c
    tcpip/info.c
    tcpip/lock.c
    tcpip/main.c
    tcpip/ninfo.c
    tcpip/proto.c
    tcpip/tinfo.c
    tcpip/wait.c
    tcpip.rc)

add_library(tcpip SHARED ${CMAKE_CURRENT_BINARY_DIR}/tcpip_precomp.h.gch ${SOURCE})

set_module_type(tcpip kernelmodedriver)

target_link_libraries(tcpip
    ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def
    ip
    oskittcp
    pseh
    chew
    -lndis
    -lntoskrnl
    -lhal)

add_pch(tcpip ${CMAKE_CURRENT_SOURCE_DIR}/include/precomp.h ${SOURCE})
add_dependencies(tcpip tcpip_def psdk bugcodes buildno_header)

add_cab_target(tcpip 2)