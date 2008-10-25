<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comcat_winetest" type="win32cui" installbase="bin" installname="comcat_winetest.exe" allowwarnings="true">
	<include base="comcat_winetest">.</include>
	<file>comcat.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
