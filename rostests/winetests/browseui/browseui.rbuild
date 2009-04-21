<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="browseui_winetest" type="win32cui" installbase="bin" installname="browseui_winetest.exe" allowwarnings="true">
	<include base="browseui_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>autocomplete.c</file>
	<file>testlist.c</file>
</module>
</group>
