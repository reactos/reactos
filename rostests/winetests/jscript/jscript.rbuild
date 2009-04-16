<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="jscript_winetest" type="win32cui" installbase="bin" installname="jscript_winetest.exe" allowwarnings="true">
	<include base="jscript_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>jscript.c</file>
	<file>run.c</file>
	<file>testlist.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
