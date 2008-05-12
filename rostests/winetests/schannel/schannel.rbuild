<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="schannel_winetest" type="win32cui" installbase="bin" installname="schannel_winetest.exe" allowwarnings="true">
	<include base="schannel_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>main.c</file>
	<file>testlist.c</file>
</module>
</group>
