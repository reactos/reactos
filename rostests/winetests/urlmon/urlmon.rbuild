<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="urlmon_winetest" type="win32cui" installbase="bin" installname="urlmon_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="urlmon_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>generated.c</file>
	<file>misc.c</file>
	<file>protocol.c</file>
	<file>stream.c</file>
	<file>url.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>urlmon</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
