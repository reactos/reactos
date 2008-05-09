<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mapi32_winetest" type="win32cui" installbase="bin" installname="mapi32_winetest.exe" allowwarnings="true">
	<include base="mapi32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>imalloc.c</file>
	<file>prop.c</file>
	<file>util.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
