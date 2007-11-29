<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mapi32_winetest" type="win32cui" installbase="bin" installname="mapi32_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="mapi32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>imalloc.c</file>
	<file>prop.c</file>
	<file>util.c</file>
	<file>testlist.c</file>
</module>
