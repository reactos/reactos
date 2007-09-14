<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="oskittcp" type="staticlibrary" allowwarnings="true">
	<define name="__REACTOS__">1</define>
	<define name="__NTDRIVER__"/>
	<define name="KERNEL"/>
	<define name="_DISABLE_TIDENTS"/>
	<define name="__USE_W32API"/>
	<define name="__NO_CTYPE_INLINES" />
	<include base="oskittcp">include/freebsd</include>
	<include base="oskittcp">include/freebsd/sys/include</include>
	<include base="oskittcp">include/freebsd/src/sys</include>
	<include base="oskittcp">include/freebsd/dev/include</include>
	<include base="oskittcp">include/freebsd/net/include</include>
	<include base="oskittcp">include</include>
	<directory name="oskittcp">
		<file>defaults.c</file>
		<file>in.c</file>
		<file>in_cksum.c</file>
		<file>in_pcb.c</file>
		<file>in_proto.c</file>
		<file>in_rmx.c</file>
		<file>inet_ntoa.c</file>
		<file>interface.c</file>
		<file>ip_input.c</file>
		<file>ip_output.c</file>
		<file>kern_clock.c</file>
		<file>kern_subr.c</file>
		<file>param.c</file>
		<file>radix.c</file>
		<file>random.c</file>
		<file>raw_cb.c</file>
		<file>raw_ip.c</file>
		<file>raw_usrreq.c</file>
		<file>route.c</file>
		<file>rtsock.c</file>
		<file>scanc.c</file>
		<file>sleep.c</file>
		<file>tcp_input.c</file>
		<file>tcp_output.c</file>
		<file>tcp_subr.c</file>
		<file>tcp_usrreq.c</file>
		<file>tcp_debug.c</file>
		<file>tcp_timer.c</file>
		<file>uipc_domain.c</file>
		<file>uipc_mbuf.c</file>
		<file>uipc_socket.c</file>
		<file>uipc_socket2.c</file>
	</directory>
</module>
