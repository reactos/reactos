
if(ARCH STREQUAL "i386")
    add_asm_files(ip_asm network/i386/checksum.S)
endif()

list(APPEND SOURCE
    lwip_glue/ip.c
    lwip_glue/memory.c
    lwip_glue/sys_arch.c
    lwip_glue/tcp.c
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
    transport/udp/udp.c)

add_library(ip OBJECT ${SOURCE} ${ip_asm})

target_link_libraries(ip lwipcore)
add_importlibs(lwipcore ntoskrnl)

target_include_directories(ip BEFORE
    PRIVATE ${LWIP_INCLUDE_DIRS}
    PRIVATE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include)

add_pch(ip precomp.h SOURCE)
