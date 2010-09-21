<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="lwip" type="staticlibrary">
	<include base="lwip">src/include</include>
	<include base="lwip">src/include/ipv4</include>
	<directory name="src">
		<file>rosip.c</file>
		<file>rostcp.c</file>
		<file>rosmem.c</file>
		<file>sys_arch.c</file>
		<directory name="api">
			<file>err.c</file>
			<file>netbuf.c</file>
			<file>netifapi.c</file>
			<file>tcpip.c</file>
		</directory>
		<directory name="core">
			<file>init.c</file>
			<file>mem.c</file>
			<file>memp.c</file>
			<file>netif.c</file>
			<file>pbuf.c</file>
			<file>stats.c</file>
			<file>sys.c</file>
			<file>tcp_in.c</file>
			<file>tcp_out.c</file>
			<file>tcp.c</file>
			<directory name="ipv4">
				<file>inet_chksum.c</file>
				<file>inet.c</file>
				<file>ip.c</file>
				<file>ip_addr.c</file>
				<file>ip_frag.c</file>
			</directory>
		</directory>
	</directory>
</module>
