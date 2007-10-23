<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="finger" type="win32cui" installbase="system32" installname="finger.exe">
	<include base="finger">.</include>
	<define name="__USE_W32_SOCKETS" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>finger.c</file>
	<file>err.c</file>
	<file>getopt.c</file>
	<file>net.c</file>
	<file>finger.rc</file>
</module>
