<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="iphlpapi_winetest" type="win32cui" installbase="bin" installname="iphlpapi_winetest.exe" allowwarnings="true">
	<include base="iphlpapi_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>iphlpapi.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
