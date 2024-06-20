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

set(LWIP_TESTDIR ${LWIP_DIR}/test/unit)
set(LWIP_TESTFILES
	${LWIP_TESTDIR}/lwip_unittests.c
	${LWIP_TESTDIR}/api/test_sockets.c
	${LWIP_TESTDIR}/arch/sys_arch.c
	${LWIP_TESTDIR}/core/test_def.c
	${LWIP_TESTDIR}/core/test_dns.c
	${LWIP_TESTDIR}/core/test_mem.c
	${LWIP_TESTDIR}/core/test_netif.c
	${LWIP_TESTDIR}/core/test_pbuf.c
	${LWIP_TESTDIR}/core/test_timers.c
	${LWIP_TESTDIR}/dhcp/test_dhcp.c
	${LWIP_TESTDIR}/etharp/test_etharp.c
	${LWIP_TESTDIR}/ip4/test_ip4.c
	${LWIP_TESTDIR}/ip6/test_ip6.c
	${LWIP_TESTDIR}/mdns/test_mdns.c
	${LWIP_TESTDIR}/mqtt/test_mqtt.c
	${LWIP_TESTDIR}/tcp/tcp_helper.c
	${LWIP_TESTDIR}/tcp/test_tcp_oos.c
	${LWIP_TESTDIR}/tcp/test_tcp_state.c
	${LWIP_TESTDIR}/tcp/test_tcp.c
	${LWIP_TESTDIR}/udp/test_udp.c
	${LWIP_TESTDIR}/ppp/test_pppos.c
)
