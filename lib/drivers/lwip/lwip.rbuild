<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="lwip" type="staticlibrary" allowwarnings="true">
	<include base="lwip">src/include</include>
	<include base="lwip">src/include/ipv4</include>
	<include base="tcpip">include</include>
	<directory name="src">
		<file>rosip.c</file>
		<file>rostcp.c</file>
		<file>rosmem.c</file>
		<file>sys_arch.c</file>
		<directory name="api">
			<file>api_lib.c</file>
			<file>api_msg.c</file>
			<file>err.c</file>
			<file>netbuf.c</file>
			<file>netdb.c</file>
			<file>netifapi.c</file>
			<file>sockets.c</file>
			<file>tcpip.c</file>
		</directory>
		<directory name="core">
			<file>def.c</file>
			<file>dhcp.c</file>
			<file>dns.c</file>
			<file>init.c</file>
			<file>mem.c</file>
			<file>memp.c</file>
			<file>netif.c</file>
			<file>pbuf.c</file>
			<file>raw.c</file>
			<file>stats.c</file>
			<file>sys.c</file>
			<file>tcp_in.c</file>
			<file>tcp_out.c</file>
			<file>tcp.c</file>
			<file>timers.c</file>
			<file>udp.c</file>
			<directory name="ipv4">
				<file>autoip.c</file>
				<file>icmp.c</file>
				<file>igmp.c</file>
				<file>inet_chksum.c</file>
				<file>inet.c</file>
				<file>ip.c</file>
				<file>ip_addr.c</file>
				<file>ip_frag.c</file>
			</directory>
			<!--directory name="ipv6">
				<file>icmp6.c</file>
				<file>inet6.c</file>
				<file>ip6_addr.c</file>
				<file>ip6.c</file>
			</directory-->
			<directory name="snmp">
				<file>asn1_dec.c</file>
				<file>asn1_enc.c</file>
				<file>mib_structs.c</file>
				<file>mib2.c</file>
				<file>msg_in.c</file>
				<file>msg_out.c</file>
			</directory>
		</directory>
	</directory>
</module>
