<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="uxtheme_winetest" type="win32cui" installbase="bin" installname="uxtheme_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="uxtheme_winetest">.</include>
	<file>system.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
