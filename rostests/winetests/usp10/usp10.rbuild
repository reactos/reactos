<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="usp10_winetest" type="win32cui" installbase="bin" installname="usp10_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="usp10_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>usp10.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>usp10</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
