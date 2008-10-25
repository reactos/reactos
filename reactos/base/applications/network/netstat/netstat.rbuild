<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="netstat" type="win32cui" installbase="system32" installname="netstat.exe" allowwarnings="true">
	<include base="netstat">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>ws2_32</library>
	<library>snmpapi</library>
	<library>iphlpapi</library>
	<file>netstat.c</file>
	<file>netstat.rc</file>
</module>
