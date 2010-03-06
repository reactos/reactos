<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="xmllite_winetest" type="win32cui" installbase="bin" installname="xmllite_winetest.exe" allowwarnings="true">
	<include base="wininet_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>xmllite</library>
	<library>ole32</library>
	<library>ntdll</library>
	<file>reader.c</file>
	<file>testlist.c</file>
</module>
</group>
