<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="imagehlp_winetest" type="win32cui" installbase="bin" installname="imagehlp_winetest.exe" allowwarnings="true">
	<include base="imagehlp_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>image.c</file>
	<file>integrity.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>ntdll</library>
</module>
</group>
