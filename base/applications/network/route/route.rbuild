<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="route" type="win32cui" installbase="system32" installname="route.exe" unicode="yes">
	<include base="route">.</include>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<file>route.c</file>
	<file>route.rc</file>
</module>
