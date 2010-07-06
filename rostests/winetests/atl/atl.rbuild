<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="atl_winetest" type="win32cui" installbase="bin" installname="atl_winetest.exe" allowwarnings="true">
	<include base="atl_winetest">.</include>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>atl</library>
	<file>atl_ax.c</file>
	<file>module.c</file>
	<file>testlist.c</file>
</module>
</group>
