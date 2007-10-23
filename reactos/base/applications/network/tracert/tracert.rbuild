<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="tracert" type="win32cui" installbase="system32" installname="tracert.exe">
	<include base="tracert">.</include>
	<define name="__USE_W32_SOCKETS" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>tracert.c</file>
	<file>tracert.rc</file>
</module>
