<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wlanapi_winetest" type="win32cui" installbase="bin" installname="wlanapi_winetest.exe" allowwarnings="true">
	<include base="wininet_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>wlanapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>wlanapi.c</file>
	<file>testlist.c</file>
</module>
</group>
