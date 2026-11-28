
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lwip)
set(LWIP_INCLUDE_DIRS
    ${LWIP_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lwip
)
include(${LWIP_DIR}/src/Filelists.cmake)
add_dependencies(lwipcore xdk)
if(TCPIP_USE_FNET)
    add_subdirectory(fnet_port)
endif()
add_subdirectory(ip)

add_definitions(
    -DNDIS40
    -D_NTDRIVER_)

spec2def(tcpip.sys tcpip.spec)

list(APPEND SOURCE
    chew/workqueue.c
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
    include/lwip/precomp.h)

add_library(tcpip MODULE
    ${SOURCE}
    tcpip.rc
    ${CMAKE_CURRENT_BINARY_DIR}/tcpip.def)

target_include_directories(tcpip BEFORE
    PRIVATE ${LWIP_INCLUDE_DIRS}
    PRIVATE include)

target_link_libraries(tcpip ip ${PSEH_LIB})
if(TCPIP_USE_FNET)
    target_link_libraries(tcpip fnetport)
    # Shares CONNECTION_ENDPOINT with the ip library, which has FNET only fields
    target_compile_definitions(tcpip PRIVATE TCPIP_USE_FNET)
endif()
set_module_type(tcpip kernelmodedriver)
add_importlibs(tcpip ndis ntoskrnl hal)
add_pch(tcpip include/lwip/precomp.h SOURCE)
add_cd_file(TARGET tcpip DESTINATION reactos/system32/drivers FOR all)
