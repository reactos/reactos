<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="avifil32_winetest" type="win32cui" installbase="bin" installname="avifil32_winetest.exe" allowwarnings="true">
	<include base="avifil32_winetest">.</include>
	<library>avifil32</library>
	<library>wine</library>
	<file>api.c</file>
	<file>testlist.c</file>
</module>
</group>
