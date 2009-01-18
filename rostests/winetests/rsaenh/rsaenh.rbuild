<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rsaenh_winetest" type="win32cui" installbase="bin" installname="rsaenh_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="rsaenh_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>rsaenh.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
