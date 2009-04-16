<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="lz32_winetest" type="win32cui" installbase="bin" installname="lz32_winetest.exe" allowwarnings="true">
	<include base="lz32_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>lzexpand_main.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>lz32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
