<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="schannel_winetest" type="win32cui" installbase="bin" installname="schannel_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="schannel_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>main.c</file>
	<file>testlist.c</file>
</module>
</group>
