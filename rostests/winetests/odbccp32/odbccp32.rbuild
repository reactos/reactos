<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="odbccp32_winetest" type="win32cui" installbase="bin" installname="odbccp32_winetest.exe" allowwarnings="true">
	<include base="odbccp32_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>misc.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>odbccp32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
