
add_definitions(-D__NTDRIVER__)

include_directories(
    BEFORE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include
    ${REACTOS_SOURCE_DIR}/lib/drivers/lwip/src/include
    ${REACTOS_SOURCE_DIR}/lib/drivers/lwip/src/include/ipv4)

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
