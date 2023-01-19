
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lwip)
set(LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip_glue/include")

add_library(lwipglue STATIC
	lwip_glue/rosip.c
	lwip_glue/rosmem.c
	lwip_glue/rostcp.c
	lwip_glue/sys_arch.c)
target_include_directories(lwipglue PRIVATE ${LWIP_INCLUDE_DIRS})
target_include_directories(lwipglue BEFORE PRIVATE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include)
add_dependencies(lwipglue xdk)
add_importlibs(lwipglue ntoskrnl)

include(${LWIP_DIR}/src/Filelists.cmake)
target_link_libraries(lwipcore lwipglue)
add_dependencies(lwipcore xdk)


if(ARCH STREQUAL "i386")
    add_asm_files(ip_asm network/i386/checksum.S)
endif()

list(APPEND SOURCE
    network/address.c
    network/arp.c
    network/checksum.c
    network/icmp.c
    network/interface.c
    network/ip.c
    network/loopback.c
    network/neighbor.c
    network/ports.c
    network/receive.c
    network/router.c
    network/routines.c
    network/transmit.c
    transport/datagram/datagram.c
    transport/rawip/rawip.c
    transport/tcp/accept.c
    transport/tcp/event.c
    transport/tcp/if.c
    transport/tcp/tcp.c
    transport/udp/udp.c
    precomp.h)

add_library(ip ${SOURCE} ${ip_asm})
add_pch(ip precomp.h SOURCE)
add_dependencies(ip asm)

target_include_directories(ip PUBLIC ${LWIP_INCLUDE_DIRS})
target_include_directories(ip BEFORE PRIVATE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include)
target_compile_definitions(ip PRIVATE -D__NTDRIVER__)
