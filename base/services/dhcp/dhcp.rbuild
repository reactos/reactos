<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dhcp" type="win32cui" installbase="system32" installname="dhcp.exe" allowwarnings="true">
	<include base="dhcp">.</include>
	<include base="dhcp">include</include>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<library>advapi32</library>
	<file>adapter.c</file>
	<file>alloc.c</file>
	<file>api.c</file>
	<file>compat.c</file>
	<file>dhclient.c</file>
	<file>dispatch.c</file>
	<file>hash.c</file>
	<file>options.c</file>
	<file>pipe.c</file>
	<file>privsep.c</file>
	<file>socket.c</file>
	<file>tables.c</file>
	<file>timer.c</file>
	<file>util.c</file>
	<file>dhcp.rc</file>
	<directory name="include">
		<pch>rosdhcp.h</pch>
	</directory>
</module>
