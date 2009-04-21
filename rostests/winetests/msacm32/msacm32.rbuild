<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msacm32_winetest" type="win32cui" installbase="bin" installname="msacm32_winetest.exe" allowwarnings="true">
	<include base="msacm32_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>kernel32</library>
	<library>msacm32</library>
	<file>msacm.c</file>
	<file>testlist.c</file>
</module>
</group>
