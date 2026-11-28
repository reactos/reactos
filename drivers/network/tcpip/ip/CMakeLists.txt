
if(ARCH STREQUAL "i386")
    add_asm_files(ip_asm network/i386/checksum.S)
endif()

if(TCPIP_USE_FNET)
    list(APPEND SOURCE
        fnet_glue/if.c
        fnet_glue/tcp.c)
    set(FNET_SHIM_SOURCE fnet_glue/fnet_shim.c)
else()
    list(APPEND SOURCE
        lwip_glue/ip.c
        lwip_glue/memory.c
        lwip_glue/sys_arch.c
        lwip_glue/tcp.c
        transport/tcp/if.c)
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
    transport/tcp/tcp.c
    transport/udp/udp.c)

add_library(ip OBJECT ${SOURCE} ${FNET_SHIM_SOURCE} ${ip_asm})

if(TCPIP_USE_FNET)
    target_link_libraries(ip fnetport)
else()
    target_link_libraries(ip lwipcore)
    add_importlibs(lwipcore ntoskrnl)
endif()

target_include_directories(ip BEFORE
    PRIVATE ${LWIP_INCLUDE_DIRS}
    PRIVATE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include)

add_pch(ip precomp.h SOURCE)

if(TCPIP_USE_FNET)
    # FNET and the ReactOS network headers define the same names, so the shim must not get the precompiled header
    set_source_files_properties(${FNET_SHIM_SOURCE} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

if(TCPIP_USE_FNET)
    target_compile_definitions(ip PRIVATE TCPIP_USE_FNET)
endif()
