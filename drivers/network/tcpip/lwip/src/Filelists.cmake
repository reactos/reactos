# This file is indended to be included in end-user CMakeLists.txt
# include(/path/to/Filelists.cmake)
# It assumes the variable LWIP_DIR is defined pointing to the
# root path of lwIP sources.
#
# This file is NOT designed (on purpose) to be used as cmake
# subdir via add_subdirectory()
# The intention is to provide greater flexibility to users to
# create their own targets using the *_SRCS variables.

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.10.0")
    include_guard(GLOBAL)
endif()

set(LWIP_VERSION_MAJOR    "2")
set(LWIP_VERSION_MINOR    "2")
set(LWIP_VERSION_REVISION "0")
# LWIP_VERSION_RC is set to LWIP_RC_RELEASE for official releases
# LWIP_VERSION_RC is set to LWIP_RC_DEVELOPMENT for Git versions
# Numbers 1..31 are reserved for release candidates
set(LWIP_VERSION_RC       "LWIP_RC_RELEASE")

if ("${LWIP_VERSION_RC}" STREQUAL "LWIP_RC_RELEASE")
    set(LWIP_VERSION_STRING
        "${LWIP_VERSION_MAJOR}.${LWIP_VERSION_MINOR}.${LWIP_VERSION_REVISION}"
    )
elseif ("${LWIP_VERSION_RC}" STREQUAL "LWIP_RC_DEVELOPMENT")
    set(LWIP_VERSION_STRING
        "${LWIP_VERSION_MAJOR}.${LWIP_VERSION_MINOR}.${LWIP_VERSION_REVISION}.dev"
    )
else()
    set(LWIP_VERSION_STRING
        "${LWIP_VERSION_MAJOR}.${LWIP_VERSION_MINOR}.${LWIP_VERSION_REVISION}.rc${LWIP_VERSION_RC}"
    )
endif()

# The minimum set of files needed for lwIP.
set(lwipcore_SRCS
    ${LWIP_DIR}/src/core/init.c
    ${LWIP_DIR}/src/core/def.c
    ${LWIP_DIR}/src/core/dns.c
    ${LWIP_DIR}/src/core/inet_chksum.c
    ${LWIP_DIR}/src/core/ip.c
    ${LWIP_DIR}/src/core/mem.c
    ${LWIP_DIR}/src/core/memp.c
    ${LWIP_DIR}/src/core/netif.c
    ${LWIP_DIR}/src/core/pbuf.c
    ${LWIP_DIR}/src/core/raw.c
    ${LWIP_DIR}/src/core/stats.c
    ${LWIP_DIR}/src/core/sys.c
    ${LWIP_DIR}/src/core/altcp.c
    ${LWIP_DIR}/src/core/altcp_alloc.c
    ${LWIP_DIR}/src/core/altcp_tcp.c
    ${LWIP_DIR}/src/core/tcp.c
    ${LWIP_DIR}/src/core/tcp_in.c
    ${LWIP_DIR}/src/core/tcp_out.c
    ${LWIP_DIR}/src/core/timeouts.c
    ${LWIP_DIR}/src/core/udp.c
)
set(lwipcore4_SRCS
    ${LWIP_DIR}/src/core/ipv4/acd.c
    ${LWIP_DIR}/src/core/ipv4/autoip.c
    ${LWIP_DIR}/src/core/ipv4/dhcp.c
    ${LWIP_DIR}/src/core/ipv4/etharp.c
    ${LWIP_DIR}/src/core/ipv4/icmp.c
    ${LWIP_DIR}/src/core/ipv4/igmp.c
    ${LWIP_DIR}/src/core/ipv4/ip4_frag.c
    ${LWIP_DIR}/src/core/ipv4/ip4.c
    ${LWIP_DIR}/src/core/ipv4/ip4_addr.c
)
set(lwipcore6_SRCS
    ${LWIP_DIR}/src/core/ipv6/dhcp6.c
    ${LWIP_DIR}/src/core/ipv6/ethip6.c
    ${LWIP_DIR}/src/core/ipv6/icmp6.c
    ${LWIP_DIR}/src/core/ipv6/inet6.c
    ${LWIP_DIR}/src/core/ipv6/ip6.c
    ${LWIP_DIR}/src/core/ipv6/ip6_addr.c
    ${LWIP_DIR}/src/core/ipv6/ip6_frag.c
    ${LWIP_DIR}/src/core/ipv6/mld6.c
    ${LWIP_DIR}/src/core/ipv6/nd6.c
)

# APIFILES: The files which implement the sequential and socket APIs.
set(lwipapi_SRCS
    ${LWIP_DIR}/src/api/api_lib.c
    ${LWIP_DIR}/src/api/api_msg.c
    ${LWIP_DIR}/src/api/err.c
    ${LWIP_DIR}/src/api/if_api.c
    ${LWIP_DIR}/src/api/netbuf.c
    ${LWIP_DIR}/src/api/netdb.c
    ${LWIP_DIR}/src/api/netifapi.c
    ${LWIP_DIR}/src/api/sockets.c
    ${LWIP_DIR}/src/api/tcpip.c
)

# Files implementing various generic network interface functions
set(lwipnetif_SRCS
    ${LWIP_DIR}/src/netif/ethernet.c
    ${LWIP_DIR}/src/netif/bridgeif.c
    ${LWIP_DIR}/src/netif/bridgeif_fdb.c
)

if (NOT ${LWIP_EXCLUDE_SLIPIF})
	list(APPEND lwipnetif_SRCS ${LWIP_DIR}/src/netif/slipif.c)
endif()

# 6LoWPAN
set(lwipsixlowpan_SRCS
    ${LWIP_DIR}/src/netif/lowpan6_common.c
    ${LWIP_DIR}/src/netif/lowpan6.c
    ${LWIP_DIR}/src/netif/lowpan6_ble.c
    ${LWIP_DIR}/src/netif/zepif.c
)

# PPP
set(lwipppp_SRCS
    ${LWIP_DIR}/src/netif/ppp/auth.c
    ${LWIP_DIR}/src/netif/ppp/ccp.c
    ${LWIP_DIR}/src/netif/ppp/chap-md5.c
    ${LWIP_DIR}/src/netif/ppp/chap_ms.c
    ${LWIP_DIR}/src/netif/ppp/chap-new.c
    ${LWIP_DIR}/src/netif/ppp/demand.c
    ${LWIP_DIR}/src/netif/ppp/eap.c
    ${LWIP_DIR}/src/netif/ppp/ecp.c
    ${LWIP_DIR}/src/netif/ppp/eui64.c
    ${LWIP_DIR}/src/netif/ppp/fsm.c
    ${LWIP_DIR}/src/netif/ppp/ipcp.c
    ${LWIP_DIR}/src/netif/ppp/ipv6cp.c
    ${LWIP_DIR}/src/netif/ppp/lcp.c
    ${LWIP_DIR}/src/netif/ppp/magic.c
    ${LWIP_DIR}/src/netif/ppp/mppe.c
    ${LWIP_DIR}/src/netif/ppp/multilink.c
    ${LWIP_DIR}/src/netif/ppp/ppp.c
    ${LWIP_DIR}/src/netif/ppp/pppapi.c
    ${LWIP_DIR}/src/netif/ppp/pppcrypt.c
    ${LWIP_DIR}/src/netif/ppp/pppoe.c
    ${LWIP_DIR}/src/netif/ppp/pppol2tp.c
    ${LWIP_DIR}/src/netif/ppp/pppos.c
    ${LWIP_DIR}/src/netif/ppp/upap.c
    ${LWIP_DIR}/src/netif/ppp/utils.c
    ${LWIP_DIR}/src/netif/ppp/vj.c
    ${LWIP_DIR}/src/netif/ppp/polarssl/arc4.c
    ${LWIP_DIR}/src/netif/ppp/polarssl/des.c
    ${LWIP_DIR}/src/netif/ppp/polarssl/md4.c
    ${LWIP_DIR}/src/netif/ppp/polarssl/md5.c
    ${LWIP_DIR}/src/netif/ppp/polarssl/sha1.c
)

# SNMPv3 agent
set(lwipsnmp_SRCS
    ${LWIP_DIR}/src/apps/snmp/snmp_asn1.c
    ${LWIP_DIR}/src/apps/snmp/snmp_core.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_icmp.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_interfaces.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_ip.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_snmp.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_system.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_tcp.c
    ${LWIP_DIR}/src/apps/snmp/snmp_mib2_udp.c
    ${LWIP_DIR}/src/apps/snmp/snmp_snmpv2_framework.c
    ${LWIP_DIR}/src/apps/snmp/snmp_snmpv2_usm.c
    ${LWIP_DIR}/src/apps/snmp/snmp_msg.c
    ${LWIP_DIR}/src/apps/snmp/snmpv3.c
    ${LWIP_DIR}/src/apps/snmp/snmp_netconn.c
    ${LWIP_DIR}/src/apps/snmp/snmp_pbuf_stream.c
    ${LWIP_DIR}/src/apps/snmp/snmp_raw.c
    ${LWIP_DIR}/src/apps/snmp/snmp_scalar.c
    ${LWIP_DIR}/src/apps/snmp/snmp_table.c
    ${LWIP_DIR}/src/apps/snmp/snmp_threadsync.c
    ${LWIP_DIR}/src/apps/snmp/snmp_traps.c
)

# HTTP server + client
set(lwiphttp_SRCS
    ${LWIP_DIR}/src/apps/http/altcp_proxyconnect.c
    ${LWIP_DIR}/src/apps/http/fs.c
    ${LWIP_DIR}/src/apps/http/http_client.c
    ${LWIP_DIR}/src/apps/http/httpd.c
)

# MAKEFSDATA HTTP server host utility
set(lwipmakefsdata_SRCS
    ${LWIP_DIR}/src/apps/http/makefsdata/makefsdata.c
)

# IPERF server
set(lwipiperf_SRCS
    ${LWIP_DIR}/src/apps/lwiperf/lwiperf.c
)

# SMTP client
set(lwipsmtp_SRCS
    ${LWIP_DIR}/src/apps/smtp/smtp.c
)

# SNTP client
set(lwipsntp_SRCS
    ${LWIP_DIR}/src/apps/sntp/sntp.c
)

# MDNS responder
set(lwipmdns_SRCS
    ${LWIP_DIR}/src/apps/mdns/mdns.c
    ${LWIP_DIR}/src/apps/mdns/mdns_out.c
    ${LWIP_DIR}/src/apps/mdns/mdns_domain.c
)

# NetBIOS name server
set(lwipnetbios_SRCS
    ${LWIP_DIR}/src/apps/netbiosns/netbiosns.c
)

# TFTP server files
set(lwiptftp_SRCS
    ${LWIP_DIR}/src/apps/tftp/tftp.c
)

# MQTT client files
set(lwipmqtt_SRCS
    ${LWIP_DIR}/src/apps/mqtt/mqtt.c
)

# ARM MBEDTLS related files of lwIP rep
set(lwipmbedtls_SRCS
    ${LWIP_DIR}/src/apps/altcp_tls/altcp_tls_mbedtls.c
    ${LWIP_DIR}/src/apps/altcp_tls/altcp_tls_mbedtls_mem.c
    ${LWIP_DIR}/src/apps/snmp/snmpv3_mbedtls.c
)

# All LWIP files without apps
set(lwipnoapps_SRCS
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipcore6_SRCS}
    ${lwipapi_SRCS}
    ${lwipnetif_SRCS}
    ${lwipsixlowpan_SRCS}
    ${lwipppp_SRCS}
)

# LWIPAPPFILES: All LWIP APPs
set(lwipallapps_SRCS
    ${lwipsnmp_SRCS}
    ${lwiphttp_SRCS}
    ${lwipiperf_SRCS}
    ${lwipsmtp_SRCS}
    ${lwipsntp_SRCS}
    ${lwipmdns_SRCS}
    ${lwipnetbios_SRCS}
    ${lwiptftp_SRCS}
    ${lwipmqtt_SRCS}
)

# Generate lwip/init.h (version info)
configure_file(${LWIP_DIR}/src/include/lwip/init.h.cmake.in ${LWIP_DIR}/src/include/lwip/init.h)

# Documentation
set(DOXYGEN_DIR ${LWIP_DIR}/doc/doxygen)
set(DOXYGEN_OUTPUT_DIR output)
set(DOXYGEN_IN  ${LWIP_DIR}/doc/doxygen/lwip.Doxyfile.cmake.in)
set(DOXYGEN_OUT ${LWIP_DIR}/doc/doxygen/lwip.Doxyfile)
configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})

find_package(Doxygen)
if (DOXYGEN_FOUND)
    message(STATUS "Doxygen build started")

    add_custom_target(lwipdocs
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_DIR}/${DOXYGEN_OUTPUT_DIR}/html
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${DOXYGEN_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
else (DOXYGEN_FOUND)
    message(STATUS "Doxygen needs to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

# lwIP libraries
add_library(lwipcore EXCLUDE_FROM_ALL ${lwipnoapps_SRCS})
target_compile_options(lwipcore PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipcore PRIVATE ${LWIP_DEFINITIONS}  ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipcore PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})

add_library(lwipallapps EXCLUDE_FROM_ALL ${lwipallapps_SRCS})
target_compile_options(lwipallapps PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipallapps PRIVATE ${LWIP_DEFINITIONS}  ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipallapps PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})

add_library(lwipmbedtls EXCLUDE_FROM_ALL ${lwipmbedtls_SRCS})
target_compile_options(lwipmbedtls PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipmbedtls PRIVATE ${LWIP_DEFINITIONS}  ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipmbedtls PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})
