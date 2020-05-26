
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/lwip/src/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/lwip/src/include/ipv4)

add_definitions(
    -DNDIS40
    -D_NTDRIVER_)

spec2def(tcpip.sys tcpip.spec)

list(APPEND SOURCE
    datalink/lan.c
    tcpip/ainfo.c
    tcpip/buffer.c
    tcpip/cinfo.c
    tcpip/dispatch.c
    tcpip/fileobjs.c
    tcpip/icmp.c
    tcpip/iinfo.c
    tcpip/info.c
    tcpip/lock.c
    tcpip/main.c
    tcpip/ninfo.c
    tcpip/proto.c
    tcpip/tinfo.c
    tcpip/wait.c
    include/precomp.h)

add_library(tcpip MODULE
    ${SOURCE}
    tcpip.rc
    ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def)

target_link_libraries(tcpip ip lwip ${PSEH_LIB} chew)
set_module_type(tcpip kernelmodedriver)
add_importlibs(tcpip ndis ntoskrnl hal)
add_pch(tcpip include/precomp.h SOURCE)
add_cd_file(TARGET tcpip DESTINATION reactos/system32/drivers FOR all)
