
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/lwip/src/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/lwip/src/include/ipv4)

add_definitions(-DNDIS51)

spec2def(tcpip.sys tcpip.spec)

list(APPEND SOURCE
    address.c
    entities.c
    information.c
    interface.c
    main.c
    ndis_lwip.c
    tcp.c
    stubs.c
    precomp.h
)

add_library(tcpip SHARED
    ${SOURCE}
    tcpip.rc
    ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def)

target_link_libraries(tcpip lwip ${PSEH_LIB} libcntpr)
set_module_type(tcpip kernelmodedriver)
add_importlibs(tcpip ndis ntoskrnl hal)
add_pch(tcpip precomp.h SOURCE)
add_cd_file(TARGET tcpip DESTINATION reactos/system32/drivers FOR all)
