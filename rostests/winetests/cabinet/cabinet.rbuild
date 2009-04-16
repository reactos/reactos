<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet_winetest" type="win32cui" installbase="bin" installname="cabinet_winetest.exe" allowwarnings="true">
	<include base="cabinet_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>extract.c</file>
	<file>fdi.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cabinet</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
