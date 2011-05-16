
include_directories(
    BEFORE include/freebsd/src/sys
    include/freebsd
    include/freebsd/sys/include
    include/freebsd/dev/include
    include/freebsd/net/include
    include)

add_definitions(
    -D__NTDRIVER__
    -DKERNEL
    -DOSKIT)

list(APPEND SOURCE
    oskittcp/defaults.c
    oskittcp/in.c
    oskittcp/in_cksum.c
    oskittcp/in_pcb.c
    oskittcp/in_proto.c
    oskittcp/in_rmx.c
    oskittcp/inet_ntoa.c
    oskittcp/interface.c
    oskittcp/ip_input.c
    oskittcp/ip_output.c
    oskittcp/kern_clock.c
    oskittcp/kern_subr.c
    oskittcp/osenv.c
    oskittcp/param.c
    oskittcp/radix.c
    oskittcp/random.c
    oskittcp/raw_cb.c
    oskittcp/raw_ip.c
    oskittcp/raw_usrreq.c
    oskittcp/route.c
    oskittcp/rtsock.c
    oskittcp/scanc.c
    oskittcp/sleep.c
    oskittcp/tcp_input.c
    oskittcp/tcp_output.c
    oskittcp/tcp_subr.c
    oskittcp/tcp_usrreq.c
    oskittcp/tcp_debug.c
    oskittcp/tcp_timer.c
    oskittcp/uipc_domain.c
    oskittcp/uipc_mbuf.c
    oskittcp/uipc_socket.c
    oskittcp/uipc_socket2.c)

add_library(oskittcp ${SOURCE})
add_dependencies(oskittcp bugcodes)
