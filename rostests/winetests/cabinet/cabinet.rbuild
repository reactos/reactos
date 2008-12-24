<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet_winetest" type="win32cui" installbase="bin" installname="cabinet_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="cabinet_winetest">.</include>
	<file>extract.c</file>
	<file>fdi.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cabinet</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
