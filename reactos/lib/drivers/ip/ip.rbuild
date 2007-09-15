<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ip" type="staticlibrary" allowwarnings="true">
	<define name="__NTDRIVER__"/>
	<define name="__USE_W32API"/>
	<include base="tcpip">include</include>
	<include base="oskittcp">include</include>
	<directory name="network">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>checksum.S</file>
			</directory>
		</if>
		<file>address.c</file>
		<file>arp.c</file>
		<file>checksum.c</file>
		<file>icmp.c</file>
		<file>interface.c</file>
		<file>ip.c</file>
		<file>loopback.c</file>
		<file>memtrack.c</file>
		<file>neighbor.c</file>
		<file>ports.c</file>
		<file>receive.c</file>
		<file>router.c</file>
		<file>routines.c</file>
		<file>transmit.c</file>
	</directory>
	<directory name="transport">
		<directory name="datagram">
			<file>datagram.c</file>
		</directory>
		<directory name="rawip">
			<file>rawip.c</file>
		</directory>
		<directory name="tcp">
			<file>accept.c</file>
			<file>event.c</file>
			<file>if.c</file>
			<file>tcp.c</file>
		</directory>
		<directory name="udp">
			<file>udp.c</file>
		</directory>
	</directory>
</module>
