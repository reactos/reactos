<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="browseui_winetest" type="win32cui" installbase="bin" installname="browseui_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="browseui_winetest">.</include>
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
