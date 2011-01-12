<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ping" type="win32cui" installbase="system32" installname="ping.exe" unicode="true">
	<include base="ping">.</include>
	<define name="__USE_W32_SOCKETS" />
	<library>ws2_32</library>
	<file>ping.c</file>
	<file>ping.rc</file>
</module>
