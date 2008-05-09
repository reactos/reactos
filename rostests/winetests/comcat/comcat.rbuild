<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comcat_winetest" type="win32cui" installbase="bin" installname="comcat_winetest.exe" allowwarnings="true">
	<include base="comcat_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
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
