<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="tracert" type="win32cui" installbase="system32" installname="tracert.exe">
	<include base="tracert">.</include>
	<define name="__USE_W32_SOCKETS" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>tracert.c</file>
	<file>tracert.rc</file>
</module>
