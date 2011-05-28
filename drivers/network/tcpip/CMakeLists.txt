
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

add_definitions(
    -DNDIS40
    -D_NTDRIVER_)

spec2def(tcpip.sys tcpip.spec)

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
    tcpip.rc
    ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def)

add_library(tcpip SHARED ${CMAKE_CURRENT_BINARY_DIR}/tcpip_precomp.h.gch ${SOURCE})

target_link_libraries(tcpip
    ip
    oskittcp
    ${PSEH_LIB}
    chew)

set_module_type(tcpip kernelmodedriver)
add_importlibs(tcpip ndis ntoskrnl hal)

add_pch(tcpip ${CMAKE_CURRENT_SOURCE_DIR}/include/precomp.h ${SOURCE})

add_cd_file(TARGET tcpip DESTINATION reactos/system32/drivers FOR all)
