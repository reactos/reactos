<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ws2_32_winetest" type="win32cui" installbase="bin" installname="ws2_32_winetest.exe" allowwarnings="true">
	<include base="ws2_32_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>protocol.c</file>
	<file>sock.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ws2_32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
