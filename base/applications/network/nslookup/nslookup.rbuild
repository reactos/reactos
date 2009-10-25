<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="nslookup" type="win32cui" installbase="system32" installname="nslookup.exe">
	<include base="nslookup">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>ws2_32</library>
	<library>snmpapi</library>
	<library>iphlpapi</library>
	<file>nslookup.c</file>
	<file>utility.c</file>
	<file>nslookup.rc</file>
</module>
