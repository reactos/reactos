<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="odbccp32_winetest" type="win32cui" installbase="bin" installname="odbccp32_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="odbccp32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>odbccp32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>misc.c</file>
	<file>testlist.c</file>
</module>
