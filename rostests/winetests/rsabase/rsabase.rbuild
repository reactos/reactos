<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rsabase_winetest" type="win32cui" installbase="bin" installname="rsabase_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="rsabase_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>rsabase.c</file>
	<file>testlist.c</file>
</module>
