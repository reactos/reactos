# This file is indended to be included in end-user CMakeLists.txt
# include(/path/to/Filelists.cmake)
# It assumes the variable LWIP_CONTRIB_DIR is defined pointing to the
# root path of lwIP/contrib sources.
#
# This file is NOT designed (on purpose) to be used as cmake
# subdir via add_subdirectory()
# The intention is to provide greater flexibility to users to
# create their own targets using the *_SRCS variables.

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.10.0")
    include_guard(GLOBAL)
endif()

set(lwipcontribexamples_SRCS
    ${LWIP_CONTRIB_DIR}/examples/httpd/fs_example/fs_example.c
    ${LWIP_CONTRIB_DIR}/examples/httpd/https_example/https_example.c
    ${LWIP_CONTRIB_DIR}/examples/httpd/ssi_example/ssi_example.c
    ${LWIP_CONTRIB_DIR}/examples/lwiperf/lwiperf_example.c
    ${LWIP_CONTRIB_DIR}/examples/mdns/mdns_example.c
    ${LWIP_CONTRIB_DIR}/examples/mqtt/mqtt_example.c
    ${LWIP_CONTRIB_DIR}/examples/ppp/pppos_example.c
    ${LWIP_CONTRIB_DIR}/examples/snmp/snmp_private_mib/lwip_prvmib.c
    ${LWIP_CONTRIB_DIR}/examples/snmp/snmp_v3/snmpv3_dummy.c
    ${LWIP_CONTRIB_DIR}/examples/snmp/snmp_example.c
    ${LWIP_CONTRIB_DIR}/examples/sntp/sntp_example.c
    ${LWIP_CONTRIB_DIR}/examples/tftp/tftp_example.c
)
add_library(lwipcontribexamples EXCLUDE_FROM_ALL ${lwipcontribexamples_SRCS})
target_compile_options(lwipcontribexamples PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipcontribexamples PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipcontribexamples PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})

set(lwipcontribapps_SRCS
    ${LWIP_CONTRIB_DIR}/apps/httpserver/httpserver-netconn.c
    ${LWIP_CONTRIB_DIR}/apps/chargen/chargen.c
    ${LWIP_CONTRIB_DIR}/apps/udpecho/udpecho.c
    ${LWIP_CONTRIB_DIR}/apps/tcpecho/tcpecho.c
    ${LWIP_CONTRIB_DIR}/apps/shell/shell.c
    ${LWIP_CONTRIB_DIR}/apps/udpecho_raw/udpecho_raw.c
    ${LWIP_CONTRIB_DIR}/apps/tcpecho_raw/tcpecho_raw.c
    ${LWIP_CONTRIB_DIR}/apps/netio/netio.c
    ${LWIP_CONTRIB_DIR}/apps/ping/ping.c
    ${LWIP_CONTRIB_DIR}/apps/socket_examples/socket_examples.c
    ${LWIP_CONTRIB_DIR}/apps/rtp/rtp.c
)
add_library(lwipcontribapps EXCLUDE_FROM_ALL ${lwipcontribapps_SRCS})
target_compile_options(lwipcontribapps PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipcontribapps PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipcontribapps PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})

set(lwipcontribaddons_SRCS
    ${LWIP_CONTRIB_DIR}/addons/tcp_isn/tcp_isn.c
    ${LWIP_CONTRIB_DIR}/addons/ipv6_static_routing/ip6_route_table.c
#    ${LWIP_CONTRIB_DIR}/addons/netconn/external_resolve/dnssd.c
#    ${LWIP_CONTRIB_DIR}/addons/tcp_md5/tcp_md5.c
)
add_library(lwipcontribaddons EXCLUDE_FROM_ALL ${lwipcontribaddons_SRCS})
target_compile_options(lwipcontribaddons PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwipcontribaddons PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwipcontribaddons PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})
